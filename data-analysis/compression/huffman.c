#include "huffman.h"

typedef struct {
    uint64_t bit_symbol;
    uint64_t pattern;
} SymbolTranslation;

typedef struct Node Node;

struct Node {
    enum { Leaf, Internal } type;
    union {
        struct {
            uint64_t bit_symbol;
        } leaf;

        struct {
            Node *left;
            Node *right;
        } internal;
    } data;

    uint64_t frequency;
    Node *parent;
};

void create_table(Array values, Array frequency, Array* symbol_table) {
    /*
        Algorithm:

        impl priority queue 

        
    */
}