fn main() {
    let _build = cxx_build::bridge("src/lib.rs");
    println!("cargo:rustc-link-arg-bins=-Tlinkall.x");
    println!("cargo:rerun-if-changed=src/lib.rs");
}