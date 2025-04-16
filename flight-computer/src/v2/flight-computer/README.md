# Build Insructions

- Navigate to `../rust-lib` and run commands
    - Run `cargo +esp build --release --verbose` to build the libaries
    - Run `cp target/xtensa-esp32-espidf/release/libcompress.a ../pthread/main`
    - Run `cp target/xtensa-esp32-espidf/cxxbridge/compress/src/* ../pthread/main`
- Come back here
    - Export esp-idf if you haven't done so already
    - Run `idf.py build`

Done!