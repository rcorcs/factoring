/*
 * datatypes/node.c
 *
 * version:   1.0.0
 * date:      14 September 2011
 * author:    (rcor) Rodrigo Caetano de Oliveira Rocha
 *
*/
#include "node.h"


#ifdef __cplusplus
extern "C" {
#endif

node_t *alloc_node(size_t size)
{
    node_t *node = (node_t*)malloc(sizeof(node_t));
    node->size = size;
    node->data = malloc(size);
    node->prev = NULL;
    node->next = NULL;
    return node;
}

node_t *create_node(size_t size, node_t *prev, node_t *next)
{
    node_t *node = (node_t*)malloc(sizeof(node_t));
    node->size = size;
    node->data = malloc(size);
    node->prev = prev;
    node->next = next;
    return node;
}

void destroy_node(node_t **node)
{
    if(node && *node){
        free((*node)->data);
        free(*node);
        *node = 0;
    }
}

void swap_nodes(node_t *node1, node_t *node2)
{
    void *data;
    size_t size;

    size = node1->size;
    node1->size = node2->size;
    node2->size = size;

    data = node1->data;
    node1->data = node2->data;
    node2->data = data;
}


#ifdef __cplusplus
}
#endif
