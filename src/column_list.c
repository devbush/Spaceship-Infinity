/*
 *        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */
#include "column_list.h"

#include <stdlib.h>

/* If you need other headers, include them here: */

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

struct column_list {
  column* col;
  column_list* next;
  column_list* prev;
};

////////////////////////////////////////////////////////////////////////////////
// local functions declarations
////////////////////////////////////////////////////////////////////////////////

/* If you need auxiliary functions, declare them here: */
static inline column_list* column_list_start(const column_list* const l);
static inline column_list* column_list_end(const column_list* const l);

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

column_list* column_list_new(void) {
  return NULL;
}

void column_list_destroy(column_list* const l) {
  column_list* tmp = l;
  while(tmp) {
    column_list* next = tmp->next;
    column_destroy(tmp->col);
    free(tmp);
    tmp = next;
  }
}

////////////////////////////////////////////////////////////////////////////////
// getters
////////////////////////////////////////////////////////////////////////////////

column* column_list_get_column(const column_list* const l, const size_t i) {
  column_list* tmp = column_list_start(l);
  for(size_t index = 0; index < i; ++index)
    tmp = tmp->next;

  return tmp->col;
}

size_t column_list_get_size(const column_list* const l) {
  size_t size = 0;
  const column_list* tmp;
  for(tmp = column_list_start(l); tmp; tmp = tmp->next)
    size++;

  return size;
}

////////////////////////////////////////////////////////////////////////////////
// setters / modifiers
////////////////////////////////////////////////////////////////////////////////

column_list* column_list_push_front(column_list* const l, column* const c) {
  column_list* const new_col = malloc(sizeof *new_col);

  new_col->col = c;

	column_list* const start = column_list_start(l);
	new_col->prev = NULL;
	new_col->next = start;
	if (start)
		start->prev = new_col;

	return new_col;
}

column_list* column_list_push_back(column_list* const l, column* const c) {
  column_list* const new_col = malloc(sizeof *new_col);

  new_col->col = c;

	column_list* const end = column_list_end(l);
	new_col->prev = end;
	new_col->next = NULL;
	if (end)
		end->next = new_col;

	return new_col;
}

column_list* column_list_pop_front(column_list* const l) {
	if (!l)
		return NULL;

	column_list* const start = column_list_start(l);
	column_list* const res = start->next;

	column_destroy(start->col);
	free(start);
	if (res)
		res->prev = NULL;

	return res;
}

column_list* column_list_pop_back(column_list* const l) {
	if (!l)
		return NULL;

	column_list* const end = column_list_end(l);
	column_list* const res = end->prev;

  column_destroy(end->col);
	free(end);
	if (res)
		res->next = NULL;

	return res;
}

////////////////////////////////////////////////////////////////////////////////
// local functions definitions
////////////////////////////////////////////////////////////////////////////////

/* If you need auxiliary functions, define them here: */
column_list* column_list_start(const column_list* l) {
	if (!l)
		return NULL;

	const column_list* tmp = l;
	while (tmp->prev)
		tmp = tmp->prev;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	return (column_list*) tmp;
	#pragma GCC diagnostic pop
}

column_list* column_list_end(const column_list* const l) {
	if (!l)
		return NULL;

	const column_list* tmp = l;
	while (tmp->next)
		tmp = tmp->next;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	return (column_list*) tmp;
	#pragma GCC diagnostic pop
}
