# CDDL HEADER START
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#
# CDDL HEADER END

# Copyright 2016 Saso Kiselkov. All rights reserved.

# Shared library without any Qt functionality
TEMPLATE = lib
QT -= gui core

QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64

CONFIG += warn_on plugin debug
CONFIG -= thread exceptions qt rtti

VERSION = 1.0.0

debug = $$[DEBUG]
dll = $$[ACFUTILS_DLL]
noerrors = $$[NOERRORS]

INCLUDEPATH += $$[LIBACFUTILS]/SDK/CHeaders/XPLM
INCLUDEPATH += $$[LIBACFUTILS]/SDK/CHeaders/Widgets
INCLUDEPATH += $$[LIBACFUTILS]/SDK

# Always just use the libacfutils OpenAL headers for predictability.
# The ABI is X-Plane-internal and stable anyway.
INCLUDEPATH += $$[LIBACFUTILS]/OpenAL/include
INCLUDEPATH += $$[LIBACFUTILS]/src

# Aircraft-type-specific APIs
INCLUDEPATH += ../acf_apis



QMAKE_CFLAGS += -std=c11 -g -W -Wall -Wextra  -fvisibility=hidden
contains(noerrors, 0) {
	QMAKE_CFLAGS += -Werror
}
QMAKE_CFLAGS += -Wunused-result

!macx {
	QMAKE_CFLAGS += -Wno-format-truncation -Wno-cast-function-type
	QMAKE_CFLAGS += -Wno-stringop-overflow -Wno-missing-field-initializers
}


# _GNU_SOURCE needed on Linux for getline()
# DEBUG - used by our ASSERT macro
# _FILE_OFFSET_BITS=64 to get 64-bit ftell and fseek on 32-bit platforms.
# _USE_MATH_DEFINES - sometimes helps getting M_PI defined from system headers
DEFINES += _GNU_SOURCE DEBUG _FILE_OFFSET_BITS=64 _USE_MATH_DEFINES

# Latest X-Plane APIs. No legacy support needed.
DEFINES += XPLM200 XPLM210 XPLM300 XPLM301
DEFINES += \
    XRAAS2_BUILD_VERSION=\'\"$$system("git describe --abbrev=0 --tags")\"\'

# Aircraft-specific defines
DEFINES += ACF_TYPE=$$[ACF_TYPE]

XRAAS_EMBED=$$[XRAAS_EMBED]
contains(XRAAS_EMBED, yes) {
	DEFINES += XRAAS_IS_EMBEDDED
}


win32 {
	DEFINES += APL=0 IBM=1 LIN=0 _WIN32_WINNT=0x0600
	TARGET = win.xpl
	INCLUDEPATH += /usr/include/GL
	QMAKE_DEL_FILE = rm -f
	LIBS += -static-libgcc

	QMAKE_CFLAGS += $$system("$$[LIBACFUTILS]/pkg-config-deps win-64 --static-openal --cflags")
	LIBS += -L$$[LIBACFUTILS]/qmake/win64 -lacfutils
	LIBS += $$system("$$[LIBACFUTILS]/pkg-config-deps win-64 --static-openal  --libs")

	LIBS += -L$$[LIBACFUTILS]/SDK/Libraries/Win -lXPLM_64
	LIBS += -L$$[LIBACFUTILS]/SDK/Libraries/Win -lXPWidgets_64
	LIBS += -L/usr/x86_64-w64-mingw32 -lglu32 -lopengl32
	LIBS += -ldbghelp
}

linux-g++-64 {
	DEFINES += APL=0 IBM=0 LIN=1
	TARGET = lin.xpl
	QMAKE_CFLAGS += -fno-stack-protector
	QMAKE_CFLAGS += $$system("$$[LIBACFUTILS]/pkg-config-deps linux-64 \
	    --static-openal --cflags")
	LIBS += -L $$[LIBACFUTILS]/qmake/lin64 -lacfutils
	LIBS += $$system("$$[LIBACFUTILS]/pkg-config-deps linux-64  --static-openal --libs")
}



macx {
	DEFINES += APL=1 IBM=0 LIN=0 TARGET_OS_MAC=1
	DEFINES += LACF_GLEW_USE_NATIVE_TLS=0
	TARGET = mac.xpl
	INCLUDEPATH += ../OpenAL/include
	LIBS += -F$$[LIBACFUTILS]/SDK/Libraries/Mac
	LIBS += -framework XPLM -framework XPWidgets
	LIBS += -framework OpenGL -framework OpenAL
	###LIBS += -framework OpenGL -framework AudioToolbox
	LIBS += -framework CoreAudio -framework AudioUnit
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.13
}

macx-clang {
    DEFINES += TARGET_OS_MAC=1
	QMAKE_CFLAGS += $$system("$$[LIBACFUTILS]/pkg-config-deps mac-64 \
	   --static-openal --cflags")
	LIBS += $$system("$$[LIBACFUTILS]/pkg-config-deps mac-64 --static-openal --libs")
}


HEADERS += ../src/*.h ../api/c/XRAAS_ND_msg_decode.h
SOURCES += ../src/*.c ../api/c/XRAAS_ND_msg_decode.c