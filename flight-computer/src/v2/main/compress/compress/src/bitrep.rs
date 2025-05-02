#[derive(Clone, PartialEq, Eq)]
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
            self.bytes.push(0b0000000 | bit << 7);
            self.bits_free = 7;
        }
    }

    pub fn push_zero(&self) -> BitRep {
        let mut br = self.clone();
        br.write_zero();
        br
    }

    pub fn push_one(&self) -> BitRep {
        let mut br = self.clone();
        br.write_one();
        br  
    }

    pub fn len(&self) -> usize {
        return 8 * self.bytes.len() - self.bits_free as usize;
    }

    pub fn get_bit(&self, bit_idx: usize) -> Option<u8> {
        if bit_idx >= self.len() { return None; }
        let byte = bit_idx / 8;
        let bit_offset = bit_idx % 8;
        let val = (self.bytes[byte] >> (7 - bit_offset)) & 1;
        return Some(val);
    }

    pub fn iter<'a>(&'a self) -> BitRepIter<'a> {
        BitRepIter { br: self, idx: 0 }
    }
}

pub struct BitRepIter<'a> {
    br: &'a BitRep,
    idx: usize,
}

impl<'a> Iterator for BitRepIter<'a> {
    type Item = u8;
    fn next(&mut self) -> Option<Self::Item> {
        let val = self.br.get_bit(self.idx);
        if val.is_some() {
            self.idx += 1;
        }
        return val;
    }
}