#!/bin/bash

if [ ${1,,} == "help" ]; then
        echo "PREBUILD";
        echo ""
        echo "Run this script before doing development on the C++ library";
        echo "This script builds the Rust library that is a dependency for this project";
        echo "The path to the library can be passed as the first argument of this script";
        echo "Otherwise, the script will search the enviroment variable ${ENV_VAR}";
        echo "If that fails, the default path used is ${DEFAULT}";

        echo "This script builds a static library, and C++/header files for use in the project";
else 
    # Environment variable to find rust library
    ENV_VAR="RUST_LIB_DIR";
    # Default path (Change if you use this other places!)
    DEFAULT="../rust-lib/"
    CWD=$(pwd)

    # Did they pass an argument?
    if [ -n "$1" ]; then
        # Move to directory
        cd $1;
    else 
        # Use the enviroment variable directory, unless it's empty
        # In which case, use the default
        cd ${!ENV_VAR:-${DEFAULT}};
    fi

    # Build library with the esp toolchain
    cargo +esp build --release --verbose;
    # Copy built files
    cp target/xtensa-esp32-espidf/release/libcompress.a ${CWD}/main; 
    cp target/xtensa-esp32-espidf/cxxbridge/compress/src/* ${CWD}/main;
fi