/*
 * datatypes/node.h
 *
 *  Global definitions for a node structure.
 *
 * version:   1.0.0
 * date:      14 September 2011
 * author:    (rcor) Rodrigo Caetano de Oliveira Rocha
 *
*/
#ifndef __NODE
#define __NODE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "primitives.h"

typedef struct __node{
    size_t size;
    byte_t *data;

    struct __node *prev;
    struct __node *next;
} node_t;

#define NODEGET( _TYPE_, _NODE_ ) (*((_TYPE_*)(_NODE_)->data))

node_t *alloc_node(size_t size);
node_t *create_node(size_t size, node_t *prev, node_t *next);
void destroy_node(node_t **node);
void swap_nodes(node_t *node1, node_t *node2);



#ifdef __cplusplus
}
#endif

#endif
