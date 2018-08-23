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
#include "point_list.h"

#include <stdlib.h>

/* If you need other headers, include them here: */

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

struct point_list {
  point pt;
  point_list* next;
  point_list* prev;
};

////////////////////////////////////////////////////////////////////////////////
// local functions declarations
////////////////////////////////////////////////////////////////////////////////

/* If you need auxiliary functions, declare them here: */
static inline point_list* point_list_start(const point_list* const l);
static inline point_list* point_list_end(const point_list* const l);

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

point_list* point_list_new(void) {
  return NULL;
}

void point_list_destroy(point_list* const l) {
  point_list* tmp = point_list_start(l);
  while(tmp) {
    point_list* next = tmp->next;
    free(tmp);
    tmp = next;
  }
}

////////////////////////////////////////////////////////////////////////////////
// getters
////////////////////////////////////////////////////////////////////////////////

point point_list_get_point(const point_list* const l, size_t i) {
  point_list* tmp = point_list_start(l);
  for(size_t index = 0; index < i; ++index) {
    tmp = tmp->next;
  }
  return tmp->pt;
}

size_t point_list_get_size(const point_list* const l) {
  size_t size = 0;
  const point_list* tmp;
  for(tmp = point_list_start(l); tmp; tmp = tmp->next)
    size++;

  return size;
}

bool point_list_contains(const point_list* const l, point p) {
  bool contains = false;
  const point_list* tmp;
  for(tmp = point_list_start(l); tmp; tmp = tmp->next)
    if((tmp->pt.x == p.x) && (tmp->pt.y == p.y))
      contains = true;

  return contains;
}

////////////////////////////////////////////////////////////////////////////////
// setters / modifiers
////////////////////////////////////////////////////////////////////////////////

point_list* point_list_push_front(point_list* const l, point p) {
  point_list* const new_pt = malloc(sizeof *new_pt);

  new_pt->pt = p;

	point_list* const start = point_list_start(l);
	new_pt->prev = NULL;
	new_pt->next = start;
	if (start)
		start->prev = new_pt;

	return new_pt;
}

point_list* point_list_push_back(point_list* const l, point p) {
  point_list* const new_pt = malloc(sizeof *new_pt);

  new_pt->pt = p;

	point_list* const end = point_list_end(l);
	new_pt->prev = end;
	new_pt->next = NULL;
	if (end)
		end->next = new_pt;

	return new_pt;
}

point_list* point_list_pop_front(point_list* const l) {
	if (!l)
		return NULL;

	point_list* const start = point_list_start(l);
	point_list* const res = start->next;

	free(start);
	if (res)
		res->prev = NULL;

	return res;
}

point_list* point_list_pop_back(point_list* const l) {
	if (!l)
		return NULL;

	point_list* const end = point_list_end(l);
	point_list* const res = end->prev;

	free(end);
	if (res)
		res->next = NULL;

	return res;
}

void point_list_set_point(point_list* const l, size_t i, point p) {
  point_list* tmp = point_list_start(l);
  for(size_t index = 0; index < i; ++index)
    tmp = tmp->next;
  tmp->pt = p;
}

point_list* point_list_prune_out_of_bounds(
    point_list* const l, point up_left, point bottom_right) {

  point_list* tmp = point_list_start(l);
  for(size_t i = 0; tmp; ++i) {
    if(!point_is_in_rectangle(tmp->pt, up_left, bottom_right))
      point_list_set_point(tmp, i, point_invalid());
    tmp = tmp->next;
  }

  tmp = point_list_start(l);
  while(tmp) {
    if(!point_is_valid(tmp->pt)) {
      if(!(tmp->prev) && tmp->next) {
        tmp = tmp->next;
        free(tmp->prev);
        tmp->prev = NULL;
        return tmp;
      } else if (tmp->prev && !(tmp->next)) {
        tmp = tmp->prev;
        free(tmp->next);
        tmp->next = NULL;
        return tmp;
      } else if (tmp->prev && tmp->next) {
        point_list* tmp2 = tmp;
        tmp = tmp->prev;
        tmp->next = tmp2->next;
        tmp2->next->prev = tmp;
        free(tmp2);
      } else {
        free(tmp);
        return NULL;
      }
    } else {
      tmp = tmp->next;
    }
  }

  return l;
}

void point_list_shift_left(point_list* const l) {
  const point_list* tmp;
  size_t i = 0;
  for(tmp = point_list_start(l); tmp; tmp = tmp->next) {
    point_list_set_point(l, i, point_xy(tmp->pt.x-1, tmp->pt.y));
    i++;
  }
}

void point_list_shift_right(point_list* const l) {
  const point_list* tmp;
  size_t i = 0;
  for(tmp = point_list_start(l); tmp; tmp = tmp->next) {
    point_list_set_point(l, i, point_xy(tmp->pt.x+1, tmp->pt.y));
    i++;
  }
}

////////////////////////////////////////////////////////////////////////////////
// local functions definitions
////////////////////////////////////////////////////////////////////////////////

/* If you need auxiliary functions, define them here: */
point_list* point_list_start(const point_list* l) {
	if (!l)
		return NULL;

	const point_list* tmp = l;
	while (tmp->prev)
		tmp = tmp->prev;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	return (point_list*) tmp;
	#pragma GCC diagnostic pop
}

point_list* point_list_end(const point_list* const l) {
	if (!l)
		return NULL;

	const point_list* tmp = l;
	while (tmp->next)
		tmp = tmp->next;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
	return (point_list*) tmp;
	#pragma GCC diagnostic pop
}
