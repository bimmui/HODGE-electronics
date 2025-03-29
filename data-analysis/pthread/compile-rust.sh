#!/bin/bash

cd $1
cargo +esp build --release --verbose;
cp target/xtensa-esp32-espidf/release/libcompress.a ../pthread/main; 
cp target/xtensa-esp32-espidf/cxxbridge/compress/src/* ../pthread/main