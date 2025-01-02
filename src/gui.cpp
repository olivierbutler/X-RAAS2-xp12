/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license in the file COPYING
 * or http://www.opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file COPYING.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2017 Saso Kiselkov. All rights reserved.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMMenus.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>
#include <XPStandardWidgets.h>
#include <XPWidgets.h>

#include <acfutils/assert.h>
#include <acfutils/list.h>
#include <acfutils/time.h>
#include <acfutils/widget.h>

#include "dbg_gui.h"
#include "init_msg.h"
#include "nd_alert.h"
#include "xraas2.h"
#include "xraas_cfg.h"

#include "xp_img_window.h"

#include "gui.h"
#include "gui_tooltips.h"

#ifdef XRAAS_IS_EMBEDDED
#define XRAAS_MENU_NAME_STANDALONE "X-RAAS (embedded)"
#if ACF_TYPE == FF_A320_ACF_TYPE
#define XRAAS_MENU_NAME "X-RAAS (Airbus A320)"
#else /* !ACF_TYPE */
#define XRAAS_MENU_NAME "X-RAAS (embedded)"
#endif /* !ACF_TYPE */
#else  /* !XRAAS_IS_EMBEDDED */
#define XRAAS_MENU_NAME "X-RAAS"
#endif /* !XRAAS_IS_EMBEDDED */

#define CONFIG_GUI_CMD_NAME "X-RAAS configuration..."
#define DBG_GUI_TOGGLE_CMD_NAME "Toggle debug overlay"
#define RAAS_RESET_CMD_NAME "Reset"
#define RECREATE_CACHE_CMD_NAME "Recreate data cache"

#if IBM
#define NEWLINE "\r\n"
#else /* !IBM */
#define NEWLINE "\n"
#endif /* !IBM */

#define COPYRIGHT1     \
    XRAAS_MENU_NAME    \
    " " XRAAS2_VERSION \
    "       © 2017-2025 S.Kiselkov, O.Butler. All rights reserved."
#define COPYRIGHT2                                     \
    "X-RAAS is open-source software. See COPYING for " \
    "more information."
#define TOOLTIP_HINT                                   \
    "Hint: hover your mouse cursor over any label to " \
    "show a short description of what it does."

const char *save_airline = "> SAVE airline configuration";
const char *save_aircraft = "> SAVE aircraft configuration";
const char *save_global = "> SAVE global configuration";
const char *reset_airline = "RESET airline configuration";
const char *reset_aircraft = "RESET aircraft configuration";
const char *reset_global = "RESET global configuration";

const char *config_status = NULL;
bool_t config_status_onerror = B_FALSE;
const char *reset_airline_success = "Airline configuration reset successful";
const char *reset_aircraft_success = "Aircraft configuration reset successful";
const char *reset_global_success = "Global configuration reset successful";
const char *saved_airline_success = "Airline configuration saved";
const char *saved_aircraft_success = "Aircraft configuration saved";
const char *saved_global_success = "Global configuration reset saved";

const char *save_airline_not_supported = "Saving the configuration for the default livery is not supported";
const char *error_writing_cfg = "Error writing configuration, see Log.txt for details.";
const char *error_reset_cfg = "Error resetting configuration, see Log.txt for details.";

static bool_t gui_inited = B_FALSE;


enum
{
    CONFIG_GUI_CMD,
    DBG_GUI_TOGGLE_CMD,
    RAAS_RESET_CMD,
    RECREATE_CACHE_CMD
};

#define MAIN_WINDOW_W 800
#define MAIN_WINDOW_H 700
#define TABS_HEIGHT 450
#define BUTTON_WIDTH 250

#define ROUNDED 8.0f
#define TOOLTIP_BG_COLOR ImVec4(0.2f, 0.3f, 0.8f, 1.0f)
#define ERROR_COLOR ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define SUCCESS_COLOR ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define LINE_COLOR IM_COL32(255, 255, 255, 255)
#define LINE_THICKNESS 1.0f
#define BUTTON_DISABLED 0.5f

void create_setup_window(void);
static void save_config(xraas_state_config_t *, conf_target_t);
static void reset_config(conf_target_t);
bool imgui_initied = B_FALSE;
const char *default_font_label = "default";

static int plugins_menu_item;
static XPLMMenuID root_menu;
static int dbg_gui_menu_item;

static XPLMCommandRef toggle_cfg_gui_cmd = NULL, toggle_dbg_gui_cmd = NULL,
                      recreate_cache_cmd = NULL, raas_reset_cmd = NULL;

// ## at the beginning of the string is mandatory for imgui
// they will not be dispayed in the gui window
static const char *monitor_names[NUM_MONITORS] = {
    "##Approaching runway on ground",    /* APCH_RWY_ON_GND_MON */
    "##Approaching runway in air",       /* APCH_RWY_IN_AIR_MON */
    "##Approaching short runway in air", /* APCH_RWY_IN_AIR_SHORT_MON */
    "##On runway lined up",              /* ON_RWY_LINEUP_MON */
    "##On short runway lined up",        /* ON_RWY_LINEUP_SHORT_MON */
    "##On runway lined up flaps",        /* ON_RWY_FLAP_MON */
    "##Short runway takeoff",            /* ON_RWY_TKOFF_SHORT_MON */
    "##On runway extended holding",      /* ON_RWY_HOLDING_MON */
    "##Taxiway takeoff",                 /* TWY_TKOFF_MON */
    "##Distance remaining on landing",   /* DIST_RMNG_LAND_MON */
    "##Distance remaining on RTO",       /* DIST_RMNG_RTO_MON */
    "##Taxiway landing",                 /* TWY_LAND_MON */
    "##Approaching runway end",          /* RWY_END_MON */
    "##Too high approach (upper gate)",  /* APCH_TOO_HIGH_UPPER_MON */
    "##Too high approach (lower gate)",  /* APCH_TOO_HIGH_LOWER_MON */
    "##Too fast approach (upper gate)",  /* APCH_TOO_FAST_UPPER_MON */
    "##Too fast approach (lower gate)",  /* APCH_TOO_FAST_LOWER_MON */
    "##Landing flaps (upper gate)",      /* APCH_FLAPS_UPPER_MON */
    "##Landing flaps (lower gate)",      /* APCH_FLAPS_LOWER_MON */
    "##Unstable approach",               /* APCH_UNSTABLE_MON */
    "##QNE altimeter setting",           /* ALTM_QNE_MON */
    "##QNH altimeter setting",           /* ALTM_QNH_MON */
    "##QFE altimeter setting",           /* ALTM_QFE_MON */
    "##Long landing",                    /* LONG_LAND_MON */
    "##Late rotation"                    /* LATE_ROTATION_MON */
};

static char *gen_config(xraas_state_config_t *config)
{
    char *conf_text = NULL;
    size_t conf_sz = 0;

    append_format(
        &conf_text, &conf_sz,
        "# This configuration file was automatically generated using the\n"
        "# X-RAAS configuration GUI." NEWLINE NEWLINE);

#define GEN_BOOL_CONF(widget)                             \
    append_format(                                        \
        &conf_text, &conf_sz, "%s = %s" NEWLINE, #widget, \
        (config->widget                                   \
             ? "true"                                     \
             : "false"))

    GEN_BOOL_CONF(enabled);
    GEN_BOOL_CONF(allow_helos);
    GEN_BOOL_CONF(startup_notify);
    GEN_BOOL_CONF(auto_disable_notify);
    GEN_BOOL_CONF(use_imperial);
    GEN_BOOL_CONF(us_runway_numbers);
    for (int i = 0; i < NUM_MONITORS; i++)
        append_format(
            &conf_text, &conf_sz, "%s = %s" NEWLINE, monitor_conf_keys[i],
            config->monitors[i]
                ? "true"
                : "false");
    GEN_BOOL_CONF(disable_ext_view);
    GEN_BOOL_CONF(override_electrical);
    GEN_BOOL_CONF(override_replay);
    GEN_BOOL_CONF(speak_units);
#if !LIN
    GEN_BOOL_CONF(use_tts);
#endif /* !LIN */
    GEN_BOOL_CONF(voice_female);
    GEN_BOOL_CONF(say_deep_landing);
    GEN_BOOL_CONF(nd_alerts_enabled);
    GEN_BOOL_CONF(nd_alert_overlay_enabled);
    GEN_BOOL_CONF(nd_alert_overlay_force);
    GEN_BOOL_CONF(openal_shared);

#undef GEN_BOOL_CONF

#define GEN_NUM_CONF(field, fmt)                                         \
    do                                                                   \
    {                                                                    \
        append_format(&conf_text, &conf_sz, "%s = " fmt NEWLINE, #field, \
                      config->field);                                    \
    } while (0)

#if ACF_TYPE == NO_ACF_TYPE
    /* These don't make sense in type-specific embedded scenarios */
    GEN_NUM_CONF(min_engines, "%d");
    GEN_NUM_CONF(min_mtow, "%d");
#endif
    GEN_NUM_CONF(min_takeoff_dist, "%d");
    GEN_NUM_CONF(min_landing_dist, "%d");
    GEN_NUM_CONF(min_rotation_dist, "%d");
    GEN_NUM_CONF(min_rotation_angle, "%g");
    GEN_NUM_CONF(stop_dist_cutoff, "%d");
    GEN_NUM_CONF(on_rwy_warn_initial, "%d");
    GEN_NUM_CONF(on_rwy_warn_repeat, "%d");
    GEN_NUM_CONF(on_rwy_warn_max_n, "%d");
    GEN_NUM_CONF(long_land_lim_abs, "%d");
    GEN_NUM_CONF(nd_alert_timeout, "%d");

    if (strlen(config->nd_alert_overlay_font) != 0 && strcmp(config->nd_alert_overlay_font, default_font_label) != 0 &&
        strcmp(config->nd_alert_overlay_font, ND_alert_overlay_default_font) != 0)
        append_format(&conf_text, &conf_sz, "nd_alert_overlay_font = %s" NEWLINE,
                      config->nd_alert_overlay_font);
    GEN_NUM_CONF(nd_alert_overlay_font_size, "%d");
    GEN_NUM_CONF(nd_alert_filter, "%d");
#undef GEN_NUM_CONF

#define GEN_FRACT_CONF(scrollbar)                                                              \
    do                                                                                         \
    {                                                                                          \
        append_format(&conf_text, &conf_sz, "%s = %g" NEWLINE, #scrollbar, config->scrollbar); \
    } while (0)

    GEN_FRACT_CONF(long_land_lim_fract);
    GEN_FRACT_CONF(voice_volume);
#if ACF_TYPE == NO_ACF_TYPE
    /* We get these directly from the FMS of the host aircraft */
    GEN_FRACT_CONF(min_landing_flap);
    GEN_FRACT_CONF(min_takeoff_flap);
    GEN_FRACT_CONF(max_takeoff_flap);
#endif /* ACF_TYPE == NO_ACF_TYPE */
    GEN_FRACT_CONF(gpa_limit_max);
    GEN_FRACT_CONF(gpa_limit_mult);
    //   GEN_FRACT_CONF(nd_alert_filter);

#undef GEN_FRACT_CONF

    return (conf_text);
}

typedef struct
{
    const char *string;
    bool use_chinese;
    const char *value;
} comboList_t_;

typedef struct
{
    comboList_t_ *combo_list;
    int list_size;
    const char *name;
    int selected;
} comboList_t;

comboList_t_ nd_alert_filter_list_[] = {
    {"ALL", B_FALSE, "0"},
    {"NON-R", B_FALSE, "1"},
    {"CAUT", B_FALSE, "2"}};

comboList_t nd_alert_filter_list = {nd_alert_filter_list_, IM_ARRAYSIZE(nd_alert_filter_list_),
                                    "##nd_alert_filter", 0};

class SettingsWindow : public XPImgWindow
{
public:
    SettingsWindow(WndMode _mode = WND_MODE_FLOAT_CENTERED);
    void SetVisible(bool) override;
    void CenterText(const char *text);
    bool CenterButton(const char *text);
    void Tooltip(const char *tip);
    bool_t comboList(comboList_t *list);
    bool_t save_disabled;

    ~SettingsWindow() {}

    bool_t getIsDestroy(void) { return is_destroy; }

private:
    bool_t is_destroy;
    xraas_state_config_t config;
    void LoadConfig(void);
    void comboList_free(comboList_t *list);

protected:
    void buildInterface() override;
};

SettingsWindow::SettingsWindow(WndMode _mode)
    : XPImgWindow(_mode, WND_STYLE_SOLID,
                  WndRect(0, MAIN_WINDOW_W, MAIN_WINDOW_H, 0))
{
    SetWindowTitle(XRAAS_MENU_NAME " Configuration");
    SetWindowResizingLimits(MAIN_WINDOW_W, MAIN_WINDOW_H, MAIN_WINDOW_W,
                            MAIN_WINDOW_H);
    LoadConfig();
}

void SettingsWindow::SetVisible(bool inIsVisible)
{
    LoadConfig();
    XPImgWindow::SetVisible(inIsVisible);
}
void SettingsWindow::LoadConfig(void)
{
    config = xraas_state->config;
    if (strcmp(config.nd_alert_overlay_font,
               ND_alert_overlay_default_font) == 0)
    {
        strcpy(config.nd_alert_overlay_font, default_font_label);
    }
}

void SettingsWindow::comboList_free(comboList_t *list)
{
    for (int i = 0; i < list->list_size; i++)
    {
        free((void *)list->combo_list[i].string);
        free((void *)list->combo_list[i].value);
    }
    free((void *)list->combo_list);
    list->list_size = 0;
}

bool_t SettingsWindow::comboList(comboList_t *list)
{
    bool_t is_changed = B_FALSE;
    comboList_t_ combo_previous = list->combo_list[list->selected];
    if (combo_previous.use_chinese)
    {
        ImGui::PushFont(ImgWindow::fontChinese.get());
    }

    bool_t lang_combo = ImGui::BeginCombo(list->name, combo_previous.string, 0);
    if (combo_previous.use_chinese)
    {
        ImGui::PopFont();
    }
    if (lang_combo)
    {
        for (int i = 0; i < list->list_size; i++)
        {
            int is_selected = (i == list->selected);
            if (list->combo_list[i].use_chinese)
            {
                ImGui::PushFont(ImgWindow::fontChinese.get());
            }
            if (ImGui::Selectable(list->combo_list[i].string, is_selected))
            {
                is_changed = (list->selected != i);
                list->selected = i;
            }
            if (list->combo_list[i].use_chinese)
            {
                ImGui::PopFont();
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return is_changed;
}

void SettingsWindow::CenterText(const char *text)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // Calculate horizontal position to center the text
    float textPosX = (windowSize.x - textSize.x) * 0.5f;

    // Make sure it doesn't go out of bounds
    if (textPosX > 0.0f)
        ImGui::SetCursorPosX(textPosX);

    ImGui::Text("%s", text);
}

bool SettingsWindow::CenterButton(const char *text)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // Calculate horizontal position to center the text
    float textPosX = (windowSize.x - textSize.x) * 0.5f;

    // Make sure it doesn't go out of bounds
    if (textPosX > 0.0f)
        ImGui::SetCursorPosX(textPosX);

    return ImGui::Button(text);
}

void SettingsWindow::Tooltip(const char *tip)
{
    // do the tooltip
    if (tip && ImGui::IsItemHovered())
    {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, TOOLTIP_BG_COLOR);
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(300);
        ImGui::TextUnformatted(tip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
    }
}
void SettingsWindow::buildInterface()
{

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ROUNDED);
    float tableWidth = ImGui::GetContentRegionAvail().x;
    float combowithWidth = tableWidth * 0.25f;

#define SECTION_TILE(title)                                               \
    do                                                                    \
    {                                                                     \
        ImGui::TableNextRow();                                            \
        ImGui::TableNextColumn();                                         \
        ImVec2 rowMin = ImGui::GetItemRectMin();                          \
        ImGui::Text("%s", title);                                         \
        ImGui::TableNextColumn();                                         \
        ImGui::Text(" ");                                                 \
        ImVec2 rowMax = ImGui::GetItemRectMax();                          \
        ImVec2 rowBottomStart = ImVec2(rowMin.x, rowMax.y);               \
        ImVec2 rowBottomEnd = ImVec2(rowMax.x, rowMax.y);                 \
        ImGui::GetWindowDrawList()->AddLine(rowBottomStart, rowBottomEnd, \
                                            LINE_COLOR, LINE_THICKNESS);  \
    } while (0)

#define BLANK_ROW             \
    ImGui::TableNextRow();    \
    ImGui::TableNextColumn(); \
    ImGui::Text(" ");         \
    ImGui::TableNextRow()

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define CHECK_BOX(var, label, tool_tip) \
    ImGui::TableNextRow();              \
    ImGui::TableNextColumn();           \
    ImGui::Text("%s", label);           \
    Tooltip(tool_tip);                  \
    ImGui::TableNextColumn();           \
    ImGui::Checkbox("##_" STRINGIFY(__LINE__), (bool *)&config.var)

#define CHECK_BOX_MONITOR(var, label, tool_tip) \
    ImGui::TableNextRow();                      \
    ImGui::TableNextColumn();                   \
    ImGui::Text("%s", label + 2);               \
    Tooltip(tool_tip);                          \
    ImGui::TableNextColumn();                   \
    ImGui::Checkbox(label, (bool *)&config.var)

#define COMBO_LIST(list, var, label, tool_tip)   \
    do                                           \
    {                                            \
        list.selected = config.var;              \
        ImGui::TableNextRow();                   \
        ImGui::TableNextColumn();                \
        ImGui::Text("%s", label);                \
        Tooltip(tool_tip);                       \
        ImGui::TableNextColumn();                \
        ImGui::SetNextItemWidth(combowithWidth); \
        comboList(&list);                        \
        config.var = list.selected;              \
    } while (0)

#define SLIDER_FLOAT(var, label, tool_tip, format, suffixe, min, max, multiplier) \
    do                                                                            \
    {                                                                             \
        float temp = config.var * multiplier;                                     \
        ImGui::TableNextRow();                                                    \
        ImGui::TableNextColumn();                                                 \
        ImGui::Text("%s", label);                                                 \
        Tooltip(tool_tip);                                                        \
        ImGui::TableNextColumn();                                                 \
        ImGui::SetNextItemWidth(combowithWidth);                                  \
        ImGui::SliderFloat("##" #var, &temp, min, max, format);                   \
        ImGui::SameLine();                                                        \
        ImGui::TextUnformatted(suffixe);                                          \
        config.var = temp / multiplier;                                           \
    } while (0)

#define INPUT_INT(var, label, tool_tip, suffix, step, step_fast, min, max) \
    ImGui::TableNextRow();                                                 \
    ImGui::TableNextColumn();                                              \
    ImGui::Text("%s", label);                                              \
    Tooltip(tool_tip);                                                     \
    ImGui::TableNextColumn();                                              \
    ImGui::SetNextItemWidth(combowithWidth);                               \
    ImGui::PushID(label);                                                  \
    ImGui::InputInt(suffix, &config.var, step, step_fast);                 \
    ImGui::PopID();                                                        \
    if (config.var > max)                                                  \
        config.var = max;                                                  \
    if (config.var < min)                                                  \
        config.var = min;

#define INPUT_FLOAT(var, label, tool_tip, suffix, step, step_fast, min, max, \
                    format)                                                  \
    ImGui::TableNextRow();                                                   \
    ImGui::TableNextColumn();                                                \
    ImGui::Text("%s", label);                                                \
    Tooltip(tool_tip);                                                       \
    ImGui::TableNextColumn();                                                \
    ImGui::SetNextItemWidth(combowithWidth);                                 \
    ImGui::PushID(label);                                                    \
    ImGui::InputFloat(suffix, &config.var, step, step_fast, format);         \
    ImGui::PopID();                                                          \
    if (config.var > max)                                                    \
        config.var = max;                                                    \
    if (config.var < min)                                                    \
        config.var = min;

#define INPUT_DOUBLE(var, label, tool_tip, suffix, step, step_fast, min, max, \
                     format)                                                  \
    ImGui::TableNextRow();                                                    \
    ImGui::TableNextColumn();                                                 \
    ImGui::Text("%s", label);                                                 \
    Tooltip(tool_tip);                                                        \
    ImGui::TableNextColumn();                                                 \
    ImGui::SetNextItemWidth(combowithWidth);                                  \
    ImGui::PushID(label);                                                     \
    ImGui::InputDouble(suffix, &config.var, step, step_fast, format);         \
    ImGui::PopID();                                                           \
    if (config.var > max)                                                     \
        config.var = max;                                                     \
    if (config.var < min)                                                     \
        config.var = min;

#define INPUT_TEXT(var, label, tool_tip)                        \
    ImGui::TableNextRow();                                      \
    ImGui::TableNextColumn();                                   \
    ImGui::Text("%s", label);                                   \
    Tooltip(tool_tip);                                          \
    ImGui::TableNextColumn();                                   \
    ImGui::SetNextItemWidth(combowithWidth);                    \
    ImGui::PushID(label);                                       \
    ImGui::InputText("", config.var, IM_ARRAYSIZE(config.var)); \
    ImGui::PopID();

    // Begin the tab bar
    if (ImGui::BeginTabBar("MyTabBar"))
    {

        // Tab 1
        if (ImGui::BeginTabItem("Global settings"))
        {
            ImGui::BeginChild("FixedHeightArea1", ImVec2(0, TABS_HEIGHT), true);
            ImGui::Text(" ");
            if (ImGui::BeginTable("##main_table1", 2, ImGuiTableFlags_SizingStretchSame,
                                  ImVec2(tableWidth, 0)))
            {

                CHECK_BOX(enabled, "Enabled", enabled_tooltip);

                CHECK_BOX(use_imperial, "Call out distances in feet", use_imperial_tooltip);
                CHECK_BOX(us_runway_numbers, "US runway numbers",
                          us_runway_numbers_tooltip);
                CHECK_BOX(voice_female, "Voice gender female", voice_female_tooltip);
                CHECK_BOX(speak_units, "Speak units", speak_units_tooltip);
                CHECK_BOX(say_deep_landing, "Say 'DEEP LANDING'", say_deep_landing_tooltip);
#ifndef XRAAS_IS_EMBEDDED
                CHECK_BOX(allow_helos, "Start up in helicopters", allow_helos_tooltip);
                CHECK_BOX(startup_notify, "Show startup notification",
                          startup_notify_tooltip);
                CHECK_BOX(auto_disable_notify, "Notify when X-RAAS auto-inhibits",
                          auto_disable_notify_tooltip);
#endif /* !XRAAS_IS_EMBEDDED */
                CHECK_BOX(disable_ext_view, "Silence in external views",
                          disable_ext_view_tooltip);
                CHECK_BOX(nd_alerts_enabled, "Visual alerts", nd_alerts_enabled_tooltip);
                CHECK_BOX(nd_alert_overlay_enabled, "Visual alert overlay",
                          nd_alert_overlay_enabled_tooltip);
                CHECK_BOX(nd_alert_overlay_force, "Always show visual alerts using overlay",
                          nd_alert_overlay_force_tooltip);
#ifndef XRAAS_IS_EMBEDDED
                CHECK_BOX(override_electrical, "Override electrical check",
                          override_electrical_tooltip);
#endif /* !XRAAS_IS_EMBEDDED */
                CHECK_BOX(override_replay, "Show in replay mode", override_replay_tooltip);

#if LIN
                ImGui::BeginDisabled();
#endif /* !LIN */
                CHECK_BOX(use_tts, "Use Text-To-Speech", use_tts_tooltip);
#if LIN
                ImGui::EndDisabled();
#endif /* !LIN */

                CHECK_BOX(openal_shared, "Shared audio driver context",
                          openal_shared_tooltip);

                ImGui::EndTable();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // Tab 2
        if (ImGui::BeginTabItem("Monitor settings"))
        {
            ImGui::BeginChild("FixedHeightArea2", ImVec2(0, TABS_HEIGHT), true);
            ImGui::Text(" ");
            if (ImGui::BeginTable("##main_table2", 2, ImGuiTableFlags_SizingStretchSame,
                                  ImVec2(tableWidth, 0)))
            {
                for (int i = 0; i < NUM_MONITORS; i++)
                {
#if ACF_TYPE == FF_A320_ACF_TYPE
                    if (i == ON_RWY_FLAP_MON)
                    {
                        ImGui::BeginDisabled();
                    }
#endif /* ACF_TYPE == FF_A320_ACF_TYPE */
                    CHECK_BOX_MONITOR(monitors[i], monitor_names[i], monitor_tooltips[i]);
#if ACF_TYPE == FF_A320_ACF_TYPE
                    if (i == ON_RWY_FLAP_MON)
                    {
                        ImGui::EndDisabled();
                    }
#endif /* ACF_TYPE == FF_A320_ACF_TYPE */
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // Tab 3
        if (ImGui::BeginTabItem("Customize parameters"))
        {
            ImGui::BeginChild("FixedHeightArea3", ImVec2(0, TABS_HEIGHT), true);
            ImGui::Text(" ");
            if (ImGui::BeginTable("##main_table3", 2, ImGuiTableFlags_SizingStretchSame,
                                  ImVec2(tableWidth, 0)))
            {

                SLIDER_FLOAT(voice_volume, "Audio volume", voice_volume_tooltip, "%.0f %%", "",
                             0, 100, 100);

#if ACF_TYPE == NO_ACF_TYPE
                INPUT_INT(min_engines, "Minimum number of engines", min_engines_tooltip, "",
                          1, 1, 1, 10);
                INPUT_INT(min_mtow, "Minimum MTOW", min_mtow_tooltip, "kg", 1000, 10000, 0,
                          200000);
#endif /* ACF_TYPE == NO_ACF_TYPE */
                INPUT_INT(min_takeoff_dist, "Minimum takeoff distance",
                          min_takeoff_dist_tooltip, "m", 100, 100, 0, 10000);
                INPUT_INT(min_landing_dist, "Minimum landing distance",
                          min_landing_dist_tooltip, "m", 100, 100, 0, 10000);
                INPUT_INT(min_rotation_dist, "Minimum rotation distance",
                          min_rotation_dist_tooltip, "m", 100, 100, 0, 10000);

                INPUT_DOUBLE(min_rotation_angle, "Minimum rotation angle",
                             min_rotation_angle_tooltip, "deg", 0.1f, 1.0f, 0, 10, "%.1f");

                INPUT_INT(stop_dist_cutoff, "Runway remaining cutoff length",
                          stop_dist_cutoff_tooltip, "m", 100, 100, 0, 10000);

                INPUT_INT(on_rwy_warn_initial, "Runway extended holding (initial)",
                          on_rwy_warn_initial_tooltip, "sec", 1, 10, 0, 1000);
                INPUT_INT(on_rwy_warn_repeat, "Runway extended holding (repeat)",
                          on_rwy_warn_repeat_tooltip, "sec", 1, 10, 0, 1000);
                INPUT_INT(on_rwy_warn_max_n, "Runway extended holding maximum",
                          on_rwy_warn_max_n_tooltip, "", 1, 1, 0, 100);

                SLIDER_FLOAT(gpa_limit_mult, "GPA limit multiplier", gpa_limit_mult_tooltip,
                             "%.1f", "", 0, 10, 1);
                SLIDER_FLOAT(gpa_limit_max, "GPA limit maximum", gpa_limit_max_tooltip,
                             "%.1f", "deg", 0, 10, 1);

                INPUT_INT(long_land_lim_abs, "Long landing absolute limit",
                          long_land_lim_abs_tooltip, "m", 10, 100, 0, 10000);

                SLIDER_FLOAT(long_land_lim_fract, "Long landing limit fraction",
                             long_land_lim_fract_tooltip, "%.2f", "", 0, 1, 1);

#if ACF_TYPE == NO_ACF_TYPE
                SLIDER_FLOAT(min_landing_flap, "Minimum landing flaps",
                             min_landing_flap_tooltip, "%.2f", "", 0, 1, 1);
                SLIDER_FLOAT(min_takeoff_flap, "Minimum takeoff flaps",
                             min_takeoff_flap_tooltip, "%.2f", "", 0, 1, 1);
                SLIDER_FLOAT(max_takeoff_flap, "Maximum takeoff flaps",
                             max_takeoff_flap_tooltip, "%.2f", "", 0, 1, 1);
#endif /* ACF_TYPE == NO_ACF_TYPE */

                INPUT_INT(nd_alert_timeout, "Visual alert timeout",
                          nd_alert_timeout_tooltip, "sec", 1, 1, 0, 1000);
                COMBO_LIST(nd_alert_filter_list, nd_alert_filter, "Visual alert filter", nd_alert_filter_tooltip);
                INPUT_TEXT(nd_alert_overlay_font, "Visual alert overlay font",
                           nd_alert_overlay_font_tooltip);
                INPUT_INT(nd_alert_overlay_font_size, "Visual alert overlay font size",
                          nd_alert_overlay_font_size_tooltip, "", 1, 1, 0, 100);

                ImGui::EndTable();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // End the tab bar
        ImGui::EndTabBar();
    }
    ImGui::Text(" ");

    if (ImGui::BeginTable("##footer", 3, ImGuiTableFlags_SizingStretchSame,
                          ImVec2(tableWidth, 0)))
    {
        const char *button_title;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        button_title = (config.cfg_type == CONFIG_TARGET_LIVERY) ? save_airline : save_airline + 2;
        if (ImGui::Button(button_title, ImVec2(BUTTON_WIDTH, 0)))
        {
            save_config(&config, CONFIG_TARGET_LIVERY);
            LoadConfig();
        }
        Tooltip(save_liv_tooltip);
        ImGui::TableNextColumn();
        CenterText(" ");
        ImGui::TableNextColumn();
        if (ImGui::Button(reset_airline, ImVec2(BUTTON_WIDTH, 0)))
        {
            reset_config(CONFIG_TARGET_LIVERY);
            LoadConfig();
        }
        Tooltip(reset_liv_tooltip);

#ifdef XRAAS_IS_EMBEDDED
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(config.cfg_type != CONFIG_TARGET_GLOBAL);
        if (ImGui::Button("SAVE configuration", ImVec2(BUTTON_WIDTH, 0)))
        {
            save_config(&config, CONFIG_TARGET_GLOBAL);
            LoadConfig();
        }
        Tooltip(save_acf_tooltip);
        ImGui::TableNextColumn();
        CenterText(" ");
        ImGui::TableNextColumn();
        if (ImGui::Button("RESET configuration", ImVec2(BUTTON_WIDTH, 0)))
        {
            reset_config(CONFIG_TARGET_GLOBAL);
            LoadConfig();
        }
        ImGui::EndDisabled();
        Tooltip(reset_acf_tooltip);
#else  /* !XRAAS_IS_EMBEDDED */
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        button_title = (config.cfg_type == CONFIG_TARGET_AIRCRAFT) ? save_aircraft : save_aircraft + 2;
        ImGui::BeginDisabled(config.cfg_type == CONFIG_TARGET_LIVERY);
        if (ImGui::Button(button_title, ImVec2(BUTTON_WIDTH, 0)))
        {
            save_config(&config, CONFIG_TARGET_AIRCRAFT);
            LoadConfig();
        }
        Tooltip(save_acf_tooltip);
        ImGui::TableNextColumn();
        CenterText(" ");
        ImGui::TableNextColumn();
        if (ImGui::Button(reset_aircraft, ImVec2(BUTTON_WIDTH, 0)))
        {
            reset_config(CONFIG_TARGET_AIRCRAFT);
            LoadConfig();
        }
        ImGui::EndDisabled();
        Tooltip(reset_acf_tooltip);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        button_title = (config.cfg_type == CONFIG_TARGET_GLOBAL) ? save_global : save_global + 2;
        ImGui::BeginDisabled(config.cfg_type != CONFIG_TARGET_GLOBAL);
        if (ImGui::Button(button_title, ImVec2(BUTTON_WIDTH, 0)))
        {
            save_config(&config, CONFIG_TARGET_GLOBAL);
            LoadConfig();
        }
        Tooltip(save_glob_tooltip);
        ImGui::TableNextColumn();
        CenterText(" ");
        ImGui::TableNextColumn();
        if (ImGui::Button(reset_global, ImVec2(BUTTON_WIDTH, 0)))
        {
            reset_config(CONFIG_TARGET_GLOBAL);
            LoadConfig();
        }
        ImGui::EndDisabled();
        Tooltip(reset_glob_tooltip);
#endif /* !XRAAS_IS_EMBEDDED */

        ImGui::EndTable();
    }
    CenterText(" ");
    CenterText(COPYRIGHT1);
    CenterText(COPYRIGHT2);
    CenterText(TOOLTIP_HINT);

    CenterText(" ");
    if (config_status != NULL)
    {
        if (config_status_onerror)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, SUCCESS_COLOR);
        }
        CenterText(config_status);
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar();
}

SettingsWindow *setup_window = nullptr;

void destroy_setup_window(void)
{

    if (setup_window != nullptr)
    {
        delete setup_window;
        setup_window = nullptr;
    }
}

void create_setup_window()
{

    if (!imgui_initied)
    {
        XPImgWindowInit();
        imgui_initied = B_TRUE;
    }
    if (setup_window == nullptr)
    {
        setup_window = new SettingsWindow();
    }
}

static char *config_target2filename(conf_target_t target)
{
    switch (target)
    {
    case CONFIG_TARGET_LIVERY:
        return (mkpathname(xraas_cfg_liv_fullpath, NULL));
    case CONFIG_TARGET_AIRCRAFT:
        return (mkpathname(xraas_cfg_acf_fullpath, NULL));
    case CONFIG_TARGET_GLOBAL:
        return (mkpathname(xraas_prefsdir, "X-RAAS.cfg", NULL));
    default:
        VERIFY(0);
    }
}

static void save_config(xraas_state_config_t *state_config, conf_target_t target)
{
    char *config;
    char *filename = config_target2filename(target);

    if (!strlen(filename))
    {
        logMsg("Saving the configuration for the default livery is not supported ");
        config_status_onerror = B_TRUE;
        config_status = save_airline_not_supported;
        free(filename);
        return;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        logMsg("Error writing configuration file %s: %s", filename,
               strerror(errno));
        config_status_onerror = B_TRUE;
        config_status = error_writing_cfg;
        free(filename);
        return;
    }

    config = gen_config(state_config);
    fputs(config, fp);
    fclose(fp);
    free(config);
    free(filename);

    xraas_fini();
    xraas_init();
    gui_update();

    config_status_onerror = B_FALSE;

    switch (target)
    {
    case CONFIG_TARGET_LIVERY:
        config_status = saved_airline_success;
        break;
    case CONFIG_TARGET_AIRCRAFT:
        config_status = saved_aircraft_success;
        break;
    case CONFIG_TARGET_GLOBAL:
        config_status = saved_global_success;
        break;
    }
}

static void reset_config(conf_target_t target)
{
    char *filename = config_target2filename(target);

    if (remove_file(filename, B_TRUE))
    {
        xraas_fini();
        xraas_init();
        gui_update();

        config_status_onerror = B_FALSE;
        switch (target)
        {
        case CONFIG_TARGET_LIVERY:
            config_status = reset_airline_success;
            break;
        case CONFIG_TARGET_AIRCRAFT:
            config_status = reset_aircraft_success;
            break;
        case CONFIG_TARGET_GLOBAL:
            config_status = reset_global_success;
            break;
        default:
            VERIFY(0);
        }
    }
    else
    {
        config_status_onerror = B_TRUE;
        config_status = error_reset_cfg;
    }
    free(filename);
}

static void menu_cb(void *menu, void *item)
{
    // #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    //	int cmd = (int)item;
    int cmd = (int)(intptr_t)item; // WARNING: Potential data loss
                                   // #pragma GCC diagnostic pop

    UNUSED(menu);
    switch (cmd)
    {
    case CONFIG_GUI_CMD:
        config_status = NULL;
        config_status_onerror = B_FALSE;
        setup_window->SetVisible(B_TRUE);
        break;
    case DBG_GUI_TOGGLE_CMD:
        ASSERT(xraas_inited);
        if (!dbg_gui_inited)
            dbg_gui_init();
        else
            dbg_gui_fini();
        XPLMCheckMenuItem(root_menu, dbg_gui_menu_item,
                          dbg_gui_inited ? xplm_Menu_Checked : xplm_Menu_Unchecked);
        break;
    case RECREATE_CACHE_CMD:
    {
        bool_t exists, isdir;
        char *cachedir =
            mkpathname(xraas_xpdir, "Output", "caches", "X-RAAS.cache", NULL);
        exists = file_exists(cachedir, &isdir);
        if (exists && ((isdir && !remove_directory(cachedir)) ||
                       (!isdir && !remove_file(cachedir, B_FALSE))))
        {
            log_init_msg(B_TRUE, INIT_ERR_MSG_TIMEOUT, NULL, NULL,
                         "Cannot remove existing data cache. See Log.txt "
                         "for more information.");
            free(cachedir);
            break;
        }
        free(cachedir);
        /*FALLTHROUGH*/
        __attribute__((fallthrough));
    }
    case RAAS_RESET_CMD:
        xraas_fini();
        xraas_init();
        gui_update();
        break;
    }
}

static void create_menu(void)
{
    plugins_menu_item =
        XPLMAppendMenuItem(XPLMFindPluginsMenu(), XRAAS_MENU_NAME, NULL, 1);
    root_menu = XPLMCreateMenu(XRAAS_MENU_NAME, XPLMFindPluginsMenu(),
                               plugins_menu_item, menu_cb, NULL);
    XPLMAppendMenuItem(root_menu, CONFIG_GUI_CMD_NAME, (void *)CONFIG_GUI_CMD, 1);
    dbg_gui_menu_item = XPLMAppendMenuItem(root_menu, DBG_GUI_TOGGLE_CMD_NAME,
                                           (void *)DBG_GUI_TOGGLE_CMD, 1);
    XPLMAppendMenuItem(root_menu, RAAS_RESET_CMD_NAME, (void *)RAAS_RESET_CMD, 1);
    XPLMAppendMenuItem(root_menu, RECREATE_CACHE_CMD_NAME,
                       (void *)RECREATE_CACHE_CMD, 1);
}

static void destroy_menu(void)
{
    XPLMDestroyMenu(root_menu);
    XPLMRemoveMenuItem(XPLMFindPluginsMenu(), plugins_menu_item);
}

static int command_cb(XPLMCommandRef cmd, XPLMCommandPhase phase,
                      void *refcon)
{
    UNUSED(cmd);
    if (phase == xplm_CommandBegin)
    {
        if (cmd == toggle_cfg_gui_cmd)
        {
            if (!setup_window->GetVisible())
            {
                setup_window->SetVisible(true);
                config_status = NULL;
                config_status_onerror = B_FALSE;
            }
            else
                setup_window->SetVisible(false);
        }
        else
            menu_cb(NULL, refcon);
    }
    return (1);
}

static void register_commands(void)
{
    toggle_cfg_gui_cmd = XPLMCreateCommand(
        "xraas/toggle_config_gui", "Shows/hides the X-RAAS configuration window");
    XPLMRegisterCommandHandler(toggle_cfg_gui_cmd, command_cb, 0,
                               (void *)CONFIG_GUI_CMD);
    toggle_dbg_gui_cmd = XPLMCreateCommand("xraas/toggle_debug_gui",
                                           "Toggles the X-RAAS debug overlay");
    XPLMRegisterCommandHandler(toggle_dbg_gui_cmd, command_cb, 0,
                               (void *)DBG_GUI_TOGGLE_CMD);
    recreate_cache_cmd = XPLMCreateCommand(
        "xraas/recreate_data_cache",
        "Tells X-RAAS to recreate its data cache from the current AIRAC "
        "database");
    XPLMRegisterCommandHandler(recreate_cache_cmd, command_cb, 0,
                               (void *)RECREATE_CACHE_CMD);
    raas_reset_cmd = XPLMCreateCommand(
        "xraas/reset", "Resets X-RAAS as if it had been power-cycled");
    XPLMRegisterCommandHandler(raas_reset_cmd, command_cb, 0,
                               (void *)RAAS_RESET_CMD);
}

static void unregister_commands(void)
{
    XPLMUnregisterCommandHandler(toggle_cfg_gui_cmd, command_cb, 0,
                                 (void *)CONFIG_GUI_CMD);
    XPLMUnregisterCommandHandler(toggle_dbg_gui_cmd, command_cb, 0,
                                 (void *)DBG_GUI_TOGGLE_CMD);
    XPLMUnregisterCommandHandler(recreate_cache_cmd, command_cb, 0,
                                 (void *)RECREATE_CACHE_CMD);
    XPLMUnregisterCommandHandler(raas_reset_cmd, command_cb, 0,
                                 (void *)RAAS_RESET_CMD);
}

void gui_init(void)
{
    if (gui_inited)
        return;
    create_menu();
    create_setup_window();
    register_commands();
    gui_inited = B_TRUE;
}

void gui_fini(void)
{
    if (!gui_inited)
        return;
    destroy_menu();
    destroy_setup_window();
    unregister_commands();
    gui_inited = B_FALSE;
}

void gui_update(void)
{
    if (!gui_inited)
        return;
    XPLMEnableMenuItem(root_menu, dbg_gui_menu_item, xraas_inited);
    XPLMCheckMenuItem(root_menu, dbg_gui_menu_item,
                      xraas_state->config.debug_graphical ? xplm_Menu_Checked
                                                          : xplm_Menu_Unchecked);
}