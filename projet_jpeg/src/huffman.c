
#include "huffman.h"
#include "common.h"


struct node {
        struct huff_table *left;
        struct huff_table *right;
};

enum node_type {
        NODE,
        LEAF,
        NB_NODE_TYPES
};

struct huff_table {
        enum node_type type;

        union {
                struct node node;
                int8_t val;
        } u;
};


struct huff_table* create_node(enum node_type type, int8_t val)
{
        struct huff_table *node = malloc(sizeof(struct huff_table));

        if (node != NULL) {
                node->type = type;

                if (type == NODE) {
                        node->u.node.left = NULL;
                        node->u.node.right = NULL;
                }
                else
                        node->u.val = val;
        }

        return node;
}

int8_t add_huffman_code(int8_t value, uint8_t code_size, struct huff_table *parent)
{
        int8_t error = 0;


        if (parent == NULL || parent->type == LEAF)
                error = 1;

        else {
                struct huff_table **left, **right;

                left = &parent->u.node.left;
                right = &parent->u.node.right;


                if (code_size == 0) {
                        if (*left == NULL)
                                *left = create_node(LEAF, value);

                        else if (*right == NULL)
                                *right = create_node(LEAF, value);

                        else
                                error = -1;
                }
                else {
                        if (*left == NULL)
                                *left = create_node(NODE, 0);

                        error = add_huffman_code(value, code_size - 1, *left);

                        if (error) {
                                if (*right == NULL)
                                        *right = create_node(NODE, 0);

                                error = add_huffman_code(value, code_size - 1, *right);
                        }
                }
        }

        return error;
}

struct huff_table *load_huffman_table(
                struct bitstream *stream, uint16_t *nb_byte_read)
{
        uint8_t code_sizes[16];
        uint16_t nb_codes = 0;
        int32_t size_read = 0;
        uint32_t dest;
        struct huff_table *table = NULL;

        if (nb_byte_read == NULL)
                return NULL;

        *nb_byte_read = -1;


        for (uint8_t i = 0; i < 16; i++) {
                size_read += read_bitstream(stream, 8, &dest, false);
                code_sizes[i] = dest & 0xFF;
        }

        for (uint8_t i = 0; i < sizeof(code_sizes); ++i)
                nb_codes += code_sizes[i];


        /* There must be less than 256 different codes */
        if (nb_codes >= 256)
                return NULL;

        else {
		table = calloc(1, sizeof(struct huff_table));
                if (table == NULL)
                        return NULL;
        }

        for (uint8_t i = 0; i < sizeof(code_sizes); ++i) {
                for (uint8_t j = 0; j < code_sizes[i]; ++j) {
                        size_read += read_bitstream(stream, 8, &dest, false);
                        add_huffman_code(dest & 0xFF, i, table);
                }
        }

        *nb_byte_read = size_read / 8;


        return table;
}

int8_t next_huffman_value(struct huff_table *table, 
                struct bitstream *stream)
{
        int8_t bit;
        int8_t result = 0;
        uint32_t dest;

        while (table && table->type == NODE) {
                read_bitstream(stream, 1, &dest, true);
                bit = dest & 1;

                if (bit)
                        table = table->u.node.right;

                else
                        table = table->u.node.left;
        }

        if (table != NULL)
            result = table->u.val;

        return result;
}

void free_huffman_table(struct huff_table *table)
{
        if (table != NULL) {
                if (table->type == NODE) {
                        free_huffman_table(table->u.node.left);
                        free_huffman_table(table->u.node.right);
                }

                SAFE_FREE(table);
        }
}

