#pragma once 

#include <algorithm>
#include <cstdint>
#include <concepts>
#include <unordered_map>
#include <variant>
#include <vector>

#include <queue>
#include <memory>

struct Pattern {
    uint64_t pattern;
    uint64_t pat_length;
};

template<std::equality_comparable V> 
struct Leaf;
template<std::equality_comparable V> 
struct Internal;

template<std::equality_comparable V> 
using Node = std::variant<Leaf<V>, Internal<V>>;

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


// maybe a uint64_t?
template<std::equality_comparable V>
std::unordered_map<V, Pattern> create_table(std::vector<V> values, std::vector<uint32_t> frequency) {
    std::vector<Node<V>> nodes = std::vector<Node<V>>(values.size());
    auto leaf_map_fn = [i = 0](V value) mutable { return Leaf { value, i++ }; };
    std::transform(values.cbegin(), values.cend(), nodes.begin(), leaf_map_fn);

    std::priority_queue<Node<V>> queue {std::less<Node<V>>(), nodes};
    
    while (queue.size() > 1) {
        Node<V> top1 = queue.top(); queue.pop();
        Node<V> top2 = queue.top(); queue.pop();

        Node<V> new_node = Internal { 
            left: std::unique_ptr<Node<V>>(top1),
            right: std::unique_ptr<Node<V>>(top2), 
        };

        queue.push(new_node);
    }

    std::unordered_map<V, Pattern> map = new std::unordered_map(nodes.size());
    Node<V> only_node = queue.top();
    add_node_to_map(map, only_node, 0, 1);
    return map;
}
