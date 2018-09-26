/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "sll.h"

/*
 * Adds new at ref so that (*ref) points to new and
 * its "next" points the "old ref".
 * */
void sll_add_node(struct sll_node **ref, struct sll_node *new)
{
	new->next = (*ref);
	*ref = new;
}

struct sll_node *sll_add(struct sll_node **ref)
{
	struct sll_node *new = SLL_NEW_NODE;
	sll_add_node(ref, new);
	return new;
}

/*
 * returns a ref to node found from head having the same key (of length
 * sz_key) using memcmp(), returns NULL if not found.
 * */
struct sll_node **sll_find(struct sll_node **head, 
		const void *key, size_t sz_key)
{
	struct sll_node **walk;

	sll_for_ref(walk, head) {
		if ((*walk)->sz_key == sz_key) {
			if (memcmp((*walk)->key, key, sz_key) == 0)
				return walk;
		}
	}

	return NULL;
}

/*
 * returns node if found with the same key (of length sz_key), if not found
 * adds a node using sll_add and returns its allocation.
 */
struct sll_node *sll_repl(struct sll_node **head, 
		const void *key, size_t sz_key)
{
	struct sll_node **ref_target;

	ref_target = sll_find(head, key, sz_key);

	if (ref_target) {
		return *ref_target;
	} else {
		return sll_add(head);
	}
}

/* makes (*ref) point to (*ref)->next. Returns removed */
struct sll_node *sll_rm(struct sll_node **ref)
{
	struct sll_node *target;

	target = (*ref);
	(*ref) = (*ref)->next;
	return target;
}

/*
 * Frees all nodes, their data & key (using free_data and key) from ref.
 * *ref assigned to null, returns # of nodes deleted.
 * */
size_t sll_rmall(struct sll_node **ref,
		void (*free_data)(void *), void (*free_key)(void *))
{
	struct sll_node *cur, *next;
	size_t delcnt = 0;

	for (cur = *ref; cur; cur = next) {
		next = cur->next;
		if (free_data)
			free_data(cur->data);
		if (free_key)
			free_key(cur->key);
		free(cur);
		delcnt++;
	}

	*ref = NULL;
	return delcnt;
}
