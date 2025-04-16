use lazy_static::lazy_static;
use macros::export_concrete;
use paste::paste;

mod bitrep;
mod efloat;
mod huffman;

#[cxx::bridge(namespace = "ffi  ")] 
mod ffi {

    extern "Rust" {
        
    }
}

pub trait Compress {
    fn return_value(&self) -> u32;
    fn make() -> Self;
}

struct Compressor {
    inner: ()//Thinger,
}

impl Compressor {
    fn return_value(&self) -> u32 {
        5//self.inner.return_value()
    }
    fn make() -> Self {
        Self { inner: ()}//Thinger::make() }
    }
}
fn make() -> Box<Compressor> {
    Box::new(Compressor::make())
}

#[cxx::bridge(namespace = "compress")]
mod CompressorModule {
    extern "Rust" {
        type Compressor;
        fn return_value(&self) -> u32;
        fn make() -> Box<Compressor>;
    }
}
