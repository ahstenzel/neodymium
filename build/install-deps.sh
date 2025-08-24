#!/usr/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd ${SCRIPT_DIR}

# Check for sudo
if [[ $(id -u) -ne 0 ]]; then
	echo "Please run with root permissions."
	exit 1
fi

# Install packages
apt install make build-essential devscripts debhelper git libncurses-dev dialog

# Get repositories
cd ..
mkdir -p vendor
cd vendor
git clone https://github.com/ahstenzel/vex.git
cd ${SCRIPT_DIR}