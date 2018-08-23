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
#include "game.h"

#include <stdio.h>
#include <tgmath.h>
#include <float.h>
#include <sysexits.h>

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

struct game {
  spaceship_options options;
  terrain* map;
  point ship;
  bool debug;
  double delay;
  double elapsed_time;
  intmax_t bonus;
  int last_key;
  point_list* bullets;
  size_t bullet_max;
  intmax_t lives;
  int pause;
  bool laser;
};

////////////////////////////////////////////////////////////////////////////////
// local functions declarations
////////////////////////////////////////////////////////////////////////////////

static intmax_t game_get_bonus(const game* g);
static column* game_get_ship_column(const game* g);

static void game_shift_right(game* g);
static void game_move_bullets(game* g);
static void game_fall(game* g);
static void game_check_bullets(game* g);
static void game_check_special_cells(game* g);
static void game_add_bonus(game* g, intmax_t bonus);
static void game_lose_life(game* const g);
static void game_add_life(game* const g);

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

game* game_init(const spaceship_options options) {
  const int height = options.height;
  const int w = options.width;
  const int difficulty = options.difficulty;
  const int ammo = options.ammo;
  const intmax_t lives = options.lives;

  game* const g = malloc(sizeof *g);
  if (!g) {
    perror("malloc");
    exit(EX_OSERR);
  }
  terrain* const map = terrain_init(height, w, difficulty);
  if (!map) {
    perror("malloc");
    exit(EX_OSERR);
  }
  /*
   * Select an empty cell for the ship... we don't want the game to be over
   * right away
   */
  const point ship = terrain_start_point(map);

  g->ship = ship;
  g->map = map;
  g->last_key = 0;
  g->debug = options.debug;
  g->options = options;
  g->bonus = 0;
  g->lives = lives;
  g->pause = 0;
  g->laser = false;
  g->elapsed_time = 0.0;
  if (ammo > 0)
    g->bullet_max = (size_t) ammo;
  else
    g->bullet_max = difficulty < 3 ? 5 - (size_t) difficulty : 1;
  g->bullets = point_list_new();
  g->delay = DBL_MIN;

  return g;
}

void game_destroy(game* const g) {
  if (!g)
    return;

  if (g->map)
    terrain_destroy(g->map);
  if (g->bullets)
    point_list_destroy(g->bullets);
  free(g);
}

////////////////////////////////////////////////////////////////////////////////
// getters
////////////////////////////////////////////////////////////////////////////////

double game_get_delay(const game* const g) {
  return g->delay;
}

double game_get_elapsed_time(const game* const g) {
  return g->elapsed_time;
}

intmax_t game_get_score(const game* const g) {
  const double elapsed = game_get_elapsed_time(g);
  const intmax_t bonus = game_get_bonus(g);
  const intmax_t score = bonus + (intmax_t) elapsed * 10;
  return score;
}

double game_get_constant_delay(const game* const g) {
  return g->options.constant_delay;
}

void game_reset_constant_delay(game* const g) {
  g->options.constant_delay = -1.0;
}

spaceship_options game_get_options(const game* const g) {
  return g->options;
}

terrain* game_get_map(const game* const g) {
  return g->map;
}

point game_get_ship_position(const game* const g) {
  return g->ship;
}

size_t game_get_max_ammo(const game* const g) {
  return g->bullet_max;
}

size_t game_get_fired_bullets(const game* const g) {
  return point_list_get_size(g->bullets);
}

point_list* game_get_bullets(const game* const g) {
  return g->bullets;
}

int game_get_last_input(const game* const g) {
  return g->last_key;
}

bool game_ship_is_alive(const game* const g) {
  return g->lives != 0;
}

bool game_get_laser(const game* const g) {
  return g->laser;
}


////////////////////////////////////////////////////////////////////////////////
// setters / modifiers
////////////////////////////////////////////////////////////////////////////////

void game_compute_turn(game* const g) {
  game_check_special_cells(g);
  game_check_bullets(g);
  game_shift_right(g);
  game_check_bullets(g);
  game_move_bullets(g);
  game_check_bullets(g);
  game_fall(g);
  game_check_bullets(g);
  game_check_special_cells(g);
}

void game_set_delay(game* const g, const double delay) {
  g->delay = delay;
}

void game_set_elapsed_time(game* const g, const double t) {
  g->elapsed_time = t;
}

void game_set_laser(game* const g, const bool active) {
  g->laser = active;
}

void game_pause_process_input(game* const g, const int key) {
  int p = game_pause_menu(g);
  if(p <= 4) {
    switch (key) {
      /* Haut. */
      case 65:
        p = (p - 1 < 1) ? 4 : p - 1;
        game_pause(g, p);
        break;
      /* Bas. */
      case 66:
        p = (p + 1 > 4) ? 1 : p + 1;
        game_pause(g, p);
        break;
      default:
        break;
    }
  } else {
    switch (key) {
      /* Haut. */
      case 65:
        p = (p - 1 < 11) ? 14 : p - 1;
        game_pause(g, p);
        break;
      /* Bas. */
      case 66:
        p = (p + 1 > 14) ? 11 : p + 1;
        game_pause(g, p);
        break;
      default:
        break;
    }

  }
}

void game_process_input(game* const g, const int key) {
  const spaceship_options options = g->options;
  const int height = terrain_height(g->map);
  const int difficulty = options.difficulty;
  const bool tail = options.tail;
  const bool pretty = options.pretty;

  terrain* const map = g->map;
  const int width = terrain_width(map);
  const size_t fired = point_list_get_size(g->bullets);

  point ship = g->ship;
  const size_t x = (size_t) ship.x;
  const size_t y = (size_t) ship.y;
  switch (key) {
    /* Haut. */
    case '8':
    case 'k':
    case 65:
      if (difficulty >= 2 && key != 'k')
        /* vim mode: move with h, j, k, l! */
        break;
      if (y > 0
          && (terrain_get_cell(map, x, y - 1) != CELL_WALL)
          && ((tail && pretty) ? terrain_get_cell(map, x - 1, y - 1) != CELL_WALL : true))
        ship.y--;
      break;
    /* Bas. */
    case '2':
    case 'j':
    case 66:
      /* vim mode: move with h, j, k, l! */
      if (difficulty >= 2 && key != 'j')
        break;
      if ((int) y < (height - 1)
          && (terrain_get_cell(map, x, y + 1) != CELL_WALL)
          && ((tail && pretty) ? terrain_get_cell(map, x - 1, y + 1) != CELL_WALL : true))
        ship.y++;
      break;
    /* Gauche. */
    case '4':
    case 'h':
    case 68:
      /* vim mode: move with h, j, k, l! */
      if (difficulty >= 2 && key != 'h')
        break;
      if (x >= ((tail && pretty) ? 2 : 1)
          && terrain_get_cell(map, x - 1, y) != CELL_WALL
              && ((tail && pretty) ? terrain_get_cell(map, x - 2, y) != CELL_WALL : true))
        ship.x--;

      if (ship.x <= 0 && g->options.difficulty <= 0) {
        /* Si on a atteint le bord gauche. */
        terrain_left(map);
        ship.x++;
        point_list_shift_right(g->bullets);
      }
      break;
    /* Droite. */
    case '6':
    case 'l':
    case 67:
      /* vim mode: move with h, j, k, l! */
      if (difficulty >= 2 && key != 'l')
        break;
      if ((int) x < width
          && terrain_get_cell(map, x + 1, y) != CELL_WALL)
        ship.x++;

      if (ship.x >= width - 1) {
        /* Si on a atteint le bord droit. */
        terrain_right(map);
        ship.x--;
        point_list_shift_left(g->bullets);
      }
      break;
    /* Pause. */
    case 'p':
    case 127:
      game_pause(g, 1);
      break;
    /* Space. */
    case ' ':
      if (fired < g->bullet_max) {
        point p = (point) { .x = ship.x + 1, .y = ship.y };
        if (!point_list_contains(g->bullets, p)) {
          g->bullets = point_list_push_back(g->bullets, p);
          game_add_bonus(g, -20 * (intmax_t) ((fired + 1) * (fired + 1)));
        }
      }
      break;
    default:
      break;
  }

  g->map = map;
  g->ship = ship;
  g->last_key = key;
  game_check_special_cells(g);
}


////////////////////////////////////////////////////////////////////////////////
// local functions definitions
////////////////////////////////////////////////////////////////////////////////

void game_add_bonus(game* const g, const intmax_t bonus) {
  g->bonus += bonus;
}

void game_lose_life(game* const g) {
  g->lives--;
}

void game_add_life(game* const g) {
  g->lives++;
}

void game_set_lives(game* const g, const intmax_t l) {
  g->lives = l;
}

intmax_t game_get_lives(const game* g) {
  return g->lives;
}

intmax_t game_get_bonus(const game* const g) {
  return g->bonus;
}

void game_move_bullets(game* const g) {
  point_list_shift_right(g->bullets);
}

void game_shift_right(game* const g) {
  terrain_right(g->map);
}

void game_check_special_cells(game* const g) {
  const spaceship_options options = g->options;
  const point ship = g->ship;
  const bool tail = options.tail;
  const bool pretty = options.pretty;
  column* const ch = game_get_ship_column(g);


  cell head_cell = column_get_cell(ch, (size_t) ship.y);
  if (head_cell == CELL_SECRET) {
    const int selector = (int) random() % 100;
    if (selector < 20)
      head_cell = CELL_AMMO;
    else if (selector < 40)
      head_cell = CELL_BONUS;
    else if (selector < 60)
      head_cell = CELL_MALUS;
    else if (selector < 70)
      head_cell = CELL_LASER;
    else if (selector < 85)
      head_cell = CELL_LIFE;
    else
      head_cell = CELL_EMPTY;
  }

  if (head_cell == CELL_AMMO) {
    g->bullet_max += g->bullet_max < 10 ? 1 : 0;
    column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
  }
  else if (head_cell == CELL_BONUS) {
    game_add_bonus(g, options.bonus);
    column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
  }
  else if (head_cell == CELL_MALUS) {
    game_add_bonus(g, options.malus);
    column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
  }
  else if (head_cell == CELL_LASER) {
    game_set_laser(g, true);
    column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
  }
  else if (head_cell == CELL_LIFE) {
    game_add_life(g);
    column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
  }

  if(tail && pretty) {
    column_list* columns = terrain_get_columns(g->map);
    column* const ct = column_list_get_column(columns, (size_t) ship.x - 1);
    cell tail_cell = column_get_cell(ct, (size_t) ship.y);

    if (tail_cell == CELL_SECRET) {
      const int selector = (int) random() % 100;
      if (selector < 20)
        tail_cell = CELL_AMMO;
      else if (selector < 40)
        tail_cell = CELL_BONUS;
      else if (selector < 60)
        tail_cell = CELL_MALUS;
      else if (selector < 65)
        tail_cell = CELL_LASER;
      else if (selector < 80)
        head_cell = CELL_LIFE;
      else
        tail_cell = CELL_EMPTY;
    }

    if (tail_cell == CELL_AMMO) {
      g->bullet_max += g->bullet_max < 10 ? 1 : 0;
      column_set_cell(ct, (size_t) ship.y, CELL_EMPTY);
    }
    else if (tail_cell == CELL_BONUS) {
      game_add_bonus(g, options.bonus);
      column_set_cell(ct, (size_t) ship.y, CELL_EMPTY);
    }
    else if (tail_cell == CELL_MALUS) {
      game_add_bonus(g, options.malus);
      column_set_cell(ct, (size_t) ship.y, CELL_EMPTY);
    }
    else if (tail_cell == CELL_LASER) {
      game_set_laser(g, true);
      column_set_cell(ch, (size_t) ship.y, CELL_EMPTY);
    }
    else if (tail_cell == CELL_LIFE) {
      game_add_life(g);
      column_set_cell(ct, (size_t) ship.y, CELL_EMPTY);
    }
  }
}

column* game_get_ship_column(const game* g) {
  terrain* const map = g->map;
  const point ship = g->ship;
  const column_list* columns = terrain_get_columns(map);
  column* const c = column_list_get_column(columns, (size_t) ship.x);
  return c;
}

void game_fall(game* const g) {
  terrain_fall(g->map);
}

void game_check_bullets(game* const g) {
  const spaceship_options options = g->options;
  const point up_left = { .x = 0, .y = 0, };
  const point bottom_right = { .x = options.width, .y = options.height, };

  const column_list* map = terrain_get_columns(g->map);

  g->bullets = point_list_prune_out_of_bounds(g->bullets, up_left, bottom_right);
  const size_t count = point_list_get_size(g->bullets);
  for (size_t i = 0; i < count; ++i) {
    const point position = point_list_get_point(g->bullets, i);
    if (point_is_valid(position)) {
      column* const c = column_list_get_column(map, (size_t) position.x);
      if (c && column_get_cell(c, (size_t) position.y) != CELL_EMPTY) {
        column_set_cell(c, (size_t) position.y, CELL_EMPTY);
        point_list_set_point(g->bullets, i, point_invalid());
      }
    }
  }
  g->bullets = point_list_prune_out_of_bounds(g->bullets, up_left, bottom_right);
}

bool game_check_hit(const game* const g) {
  bool shot_itself = false;
  const point ship = g->ship;
  const bool tail = g->options.tail;
  const bool friendly = g->options.friendly;
  cell tail_cell = CELL_EMPTY;
  column_list* columns = terrain_get_columns(g->map);
  point_list* b = game_get_bullets(g);

  const column* const c = game_get_ship_column(g);
  const cell head_cell = column_get_cell(c, (size_t) ship.y);

  if(g->options.pretty) {
    if(tail) {
      const column* const ct = column_list_get_column(columns, (size_t) ship.x - 1);
      tail_cell = column_get_cell(ct, (size_t) ship.y);
    }
    if(friendly) {
      shot_itself = point_list_contains(b, ship);
      if(tail)
        shot_itself |= point_list_contains(b, (point) { ship.x-1, ship.y });
    }
  } else {
    if(friendly)
      shot_itself = point_list_contains(b, ship);
  }
  return ((head_cell == CELL_WALL || tail_cell == CELL_WALL || shot_itself) && g->lives > 0);
}

void clear_terrain(game* const g) {
  column_list* columns = terrain_get_columns(g->map);
  size_t size = column_list_get_size(columns);
  for(size_t i = 0; i < size; ++i) {
    for(size_t j = 0; j < (size_t) g->options.height; ++j) {
      column_set_cell(column_list_get_column(columns,i), j, CELL_EMPTY);
    }
  }
}

void game_was_hit(game* const g){
  point_list* b = game_get_bullets(g);
  if(g->lives > 1) {
   clear_terrain(g);
   game_add_bonus(g,-100);
  }
  for(size_t i = 0; i < point_list_get_size(b); ++i) {
   point_list_set_point(b, i, point_invalid());
  }
  game_lose_life(g);
}

void game_pause(game* const g, int p) {
  g->pause = p;
}

bool game_is_paused(const game* const g) {
  return (g->pause != 0);
}

int game_pause_menu(const game* const g) {
  return g->pause;
}

void game_reset_bonus(game* const g) {
  g->bonus = 0;
}


void game_reset_ship(game* const g) {
  terrain* map = game_get_map(g);
  point ship = terrain_start_point(map);
  g->ship = ship;
}

void game_reset_ammo(game* const g) {
  const int ammo = g->options.ammo;
  const int difficulty = g->options.difficulty;
  if (ammo > 0)
    g->bullet_max = (size_t) ammo;
  else
    g->bullet_max = difficulty < 3 ? 5 - (size_t) difficulty : 1;
}

void game_change_friendly(game* const g) {
  g->options.friendly = (g->options.friendly == true) ? false : true;
}

void game_change_tail(game* const g) {
  g->options.tail = (g->options.tail == true) ? false : true;
}
