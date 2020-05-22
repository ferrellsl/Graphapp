/*
 *  String hash table:
 *
 *  Platform: Neutral
 *
 *  Version: 3.35  2002/12/23  First release.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

/*
 *  A hash table in which the keys and values are strings.
 *
 *  The table will dynamically resize to maintain efficiency.
 *
 *  Idea: the hash table resizes itself when certain conditions
 *  are met. These conditions are:
 *    Increase: num_nodes >= table_size * 16 (insertion) or
 *    Decrease: num_nodes <  table_size * 4 (deletion)
 *
 *  Number of nodes (num_nodes) = total number of things in table.
 *  Table size (table_size) = the size of the base hash table array.
 *  Each table entry points to a bucket (head of a linked list).
 *
 *  Note: the keys and values are not copied, for efficiency.
 *  If reading keys and/or values into a common buffer, they should
 *  be duplicated before storing them into the table. Later they
 *  should be deleted using the traversal function, before the
 *  table itself is deleted. This duplication and deletion of
 *  keys and values is unnecessary if the keys and values are
 *  compiled-in strings.
 *  Since values are only ever stored into the table, and never
 *  otherwise referenced, they could in fact be pointers to any
 *  kind of data.
 */

#include "apputils.h"

enum {
	HASH_MAX_RATIO	= 16,
	HASH_MIN_RATIO	= 4,
	HASH_MULT	= 37
};

/*
 *  Internal hashing function.
 */
static unsigned long app_hash_string(const char *str, unsigned long length)
{
	unsigned char *ch;
	unsigned long h = 0;
	for (ch = (unsigned char *) str; length--; ch++)
		h = h * HASH_MULT + *ch;
	return h;
}

/*
 *  Internal comparison function for locating nodes.
 */
static int app_compare_keys(const char *key1, unsigned long length1,
			const char *key2, unsigned long length2)
{
	int result;
	unsigned long minimum;

	minimum = length1;
	if (minimum > length2)
		minimum = length2;
	result = memcmp(key1, key2, minimum);
	if (result == 0)
		if (length1 == length2)
			return 0;
		else if (length1 < length2)
			return -1;
		else
			return 1;
	else
		return result;
}

/*
 *  Create a new empty string hash table.
 */
StringTable * app_new_string_table(void)
{
	StringTable *table;

	table = app_alloc(sizeof(StringTable));
	table->size = 1;
	table->num_nodes = 0;
	table->nodes = app_alloc(sizeof(StringNode *));
	table->nodes[0] = NULL;
	return table;
}

/*
 *  Delete the table and all nodes.
 *  Keys and values are untouched by this function.
 */
void app_del_string_table(StringTable *table)
{
	unsigned long i, size;
	StringNode **nodes;
	StringNode *n;
	StringNode *next;

	size = table->size;
	nodes = table->nodes;

	for (i=0; i < size; i++) {
		for (n=nodes[i]; n; n=next) {
			next = n->next;
			app_free(n);
		}
	}
	app_free(nodes);
	app_free(table);
}

/*
 *  Traverse all nodes in the table, applying the function to each.
 */
void app_traverse_string_table(StringTable *table,
			void (*func)(StringNode *, void *), void *data)
{
	unsigned long i, size;
	StringNode **nodes;
	StringNode *n;
	StringNode *next;

	size = table->size;
	nodes = table->nodes;

	for (i=0; i < size; i++) {
		for (n=nodes[i]; n; n=next) {
			next = n->next;
			func(n, data);
		}
	}
}

/*
 *  Find a node in the table by specifying its key.
 */
StringNode * app_locate_node(StringTable *table,
			const char *key, unsigned long length)
{
	unsigned long h;
	StringNode *n;

	h = app_hash_string(key, length) % table->size;
	for (n=table->nodes[h]; n; n=n->next)
		if (app_compare_keys(key, length, n->key, n->length) == 0)
			return n;
	return n;
}

/*
 *  Internal function used to resize the table.
 *  Create a new array and reinsert all entries.
 *  There is no need to rehash any entries, since the complete hash
 *  of each entry has previously been calculated and remembered.
 */
static void app_reinsert_all(StringTable *table, unsigned long new_size)
{
	unsigned long h, i, bytes, old_size;
	StringNode **new_nodes;
	StringNode **old_nodes;
	StringNode *n;
	StringNode *next;

	bytes = sizeof(StringNode *) * new_size;
	new_nodes = app_alloc(bytes);
	memset(new_nodes, 0, bytes);

	old_size = table->size;
	old_nodes = table->nodes;
	for (i=0; i < old_size; i++) {
		for (n=old_nodes[i]; n; n=next) {
			next = n->next;
			h = n->hash % new_size;
			n->next = new_nodes[h];
			new_nodes[h] = n;
		}
	}
	app_free(old_nodes);
	table->nodes = new_nodes;
	table->size = new_size;
}

/*
 *  Insert a node into the table with the given key.
 *  If such a node already exists, simply return it.
 */
StringNode * app_insert_node(StringTable *table,
			const char *key, unsigned long length)
{
	unsigned long h;
	StringNode *n;

	/* avoid replicating the same string */
	n = app_locate_node(table, key, length);
	if (n != NULL)
		return n;

	/* check if the table needs its size increased */
	if ( ((table->num_nodes +1) / HASH_MAX_RATIO) >= table->size )
		app_reinsert_all(table, table->size *2);
	table->num_nodes++;

	/* create a new node and insert it */
	h = app_hash_string(key, length);
	n = app_alloc(sizeof(StringNode));
	n->key = key;
	n->length = length;
	n->hash = h;
	n->value = NULL;
	h = h % table->size;
	n->next = table->nodes[h];
	table->nodes[h] = n;

	return n;
}

/*
 *  Remove the node with the given key (if any) from the table.
 *  The node is not deleted, simply returned.
 */
StringNode * app_remove_node(StringTable *table,
			const char *key, unsigned long length)
{
	unsigned long h, new_size;
	StringNode *n;
	StringNode *prev = NULL;

	/* find the node */
	h = app_hash_string(key, length) % table->size;
	for (n=table->nodes[h]; n; n=n->next) {
		if (app_compare_keys(key, length, n->key, n->length) == 0)
		{
			/* unlink the found node */
			if (prev)
				prev->next = n->next;
			else
				table->nodes[h] = n->next;
			n->next = NULL;
			break;
		}
		prev = n;
	}
	if (n == NULL)
		return NULL;

	/* check if the table needs its size decreased */
	if (table->num_nodes -1 < table->size * HASH_MIN_RATIO) {
		new_size = table->size / 2;
		if (new_size <= 0)
			new_size = 1;
		if (new_size < table->size)
			app_reinsert_all(table, new_size);
	}
	table->num_nodes--;

	return n;
}
