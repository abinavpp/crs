/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include "queue.h"

struct sll_node *queue_push(struct queue_struct *qs)
{
	qs->count++;
	if (qs->rear) {
		qs->rear = sll_add(&(qs->rear->next));
	} else { /* empty queue */
		qs->rear = sll_add(&(qs->front));
	}

	return qs->rear;
}

struct sll_node *queue_pop(struct queue_struct *qs)
{
	struct sll_node *popped;

	qs->count--;
	popped = sll_rm(&(qs->front));

	if (!qs->front) /* empty queue */
		qs->rear = NULL; /* re-init so next queue_pushes to front */
	return popped;
}
