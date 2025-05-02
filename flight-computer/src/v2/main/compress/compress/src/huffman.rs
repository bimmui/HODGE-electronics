use std::{collections::{BinaryHeap, HashMap}, hash::Hash};
use crate::bitrep::BitRep;
use itertools::Itertools;

enum HuffmanNode<T: Clone + Copy + PartialEq + Eq + Hash> {
    Inner(Box<HuffmanNode<T>>, Box<HuffmanNode<T>>),
    Leaf(T)
}

struct DataFreq<T: Clone + Copy + PartialEq + Eq + Hash> {
    pub data: HuffmanNode<T>,
    pub freq: u64,
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> PartialEq for DataFreq<T> {
    fn eq(&self, other: &Self) -> bool {
        self.freq == other.freq
    }
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> Eq for DataFreq<T> {}

// compares so that 'greater' means lower frequency 
impl<T: Clone + Copy + PartialEq + Eq + Hash> PartialOrd for DataFreq<T> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        other.freq.partial_cmp(&self.freq)
    }
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> Ord for DataFreq<T> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        other.partial_cmp(self).unwrap()
    }
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> HuffmanNode<T> {
    pub fn build(data: Vec<T>) -> HuffmanNode<T> {
        let mut freqs = data
            .iter()
            .unique()
            .map(|i| (*i, 0))
            .collect::<HashMap<T, u64>>();
        data.iter().for_each(|i| *freqs.get_mut(i).unwrap() += 1);
        let mut queue = BinaryHeap::from(
            freqs
            .into_iter()
            .map(|i| DataFreq { data: HuffmanNode::Leaf(i.0), freq: i.1 })
            .collect::<Vec<DataFreq<T>>>()
        );

        while queue.len() > 1 {
            let first = queue.pop().unwrap();
            let second = queue.pop().unwrap(); 
            let top = HuffmanNode::Inner(Box::new(first.data), Box::new(second.data));
            let top_data = DataFreq { data: top, freq: first.freq + second.freq };
            queue.push(top_data);
        }

        return queue.pop().unwrap().data;
    }
}

type TranslationTable<T: Clone + Copy + PartialEq + Eq + Hash> = HashMap<T, BitRep>;

struct HuffmanCompressor<T: Clone + Copy + PartialEq + Eq + Hash> {
    table: TranslationTable<T>
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> HuffmanCompressor<T> {
    pub fn add_to_table(table: &mut TranslationTable<T>, tree: &HuffmanNode<T>, path: BitRep) {
        match tree {
            HuffmanNode::Leaf(v) => { *table.get_mut(v).unwrap() = path },
            HuffmanNode::Inner(l, r) => {
                let left_path = path.push_zero();
                let right_path = path.push_one();
                Self::add_to_table(table, l, left_path);
                Self::add_to_table(table, r, right_path);
            }
        }
    }
    pub fn build(tree: &HuffmanNode<T>) -> HuffmanCompressor<T> {
        let mut table = TranslationTable::new();
        Self::add_to_table(&mut table, tree, BitRep::new());
        HuffmanCompressor { table }
    }
}

struct HuffmanDecoder<T: Clone + Copy + PartialEq + Eq + Hash> {
    arr: Vec<Option<T>>
}

impl<T: Clone + Copy + PartialEq + Eq + Hash> HuffmanDecoder<T> {
    pub fn bits_to_idx(b: &BitRep) -> usize {
        let mut idx = 1;
        let mut it = b.iter();
        while let Some(v) = it.next() {
            idx *= 2;
            idx += v as usize;
        }
        return idx;
    }

    pub fn bits_to_idx_consuming<I: Iterator<Item = u8>>(&self, it: &mut I) -> usize {
        let mut idx = 1;
        while self.arr[idx].is_none() {
            idx *= 2;
            idx += it.next().unwrap() as usize; // if we fail here we've built the tree wrong
        }
        return idx;
    }

    pub fn decode<I: Iterator<Item = u8>>(&self, it: &mut I) -> T {
        return self.arr[self.bits_to_idx_consuming(it)].unwrap();
    }

    fn get_values(tree: &HuffmanNode<T>, path: BitRep) -> Vec<(T, BitRep)> {
        match tree {
            HuffmanNode::Leaf(v) => vec![(*v, path)],
            HuffmanNode::Inner(l, r) => {
                let left_path = path.push_zero();
                let right_path = path.push_one();
                let mut v = HuffmanDecoder::get_values(l.as_ref(), left_path);
                v.append(&mut HuffmanDecoder::get_values(r.as_ref(), right_path));
                return v; 
            }
        }
    }

    pub fn build_decompressor(tree: &HuffmanNode<T>) -> HuffmanDecoder<T> {
        let val_list = Self::get_values(tree, BitRep::new());
        let arr_size = val_list
            .iter()
            .map(|v| Self::bits_to_idx(&v.1))
            .max().unwrap();
        let mut arr = vec![None; arr_size];
        for (val, b) in val_list {
            arr[Self::bits_to_idx(&b)] = Some(val);
        }
        HuffmanDecoder { arr }
    }
}

