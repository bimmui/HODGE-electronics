#!/bin/bash

# Print help message
# convert first arg to lowercase
if [ "${1,,}" == "help" ]; then
    echo "ESP build script with Rust linking"
    echo ""
    echo "Run or source this script to build an ESP component and link to a rust library"
    echo "The Rust files are built with prebuild.sh, which should be in this same directory"
    echo "The first argument to this script is interpreted as the path to the ESP-IDF export script"
    echo "If none is provided, the script attempts to find one, starting at $HOME/esp/"
    echo "The second argument is interpreted as an argument to prebuild.sh"
    echo "If you need to provide an argument to prebuild but don't want to pass the export script path,"
    echo "set the environment variable requested by prebuild.sh (you can find it by calling ./prebuild.sh help)"
    echo ""
else 
    # Forward the second argument to the prebuild script
    ./prebuild.sh $2
    if [ -z "$ESP_IDF_VERSION" ]; then 
        if [ -n "$1" ]; then
            source $1
        else 
            chosen=""
            for folder in $HOME/esp/v*; 
                do
                vfile=$(basename "$folder")
                if [[ chosen < "$vfile" ]]; then
                    chosen=$vfile
                fi;
            done
            export_path="$HOME/esp/$chosen/esp-idf/export.sh"
            source $export_path
        fi 
    fi
    idf.py build
fi

