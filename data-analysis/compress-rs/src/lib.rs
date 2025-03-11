pub trait DoThing {
    fn return_value() -> u32;
}

pub struct Thinger {}

impl DoThing for Thinger {
    fn return_value() -> u32 { 6 }
}

#[cxx::bridge] 
mod ffi {
    extern "Rust" {
        fn return_five() -> u32;
    }
}

fn return_five() -> u32 {
    return 5;
}