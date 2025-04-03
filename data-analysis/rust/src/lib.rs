#![allow(non_camel_case_types)]

#[cxx::bridge] 
mod ffi {
    extern "Rust" {
        fn return_five() -> u32;
    }
}

fn return_five() -> u32 {
    5
}