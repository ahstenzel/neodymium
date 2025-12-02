#!/bin/bash
RED='\e[91m'
GREEN='\e[92m'
NC='\e[0m'
BUILD_CONF=none
BUILD_ARCH=none
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd ${SCRIPT_DIR} > /dev/null

function show_help
{
	echo "Usage: $(basename $0) [-b config] [-a arch]"
	echo " -b) Build configuration [debug(default) | release]"
	echo " -a) Build architecture [x64(default) | arm64]"
	echo " -h) Show this help dialogue"
}

# Get user options
while test $# -gt 0; do
	case "$1" in
		-b)
			shift
			BUILD_CONF=$1
			shift
			;;
		-a)
			shift
			BUILD_ARCH=$1
			shift
			;;
		-h|*)
			shift
			show_help
			exit 0
			;;
	esac
done

# Ask user for build configuration
while [ $BUILD_CONF = none ]; do
	read -p $'Build for (\e[32mD\e[0m)ebug / (\e[32mR\e[0m)elease [D]:' conf_var
	conf_var=${conf_var:-D}
	if [[ ${conf_var} = D || ${conf_var} = d ]]; then
		BUILD_CONF=Debug
	elif [[ ${conf_var} = R || ${conf_var} = r ]]; then
		BUILD_CONF=Release
	fi
done

# Ask user for build format
while [ $BUILD_ARCH = none ]; do
	read -p $'Build for (\e[32mx\e[0m)64 / (\e[32ma\e[0m)rm64 [x]:' conf_var
	conf_var=${conf_var:-x}
	if [[ ${conf_var} = X || ${conf_var} = x ]]; then
		BUILD_ARCH=x64
	elif [[ ${conf_var} = A || ${conf_var} = a ]]; then
		BUILD_ARCH=arm64
	fi
done

# Build app
function do_cmake_build
{
	echo -e "[${GREEN}*${NC}] Configuring CMake..."
	cmake --preset $1
	if [[ $? != 0 ]]; then
		echo -e "[${RED}X${NC}] Configuration failed!"
		popd > /dev/null
		exit 1
	fi

	echo -e "[${GREEN}*${NC}] Building app..."
	cmake --build --preset $1 --target install
	if [[ $? != 0 ]]; then
		echo -e "[${RED}X${NC}] Build failed!"
		popd > /dev/null
		exit 1
	fi
	
	echo -e "[${GREEN}*${NC}] Building package..."
	pushd "./build/$1" > /dev/null
	cpack
	if [[ $? != 0 ]]; then
		echo -e "[${RED}X${NC}] Packaging failed!"
		popd > /dev/null
		popd > /dev/null
		exit 1
	fi
	popd > /dev/null
	mv ./bin/*.deb ./bin/$1
}

cd ..
if [[ ${BUILD_CONF} = Debug ]]; then
	if [[ ${BUILD_ARCH} = arm64 ]]; then
		do_cmake_build debug-arm64
	else
		do_cmake_build debug-x64
	fi
else
	if [[ ${BUILD_ARCH} = arm64 ]]; then
		do_cmake_build release-arm64
	else
		do_cmake_build release-x64
	fi
fi
echo -e "[${GREEN}*${NC}] Build successful!"
popd > /dev/null