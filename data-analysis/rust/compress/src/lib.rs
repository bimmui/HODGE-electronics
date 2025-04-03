use ffi::set_pin;
use lazy_static::lazy_static;
use macros::export_concrete;

mod efloat;
//mod huffman;

#[cxx::bridge] 
mod ffi {

    extern "Rust" {
        fn return_five() -> u32;
        fn toggle_loop();
    }

    unsafe extern  "C++" {
        include!("gpio.h");

        fn set_pin(pin: i32, level: i32);
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

const PIN: i32 = 13;

fn toggle_loop() {
    let mut level = 0;
    loop {
        set_pin(PIN, level);
        level = level ^ 1;
        std::thread::sleep(std::time::Duration::from_secs(1));
    }
}