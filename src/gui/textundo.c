/*
 *  An undo-redo facility, particularly for text editing.
 *
 *  Platform: Neutral
 *
 *  Version: 3.49  2003/09/09  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include "app.h"

typedef struct TCStack      TCStack;
typedef struct TCNode       TCNode;
typedef struct TCList       TCList;

typedef enum TextCommand {
	TC_UNDEFINED,
	TC_TYPING,
	TC_BKSP,
	TC_DEL,
	TC_PASTE,
	TC_CUT,
	TC_SELECT
} TextCommand;

struct TCNode {
	TCNode *	next;
	TextCommand	command;
	int		caret;
	int		selected;
	int		text_length;
	char *		text;
};

struct TCStack {
	int		count;
	TCNode *	head;
};

struct TCList {
	TCStack *	undo;
	TCStack *	redo;
	int		saved;
	int		add_to_undo;
};

TCNode * app_new_tcnode(void)
{
	TCNode *node;

	node = app_alloc(sizeof(TCNode));
	if (node == NULL)
		return NULL;
	node->next = NULL;
	node->command = TC_UNDEFINED;
	node->caret = 0;
	node->selected = 0;
	node->text_length = 0;
	node->text = NULL;
	return node;
}

void app_del_tcnode(TCNode *node)
{
	if (node == NULL)
		return;
	app_free(node->text);
	app_free(node);
}

TCStack * app_new_tcstack(void)
{
	TCStack *stack;

	stack = app_alloc(sizeof(TCStack));
	if (stack == NULL)
		return NULL;
	stack->count = 0;
	stack->head = NULL;
	return stack;
}

void app_del_tcstack(TCStack *stack)
{
	TCNode *node;

	if (stack == NULL)
		return;
	while ((node = stack->head) != NULL) {
		stack->head = node->next;
		app_del_tcnode(node);
	}
	app_free(stack);
}

void app_tcstack_push(TCStack *stack, TCNode *node)
{
	TCNode *prev = stack->head;
	stack->head = node;
	stack->count++;
	node->next = prev;
}

TCNode * app_tcstack_pop(TCStack *stack)
{
	TCNode *prev = stack->head;
	if (stack->count > 0) {
		stack->head = prev->next;
		stack->count--;
	}
	return prev;
}

TCList * app_new_tclist(void)
{
	TCList *list;

	list = app_alloc(sizeof(TCList));
	if (list == NULL)
		return NULL;
	list->undo = app_new_tcstack();
	list->redo = app_new_tcstack();
	list->saved = -1;
	list->add_to_undo = 1;
	if ((list->undo == NULL) || (list->redo == NULL)) {
		app_del_tcstack(list->undo);
		app_del_tcstack(list->redo);
		app_free(list);
		return NULL;
	}
	return list;
}

void app_del_tclist(TCList *list)
{
	if (list == NULL)
		return;
	app_del_tcstack(list->undo);
	app_del_tcstack(list->redo);
	app_free(list);
}

int app_tclist_undo_count(TCList *list)
{
	return list->undo->count;
}

int app_tclist_redo_count(TCList *list)
{
	return list->redo->count;
}

void app_tclist_save(TCList *list)
{
	list->saved = list->undo->count;
}

int app_tclist_is_saved(TCList *list)
{
	return (list->saved == list->undo->count);
}

void app_tclist_add(TCList *list, TextCommand cmd,
			int caret, int selected, int len, char *utf8)
{
	TCNode *node;

	node = app_new_tcnode();
	node->command = cmd;
	node->caret = caret;
	node->selected = selected;
	node->text_length = len;
	node->text = utf8;

	if (list->add_to_undo)
		app_tcstack_push(list->undo, node);
	else
		app_tcstack_push(list->redo, node);
}

