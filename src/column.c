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
#include "column.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sysexits.h>

/* If you need other headers, include them here: */

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

struct column {
  cell* cells;
  int height;
};

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

column* column_new(const int height, const int low, const int high) {
  column* const c = malloc(sizeof *c);
  if (!c) {
    perror("malloc");
    exit(EX_OSERR);
  }
  cell* const cells = malloc(sizeof *cells * (size_t) height);
  if (!cells) {
    perror("malloc");
    exit(EX_OSERR);
  }

  for (int i = 0; i < height; ++i)
    cells[i] = i > high || i < low ? CELL_WALL : CELL_EMPTY;
  c->cells = cells;
  c->height = height;

  return c;
}

void column_destroy(column* const c) {
  if (!c)
    return;

  if (c->cells)
    free(c->cells);
  free(c);
}

////////////////////////////////////////////////////////////////////////////////
// getters
////////////////////////////////////////////////////////////////////////////////

cell column_get_cell(const column* const c, const size_t i) {
  return c && c->cells && i < (size_t) c->height ? c->cells[i] : CELL_EMPTY;
}

////////////////////////////////////////////////////////////////////////////////
// setters / modifiers
////////////////////////////////////////////////////////////////////////////////

void column_set_cell(column* const c, const size_t i, const cell x) {
  if (c)
    c->cells[i] = x;
}

///////////////////////////////////////////
// Version sans gravité sur les objets
///////////////////////////////////////////

void column_fall(column* const c) {
  int groups[c->height / 2 + 2][2];
  int i;
  for(i = 0; i < c->height / 2 + 2; ++i) {
    groups[i][0] = -1;
    groups[i][1] = -1;
  }

  int index = 0;
  for(i = 0; i < c->height; ++i) {
    if(column_get_cell(c, (size_t) i) == CELL_WALL) {
      groups[index][0] = i;
      while(column_get_cell(c, (size_t) i) == CELL_WALL) {
        groups[index][1] = i;
        i++;
      }
      index++;
    }
  }

  i = 0;
  while(groups[i][0] != -1 && groups[i][1] != -1) {
    if((groups[i][0] != 0) && (groups[i][1] != c->height - 1) && (column_get_cell(c, (size_t) groups[i][1] + 1) == CELL_EMPTY)) {
      column_set_cell(c, (size_t) groups[i][0], CELL_EMPTY);
      column_set_cell(c, (size_t) groups[i][1] + 1, CELL_WALL);
    }
    i++;
  }
}



///////////////////////////////////////////
// Version avec gravité sur les objets
///////////////////////////////////////////

// void column_fall(column* const c) {
//   size_t first_empty = 0;
//   size_t last_empty = 0;
//   int only_empty = 1;
//
//   for(int i = 0; i < c->height; ++i) {
//     if(column_get_cell(c, (size_t) i) == CELL_EMPTY) {
//       first_empty = (size_t) i;
//       break;
//     }
//   }
//   for(int j = c->height - 1; j >= 0; --j) {
//     if(column_get_cell(c, (size_t) j) == CELL_EMPTY) {
//       last_empty = (size_t) j;
//       break;
//     }
//   }
//
//   if(first_empty != last_empty) {
//     for(size_t x = last_empty; x > first_empty; x--)
//       if(column_get_cell(c, x) != CELL_EMPTY)
//         only_empty = 0;
//
//     if(only_empty == 0) {
//       for(size_t y = last_empty; y > first_empty; y--)
//         column_set_cell(c, y, column_get_cell(c, y - 1));
//       column_set_cell(c, first_empty, CELL_EMPTY);
//     }
//   }
// }
