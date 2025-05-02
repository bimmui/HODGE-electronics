use crate::{bitrep::{BitRep, BitRepIter}, huffman::{HuffmanCompressor, HuffmanDecoder}};

struct EFloatDecompressor {
    exp_decomp: HuffmanDecoder<u8>
}

const SIGN_BIT: u32 = 31;
const MANTISSA_LEN: u32 = 23;
const EXP_MASK: u32 = 0xFF;


impl EFloatDecompressor {
    pub fn new(exp_decomp: HuffmanDecoder<u8>) -> EFloatDecompressor {
        EFloatDecompressor { exp_decomp }
    }

    pub fn read_float(&self, it: &mut BitRepIter) -> f32 {
        // read sign bit
        let sign_bit = it.next().unwrap() as u32;
        let exp = self.exp_decomp.decode(it) as u32;
        let mut mantissa = 0;
        for _ in 0..MANTISSA_LEN {
            mantissa <<= 1;
            mantissa |= it.next().unwrap() as u32;
        }
        let full_bits = sign_bit << SIGN_BIT | exp << MANTISSA_LEN | mantissa;
        f32::from_bits(full_bits)
    }
}

struct EFloatCompressor {
    exp_comp: HuffmanCompressor<u8>
}

impl EFloatCompressor {
    pub fn new(exp_comp: HuffmanCompressor<u8>) -> EFloatCompressor {
        EFloatCompressor { exp_comp }
    }

    pub fn write_float(&self, bits: &mut BitRep, f: f32) {
        let f_bits = f.to_bits();
        bits.write_bit((f_bits >> SIGN_BIT) as u8);
        let exp = ((f_bits >> MANTISSA_LEN) & EXP_MASK) as u8;
        let exp_bits = self.exp_comp.encode(exp);
        bits.append(&exp_bits);
        for shift in (0..MANTISSA_LEN).rev() {
            bits.write_bit((f_bits >> shift & 1) as u8);
        }
    }
}