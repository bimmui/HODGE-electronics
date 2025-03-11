#include "huffman.h"

#include <queue>
#include <memory>

//template<std::equality_comparable V> 
//struct Leaf;
//template<std::equality_comparable V> 
//struct Internal;

//template<std::equality_comparable V> 
//using Node = std::variant<Leaf<V>, Internal<V>>;

template<std::equality_comparable V> 
void add_node_to_map(std::unordered_map<V, Pattern>& map, Node<V> node, uint64_t pattern, uint64_t pat_length) {
    if (Leaf<V>* pval = std::get_if<Leaf>(node)) {
        Pattern pat = { pattern, pat_length };
        map.insert({pval->value, pat});
    } else {
        Internal internal = std::get<Internal>(node);
        uint64_t new_pattern = (pattern << 1);
        pat_length++;
        add_node_to_map(map, *internal.left, new_pattern, pat_length);
        add_node_to_map(map, *internal.right, new_pattern | 1, pat_length);
    }
}

template<std::equality_comparable V> 
struct Leaf {
    const std::strong_ordering operator<=>(const Leaf& other) {
        return this->frequency <=> other.frequency;
    }
    const bool operator==(const Leaf& other) {
        return this->frequency == other.frequency;
    }   
    
    V value;
    uint64_t frequency;
};

template<std::equality_comparable V> 
struct Internal {
    std::unique_ptr<Node<V>> left;
    std::unique_ptr<Node<V>> right;
};


