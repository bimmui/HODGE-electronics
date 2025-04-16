pub struct BitRep {
    bytes: Vec<u8>,
    bits_free: u8,
}

impl BitRep {
    pub fn new() -> BitRep {
        BitRep { bytes: Vec::new(), bits_free: 0 }
    }
    pub fn write_zero(&mut self) {
        if self.bits_free > 0 {
            self.bits_free -= 1;
        }
        else {
            self.bytes.push(0);
            self.bits_free = 7;
        }
    }

    pub fn write_one(&mut self) {
        if self.bits_free > 0 {
            self.bits_free -= 1;
            self.bytes.last_mut().map(|byte| *byte |= 1 << self.bits_free);
        }
        else {
            self.bytes.push(0b1000000);
            self.bits_free = 7;
        }
    }

    pub fn write_bit(&mut self, bit: u8) {
        if self.bits_free > 0 {
            self.bits_free -= 1;
            self.bytes.last_mut().map(|byte| *byte |= bit << self.bits_free);
        }
        else {
            self.bytes.push(0b1000000);
            self.bits_free = 7;
        }
    }
}