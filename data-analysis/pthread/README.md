# Build Insructions

- Navigate to `../rust-lib` and run commands
    - Run `cargo +esp build --release --verbose` to build the libaries
    - Run `cbindgen > compress.h` to create the c bindings and export to file
    - Run `cp target/xtensa-esp32-espidf/release/libcompress.a ../pthread/main`
    - Run `cp compress.h ../pthread/main`
- Come back here
    - Export esp-idf if you haven't done so already
    - Run `idf.py build`

Done!