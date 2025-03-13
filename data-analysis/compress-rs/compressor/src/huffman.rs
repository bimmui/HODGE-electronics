use std::{cmp::Ordering, collections::{BTreeMap, BinaryHeap, HashMap, HashSet}, hash::Hash};

use bitstream_io::BitRead;
use itertools::Itertools;

enum HuffmanNode<T> {
    Inner(Box<HuffmanNode<T>>, Box<HuffmanNode<T>>),
    Leaf(T),
}

impl<T> From<Node<T>> for HuffmanNode<T> {
    fn from(value: Node<T>) -> Self {
        match value {
            Node::Inner(l, r, _) => HuffmanNode::Inner(Box::new((*l).into()), Box::new((*r).into())),
            Node::Leaf(val, _) => HuffmanNode::Leaf(val),
        }
    }
}

impl<T: Copy> HuffmanNode<T> {
    pub fn next_value<R: BitRead + ?Sized>(&self, reader: &mut R) -> T {
        match self {
            HuffmanNode::Leaf(val) => *val,
            HuffmanNode::Inner(l, r) => {
                if reader.read_bit().unwrap() {
                    r.next_value(reader)
                } else {
                    l.next_value(reader)
                }
            }
        }
    }
}



enum Node<T> {
    Inner(Box<Node<T>>, Box<Node<T>>, u64),
    Leaf(T, u64),
}

impl<T> Node<T> {
    pub fn get_frequency(&self) -> u64 {
        match self {
            Self::Inner(_, _, freq) => *freq,
            Self::Leaf(_, freq) => *freq,
        }
    }
}

impl<T> Ord for Node<T> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        other.get_frequency().cmp(&self.get_frequency())
    }
} 

impl<T> PartialOrd for Node<T> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl<T> PartialEq for Node<T> {
    fn eq(&self, other: &Self) -> bool {
        self.cmp(other) == Ordering::Equal
    }
}

impl<T> Eq for Node<T> {}

pub fn create_huffman_table<T: Copy + PartialEq + Eq+ Hash>(data: Vec<(T, u64)>) -> Node<T> {
    let mut queue = BinaryHeap::from_iter(
        data.iter()
        .map(|(data, freq)| {
            Node::Leaf(*data, *freq)
        })
        .sorted()
        .dedup_by(|this, other| {
            match this {
                Node::Leaf(val, _) => {
                    match other {
                        Node::Leaf(val_other, _) => {
                            *val == *val_other
                        },
                        _ => panic!("Should only be creating leaf nodes!"),
                    }
                },
                _ => panic!("Should only be creating leaf nodes!"),
            }
        })
    );

    while queue.len() > 1 {
        let first = queue.pop().unwrap();
        let second = queue.pop().unwrap();
        let combined_freq = first.get_frequency() + second.get_frequency();
        let new = Node::Inner(Box::new(first), Box::new(second), combined_freq);
        queue.push(new);
    }

    queue.pop().unwrap()
}