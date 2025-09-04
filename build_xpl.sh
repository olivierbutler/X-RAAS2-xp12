#!/bin/bash

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

# Copyright 2022 Saso Kiselkov. All rights reserved.

# Invoke this script to build X-RAAS2 for both Windows and Linux, 64-bit
# flavors. Install the necessary mingw cross-compile utilities first.
# On Linux also do "apt install libopenal-dev:x86_64".

# Invoke this script to build X-RAAS2 for Mac 64-bit flavor.
# Make sure also to install macports's qt5-qtcreator package and
# have `qmake' available in your $PATH.

#########################################################################
# Set here the correct paths
# docker's interval path /xpl_dev is mapped one level up the current folder
# the libacfutils folder is expected to be at the same level of this projet, if not
# modify docket-compose.yml accordingly. 

# Host folders         | Internal docker folders
# ---------------------|---------------------
# ../projets/          | /xpl_dev/
# ├── libacfutils/     | ├── libacfutils/
# └── X-RAAS2-xp12/    | └── X-RAAS2-xp12/

PROJECT_PATH='X-RAAS2-xp12'
ACFLIB_PATH='libacfutils' 
#########################################################################

embed=no
embed_suffix=''
acf_type=0
full=''
nodoc=no

while getopts "fdeht:" opt; do
	case "$opt" in
	f)
		if [[ $(uname) = "Darwin" ]]; then
			full=1
		else 
			echo "Option ignored when compiling from linux host"
		fi
	    ;;	
	d)
	    nodoc=yes
	    ;;	
	e)
		embed=yes
		embed_suffix="_embed"
		;;
	t) 
		case "$OPTARG" in
			FF_A320)
				acf_type=1
				embed_suffix=""
				;;
			*)
				echo "Invalid aircraft type." \
				    "Try \"$0 -h\" for help" >&2
				exit 1
				;;
		esac
		embed=yes
		;;
	h)
		cat << EOF
Usage: $0 [-fdeh] [-t <acf_type>]
Options:
	-f : (macOS-only) Cross-compile to linux and window
		using the docker image provided by docker-compose.yml.
		See README-docker.md for more information.
    -d : Continue even if the documentation build failed. Useful if you don't
       have pdfTeX installed and just want to test build.
  	-e : Build an embeddable version of X-RAAS. Embeddable versions auto-inhibit
       when a globally installed version of X-RAAS is detected to prevent
       duplicated annunciations. Embeddable versions also relocate their config
       files and data cache so that they don't interfere with a global version.
  	-t <acf_type>: Build an embeddable version specifically geared towards a
       particular aircraft model. This implies "-e". Currently supported
       aircraft models are:
       *) FF_A320 : The FlightFactor Airbus A320-200. The resulting plugin
                    will be named simply "X-RAAS2".
  	-h : Show this help screen.
EOF
		exit
		;;
	*)
		echo "Try \"$0 -h\" for help." >&2
		exit 1
		;;
	esac
done

if [[ "$#" -ge $OPTIND ]]; then
	echo "Too many arguments. Try \"$0 -h\" for help" >&2
	exit 1
fi

LIBACFUTILS="$(qmake -query LIBACFUTILS)"

rm -rf output
mkdir -p output/64
mkdir -p 64
cd output

YEAR=$(date +%Y)


case "$(uname)" in
Linux)
	set -e
	rm -f ../64/win.xpl
	qmake -set CROSS_COMPILE x86_64-w64-mingw32- && \
		qmake -set XRAAS_EMBED $embed && \
		qmake -set ACF_TYPE "$acf_type" && \
		qmake -set CURRENT_YEAR "$YEAR" && \
		qmake -set NOERRORS 1 && \
		qmake -spec win32-g++ ../qmake.pro && \
		make -j $NCPUS && \
		mv debug/win.xpl1.dll ../64/win.xpl
		strip ../64/win.xpl
	if [ $? != 0 ] ; then
	exit
	fi

	make distclean > /dev/null
	#mkdir -p output/64
	rm -f ../64/lin.xpl
	qmake -set XRAAS_EMBED $embed && \
	qmake -set ACF_TYPE "$acf_type" && \
	qmake -set CURRENT_YEAR "$YEAR" && \
	qmake -set NOERRORS 1 && \
	qmake -spec linux-g++-64 ../qmake.pro && \
	make -j $NCPUS && \
	mv liblin.xpl.so ../64/lin.xpl
	strip ../64/lin.xpl
	if [ $? != 0 ] ; then
	exit
	fi
	set +e
	;;
Darwin)
	# We'll try to build on N+1 CPUs we have available. The extra +1 is to allow
	# for one make instance to be blocking on disk.
	NCPUS=$(( $(sysctl -n hw.ncpu) + 1 ))
	rm -f ../64/mac.xpl
	qmake -set XRAAS_EMBED $embed && \
	qmake -set ACF_TYPE "$acf_type" && \
	qmake -set CURRENT_YEAR "$YEAR" && \
	qmake -set NOERRORS 1 && \
	qmake -spec macx-clang ../qmake.pro && \
	make -j $NCPUS && \
	mv libmac.xpl.dylib ../64/mac.xpl
	strip -x ../64/mac.xpl
	if [ $? != 0 ] ; then
	exit
	fi
	;;
*)
	echo "Unsupported platform" >&2
	exit 1
	;;
esac

cd ..
if [ "$full" ]; then
	docker compose run --rm win-lin-build bash -c 'qmake -set LIBACFUTILS /xpl_dev/'${ACFLIB_PATH}' && cd '${PROJECT_PATH}' && ./build_xpl.sh "$@"' -- "$@"
fi