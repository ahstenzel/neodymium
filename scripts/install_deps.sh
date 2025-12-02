#!/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd ${SCRIPT_DIR} > /dev/null

# Install packages
sudo apt update && sudo apt install build-essential ninja-build make cmake gcc \
	devscripts debhelper git libncurses6 libncursesw6 libncurses-dev ncurses-doc \
	binutils-arm-linux-gnueabihf gcc-arm-linux-gnueabihf

# Get repositories
cd ..
mkdir -p vendor
cd vendor
if [ ! -d "vex" ]; then
	git clone https://github.com/ahstenzel/vex.git
fi
if [ ! -d "komihash" ]; then
	git clone https://github.com/avaneev/komihash.git
fi
popd > /dev/null