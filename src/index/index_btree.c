#include "tm/index.h"
#include <stdlib.h>
#include <string.h>

/* B-tree node for index entries */
#define TM_BTREE_ORDER 8
#define TM_BTREE_MIN_KEYS (TM_BTREE_ORDER / 2)

typedef struct tm_btree_node {
    uint64_t  keys[TM_BTREE_ORDER - 1];
    uint32_t  values[TM_BTREE_ORDER - 1];  /* segment IDs */
    struct tm_btree_node *children[TM_BTREE_ORDER];
    uint32_t  key_count;
    bool      is_leaf;
} tm_btree_node_t;

typedef struct {
    tm_btree_node_t *root;
    uint32_t         node_count;
    uint32_t         height;
    uint32_t         total_keys;
} tm_btree_t;

static tm_btree_node_t *btree_create_node(bool is_leaf)
{
    tm_btree_node_t *node = calloc(1, sizeof(*node));
    if (!node) return NULL;
    node->is_leaf = is_leaf;
    return node;
}

static void btree_destroy_node(tm_btree_node_t *node)
{
    if (!node) return;
    if (!node->is_leaf) {
        for (uint32_t i = 0; i <= node->key_count; i++) {
            btree_destroy_node(node->children[i]);
        }
    }
    free(node);
}

tm_status_t tm_btree_init(tm_btree_t *tree)
{
    if (!tree) return TM_ERR_BAD_STATE;
    memset(tree, 0, sizeof(*tree));
    tree->root = btree_create_node(true);
    if (!tree->root) return TM_ERR_ALLOC;
    tree->node_count = 1;
    tree->height = 1;
    return TM_OK;
}

void tm_btree_destroy(tm_btree_t *tree)
{
    if (tree) {
        btree_destroy_node(tree->root);
        memset(tree, 0, sizeof(*tree));
    }
}

static int btree_search_node(const tm_btree_node_t *node, uint64_t key)
{
    int lo = 0, hi = (int)node->key_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (node->keys[mid] == key) return mid;
        if (node->keys[mid] < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

uint32_t tm_btree_search(const tm_btree_t *tree, uint64_t key)
{
    if (!tree || !tree->root) return 0;
    const tm_btree_node_t *node = tree->root;
    while (node) {
        int idx = btree_search_node(node, key);
        if (idx >= 0) return node->values[idx];
        if (node->is_leaf) return 0;
        uint32_t i = 0;
        while (i < node->key_count && key > node->keys[i]) i++;
        node = node->children[i];
    }
    return 0;
}

static void btree_insert_nonfull(tm_btree_node_t *node, uint64_t key, uint32_t value);
static void btree_split_child(tm_btree_node_t *parent, uint32_t idx, tm_btree_node_t *child);

static void btree_split_child(tm_btree_node_t *parent, uint32_t idx, tm_btree_node_t *child)
{
    tm_btree_node_t *sibling = btree_create_node(child->is_leaf);
    if (!sibling) return;
    uint32_t mid = TM_BTREE_ORDER / 2 - 1;
    sibling->key_count = child->key_count - mid - 1;
    for (uint32_t j = 0; j < sibling->key_count; j++) {
        sibling->keys[j] = child->keys[mid + 1 + j];
        sibling->values[j] = child->values[mid + 1 + j];
    }
    if (!child->is_leaf) {
        for (uint32_t j = 0; j <= sibling->key_count; j++) {
            sibling->children[j] = child->children[mid + 1 + j];
        }
    }
    child->key_count = mid;
    for (int j = (int)parent->key_count; j > (int)idx; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[idx + 1] = sibling;
    for (int j = (int)parent->key_count - 1; j >= (int)idx; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }
    parent->keys[idx] = child->keys[mid];
    parent->values[idx] = child->values[mid];
    parent->key_count++;
}

static void btree_insert_nonfull(tm_btree_node_t *node, uint64_t key, uint32_t value)
{
    int i = (int)node->key_count - 1;
    if (node->is_leaf) {
        while (i >= 0 && node->keys[i] > key) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->key_count++;
    } else {
        while (i >= 0 && node->keys[i] > key) i--;
        i++;
        if (node->children[i] && node->children[i]->key_count == TM_BTREE_ORDER - 1) {
            btree_split_child(node, (uint32_t)i, node->children[i]);
            if (key > node->keys[i]) i++;
        }
        if (node->children[i])
            btree_insert_nonfull(node->children[i], key, value);
    }
}

tm_status_t tm_btree_insert(tm_btree_t *tree, uint64_t key, uint32_t value)
{
    if (!tree || !tree->root) return TM_ERR_BAD_STATE;
    if (tree->root->key_count == TM_BTREE_ORDER - 1) {
        tm_btree_node_t *new_root = btree_create_node(false);
        if (!new_root) return TM_ERR_ALLOC;
        new_root->children[0] = tree->root;
        btree_split_child(new_root, 0, tree->root);
        tree->root = new_root;
        tree->node_count++;
        tree->height++;
        btree_insert_nonfull(new_root, key, value);
    } else {
        btree_insert_nonfull(tree->root, key, value);
    }
    tree->total_keys++;
    return TM_OK;
}

static void btree_range_collect(const tm_btree_node_t *node,
                                uint64_t lo, uint64_t hi,
                                uint32_t *results, uint32_t *count, uint32_t max)
{
    if (!node || *count >= max) return;
    for (uint32_t i = 0; i < node->key_count; i++) {
        if (!node->is_leaf)
            btree_range_collect(node->children[i], lo, hi, results, count, max);
        if (node->keys[i] >= lo && node->keys[i] <= hi && *count < max) {
            results[(*count)++] = node->values[i];
        }
        if (node->keys[i] > hi) return;
    }
    if (!node->is_leaf)
        btree_range_collect(node->children[node->key_count], lo, hi, results, count, max);
}

tm_status_t tm_btree_range_query(const tm_btree_t *tree,
                                    uint64_t lo, uint64_t hi,
                                    uint32_t *results, uint32_t *count, uint32_t max)
{
    if (!tree || !results || !count) return TM_ERR_BAD_STATE;
    *count = 0;
    btree_range_collect(tree->root, lo, hi, results, count, max);
    return TM_OK;
}

typedef struct {
    uint32_t  node_count;
    uint32_t  height;
    uint32_t  total_keys;
    uint32_t  leaf_count;
    uint32_t  internal_count;
} tm_btree_stats_t;

static void btree_count_nodes(const tm_btree_node_t *node, tm_btree_stats_t *stats)
{
    if (!node) return;
    if (node->is_leaf) stats->leaf_count++;
    else {
        stats->internal_count++;
        for (uint32_t i = 0; i <= node->key_count; i++)
            btree_count_nodes(node->children[i], stats);
    }
}

tm_status_t tm_btree_compute_stats(const tm_btree_t *tree, tm_btree_stats_t *stats)
{
    if (!tree || !stats) return TM_ERR_BAD_STATE;
    memset(stats, 0, sizeof(*stats));
    stats->node_count = tree->node_count;
    stats->height = tree->height;
    stats->total_keys = tree->total_keys;
    btree_count_nodes(tree->root, stats);
    return TM_OK;
}
