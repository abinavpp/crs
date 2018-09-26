/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef QUEUEH
#define QUEUEH

#include "util.h"
#include "sll.h"

struct queue_struct {
	struct sll_node *front;
	struct sll_node *rear;

	size_t capacity;
	size_t count;
};


#define QUEUE_NEW zalloc(sizeof(struct queue_struct));

#define QUEUE_IS_EMPTY(qs)	(!(qs)->count)
#define QUEUE_IS_FULL(qs)	((qs)->count == (qs)->capacity)


extern struct sll_node *queue_push(struct queue_struct *qs);
extern struct sll_node *queue_pop(struct queue_struct *qs);

#endif
