/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef SLLH
#define SLLH

struct sll_node {
	void *data;
	size_t sz_data;

	void *key;
	size_t sz_key;

	struct sll_node *next;
};

#define SLL_NEW_NODE (struct sll_node *)zalloc(sizeof(struct sll_node));

#define SLL_SET_KEY(node, _key, _sz_key) \
	(node)->key = _key;		\
	(node)->sz_key = (_sz_key);

#define sll_for_ref(walk, from)	\
	for ((walk) = (from); (walk) && *(walk); (walk) = &((*walk)->next))

#define sll_for_node(walk, from)	\
	for((walk) = (from); (walk); (walk) = (walk)->next)

extern struct sll_node *sll_add(struct sll_node **ref);

extern void sll_add_node(struct sll_node **ref, struct sll_node *new);

extern struct sll_node **sll_find(struct sll_node **head, 
		const void *key, size_t sz_key);

extern struct sll_node *sll_repl(struct sll_node **head, 
		const void *key, size_t sz_key);

extern struct sll_node *sll_rm(struct sll_node **ref);

extern size_t sll_rmall(struct sll_node **ref, 
		void (*free_data)(void *), void (*free_key)(void *));

#endif
