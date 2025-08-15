#!/usr/bin/bash
BUILD_CONF=none
BUILD_PKG=none
USE_DIALOG=true
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd ${SCRIPT_DIR}

function show_help
{
	echo " -b) Build configuration [debug(default) | release]"
	echo " -p) Build package instead of a standalone binary"
	echo " -d) Do not use dialogs"
	echo " -h) Show this help dialogue"
}

# Check for sudo
if [[ $(id -u) -ne 0 ]]; then
	echo "Please run with root permissions."
	exit 1
fi

# Get user options
while test $# -gt 0; do
	case "$1" in
		-b)
			shift
			BUILD_CONF=$1
			shift
			;;
		-p)
			shift
			BUILD_PKG=true
			;;
		-d)
			shift
			USE_DIALOG=false
			;;
		-h|*)
			shift
			show_help
			exit 0
			;;
	esac
done

# Check for dialog tool
if [ $USE_DIALOG = true ]; then
	which dialog >/dev/null
	if [ $? -ne 0 ]; then USE_DIALOG=false; fi
fi

# Define the dialog exit status codes
: ${DIALOG_OK=0}
: ${DIALOG_CANCEL=1}
: ${DIALOG_HELP=2}
: ${DIALOG_EXTRA=3}
: ${DIALOG_ITEM_HELP=4}
: ${DIALOG_ESC=255}

# Create a temporary file and make sure it goes away when we're dome
temp_file=$(tempfile 2>/dev/null) || temp_file=/tmp/test$$
trap "rm -f $temp_file" 0 1 2 5 15

# Ask user for build configuration
while [ $BUILD_CONF = none ]; do
	if [ $USE_DIALOG = true ]; then
		dialog \
			--backtitle "neodymium" \
			--title "Build configuration" \
			--clear \
			--radiolist "Select build configuration:" 10 32 2 \
			1 "Debug" ON \
			2 "Release" OFF \
			2> $temp_file
		ret=$?

		names=(none debug release)
		case $ret in
			$DIALOG_OK)
				BUILD_CONF=${names[$(cat $temp_file)]};;
			$DIALOG_CANCEL)
				clear
				exit 0;;
			$DIALOG_ESC|*)
				exit 0;;
		esac
		> $temp_file
		clear
	else
		read -p $'Build for (\e[32mD\e[0m)ebug / (\e[32mR\e[0m)elease [D]:' conf_var
		conf_var=${conf_var:-D}
		if [[ ${conf_var} = D || ${conf_var} = d ]]; then
			BUILD_CONF=debug
		elif [[ ${conf_var} = R || ${conf_var} = r ]]; then
			BUILD_CONF=release
		fi
	fi
done

# Ask user for build format
while [ $BUILD_PKG = none ]; do
	if [ $USE_DIALOG = true ]; then
		dialog \
			--backtitle "neodymium" \
			--title "Build format" \
			--clear \
			--radiolist "Select build format:" 10 32 2 \
			1 "Binary" ON \
			2 "Package" OFF \
			2> $temp_file
		ret=$?

		names=(none debug release)
		case $ret in
			$DIALOG_OK)
				if [ $(cat $temp_file) = 2 ]; then 
					BUILD_PKG=true
				else
					BUILD_PKG=false
				fi
				;;
			$DIALOG_CANCEL)
				clear
				exit 0;;
			$DIALOG_ESC|*)
				exit 0;;
		esac
		> $temp_file
		clear
	else
		read -p $'Build (\e[32mB\e[0m)inary / (\e[32mP\e[0m)ackage [B]:' conf_var
		conf_var=${conf_var:-B}
		if [[ ${conf_var} = B || ${conf_var} = b ]]; then
			BUILD_PKG=false
		elif [[ ${conf_var} = P || ${conf_var} = p ]]; then
			BUILD_PKG=true
		fi
	fi
done

# Build app
cd ../src
VER=$(cat VERSION)

function build_pkg_debug
{
	echo "[*] Building app..."
	debuild -us -uc
	make clean
	cd ../
	if compgen -G "./neodymium-*.*" > /dev/null; then 
		echo "[*] Build successful!"
		mkdir -p ./bin/neo/debug/v${VER}
		mv ./neodymium-*.* ./bin/neo/debug/v${VER}
		mv ./neodymium_*.* ./bin/neo/debug/v${VER}
	else
		echo "[X] Build failed..."
	fi
}

function build_pkg_release
{
	echo "[*] Building app..."
	debuild -us -uc
	make clean
	cd ../
	if compgen -G "./neodymium-*.*" > /dev/null; then 
		echo "[*] Build successful!"
		mkdir -p ./bin/neo/release/v${VER}
		mv ./neodymium-*.* ./bin/neo/release/v${VER}
		mv ./neodymium_*.* ./bin/neo/release/v${VER}
	else
		echo "[X] Build failed..."
	fi
}

function build_bin_debug
{
	echo "[*] Building app..."
	make debug
	if [[ -f "./neo" ]]; then 
		echo "[*] Build successful!"
		mkdir -p ../bin/neo/debug
		mv ./neo ../bin/neo/debug
	else
		echo "[X] Build failed..."
	fi
}

function build_bin_release
{
	echo "[*] Building app..."
	make
	if [[ -f "./neo" ]]; then 
		echo "[*] Build successful!"
		mkdir -p ../bin/neo/release
		mv ./neo ../bin/neo/release
	else
		echo "[X] Build failed..."
	fi
}

if [ ${BUILD_CONF} = release ]; then
	if [ ${BUILD_PKG} = true ]; then
		if [ $USE_DIALOG = true ]; then
			build_pkg_release | fold -w 96 | dialog --programbox 30 100
		else
			build_pkg_release
		fi
	else
		if [ $USE_DIALOG = true ]; then
			build_bin_release | fold -w 96 | dialog --programbox 30 100
		else
			build_bin_release
		fi
	fi
else
	if [ ${BUILD_PKG} = true ]; then
		if [ $USE_DIALOG = true ]; then
			build_pkg_debug | fold -w 96 | dialog --programbox 30 100
		else
			build_pkg_debug
		fi
	else
		if [ $USE_DIALOG = true ]; then
			build_bin_debug | fold -w 96 | dialog --programbox 30 100
		else
			build_bin_debug
		fi
	fi
fi

if [ $USE_DIALOG = true ]; then clear; fi