include(FetchContent)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5 # Optionally specify a commit hash, version tag or branch here
)
# Set any global configuration variables such as `Rust_TOOLCHAIN` before this line!
FetchContent_MakeAvailable(Corrosion)

# Import targets defined in a package or workspace manifest `Cargo.toml` file
corrosion_import_crate(MANIFEST_PATH ../compress-rs/Cargo.toml CRATES compress-rs)
corrosion_add_cxxbridge(cxxbridge-cpp CRATE compress_rs FILES lib.rs)

ADD_CUSTOM_COMMAND(DEPENDS cxxbridge-cpp
    OUTPUT compress_rs.h
    COMMAND cp corrosion_generated/cxxbridge/cxxbridge-cpp/include/cxxbridge-cpp/lib.h compress_rs.h
)

add_executable(main main.cpp compress_rs.h)
target_link_libraries(main PUBLIC cxxbridge-cpp)
