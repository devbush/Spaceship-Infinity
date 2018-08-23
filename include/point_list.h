#ifndef _POINT_LIST_H_
#define _POINT_LIST_H_

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

#include <stddef.h>
#include <stdbool.h>

#include "point.h"

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

typedef struct point_list point_list;

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

point_list* point_list_new(void);
void point_list_destroy(point_list* l);

////////////////////////////////////////////////////////////////////////////////
// getters
////////////////////////////////////////////////////////////////////////////////

size_t point_list_get_size(const point_list* l);
point point_list_get_point(const point_list* l, size_t i);
bool point_list_contains(const point_list* l, point p);

////////////////////////////////////////////////////////////////////////////////
// setters / modifiers
////////////////////////////////////////////////////////////////////////////////

void point_list_set_point(point_list* l, size_t i, point p);

point_list* point_list_push_front(point_list* l, point p);
point_list* point_list_push_back(point_list* l, point p);
point_list* point_list_pop_front(point_list* l);
point_list* point_list_pop_back(point_list* l);

void point_list_shift_left(point_list* l);
void point_list_shift_right(point_list* l);
point_list* point_list_prune_out_of_bounds(
    point_list* l, point up_left, point bottom_right);

#endif
