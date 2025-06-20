#!/bin/bash

XPLANE_PLUGIN_DIR="$HOME/X-Plane 12/Resources/plugins/X-RAAS2/64"

case "$(uname)" in
Linux)
    echo "copying 64/lin.xpl to ${XPLANE_PLUGIN_DIR}"
	cp -v 64/lin.xpl "${XPLANE_PLUGIN_DIR}"
    echo "copying 64/win.xpl to ${XPLANE_PLUGIN_DIR}"
	cp -v 64/win.xpl "${XPLANE_PLUGIN_DIR}"
	;;
Darwin)
    echo "copying 64/mac.xpl to ${XPLANE_PLUGIN_DIR}"
	cp -v 64/mac.xpl "${XPLANE_PLUGIN_DIR}"
    echo "removing quarantine"
    xattr -dr com.apple.quarantine "${XPLANE_PLUGIN_DIR}"
	;;
*)
	echo "Unsupported platform" >&2
	exit 1
	;;
esac