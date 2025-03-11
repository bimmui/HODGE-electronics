use lazy_static::lazy_static;
use macros::make_answer;

#[cxx::bridge] 
mod ffi {

    extern "Rust" {
        fn return_five() -> u32;
        fn return_value() -> u32;
    }
}

type Compressor = Thinger;

#[make_answer]
pub trait DoThing: Sync {
    fn return_value(&self) -> u32;
    fn make() -> Self where Self: Sized;
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

lazy_static! {
    static ref THING_DOER: Box<dyn DoThing> = Box::new(Thinger {});
}

fn return_value() -> u32 {
    THING_DOER.return_value()
}

fn return_five() -> u32 {
    5
}