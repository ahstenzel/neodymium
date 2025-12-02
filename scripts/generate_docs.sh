#!/usr/bin/bash
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd ${SCRIPT_DIR} > /dev/null

cd ..
doxygen Doxyfile
popd > /dev/null