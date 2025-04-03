use lazy_static::lazy_static;
use macros::export_concrete;

mod efloat;
//mod huffman;

#[cxx::bridge] 
mod ffi {

    extern "Rust" {
        fn return_five() -> u32;
    }
}

#[export_concrete(concrete = Thinger, wrapper = Compressor, module = CompressorModule)]
pub trait DoThing: Sync {
    fn return_value(&self) -> u32;
    fn make() -> Self;
}


struct Thinger {}

impl Thinger {
    pub fn pub_thing() -> u32 {
        17
    }
}

impl DoThing for Thinger {
    fn return_value(&self) -> u32 { 6 }
    fn make() -> Self { Thinger {} }
}

fn return_five() -> u32 {
    5
}