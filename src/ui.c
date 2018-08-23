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
#include "ui.h"

#include <stdio.h>
#include <ncurses.h>
#include <time.h>
#include <sysexits.h>

////////////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////////////

struct interface
{
  WINDOW* game_window;
  WINDOW* debug_window;
  WINDOW* infos_window;
  WINDOW* pause_window;
  WINDOW* title_window;
  int quit;
};

////////////////////////////////////////////////////////////////////////////////
// file-scope variables
////////////////////////////////////////////////////////////////////////////////

static const char* cell_strings[] =
{
  [CELL_EMPTY] = " ",
  [CELL_WALL] = "0",
  [CELL_AMMO] = "O",
  [CELL_BONUS] = "O",
  [CELL_MALUS] = "O",
  [CELL_SECRET] = "O",
  [CELL_LIFE] = "+",
  [CELL_LASER] = "l",
  [CELL_UNKNOWN] = "?",
};

////////////////////////////////////////////////////////////////////////////////
// local functions declarations
////////////////////////////////////////////////////////////////////////////////

static inline chtype _cell_chtype(cell c);
static inline void _cell_wprint(WINDOW* window, cell c, bool pretty);
static inline double _threshold(spaceship_options options, double d);
static inline double _time_difference(struct timespec t0, struct timespec t1);
static void _display_game(WINDOW* window, const game* g);
static void _display_debug(WINDOW* window, const game* g);
static void _display_infos(WINDOW* window, const game* g);
static void _display_pause(WINDOW* window, const game* g);
static void _display_pause_options(WINDOW* window, const game* g);
static void _display_title(WINDOW* window, const game* g);

////////////////////////////////////////////////////////////////////////////////
// init./destroy etc.
////////////////////////////////////////////////////////////////////////////////

interface* interface_init(const spaceship_options o)
{
  interface* ui = malloc(sizeof * ui);
  if (!ui)
  {
    perror("malloc");
    exit(EX_OSERR);
  }

  initscr();
  cbreak();
  noecho();
  curs_set(0);
  if (has_colors())
  {
    start_color();

    short fg, bg;
    pair_content(0, &fg, &bg);

    #ifdef NCURSES_VERSION
      bg = use_default_colors() == OK ? -1 : bg;
    #endif

    init_pair(1, COLOR_RED, bg);
    init_pair(2, COLOR_GREEN, bg);
    init_pair(3, COLOR_YELLOW, bg);
    init_pair(4, COLOR_BLUE, bg);
    init_pair(5, COLOR_MAGENTA, bg);
    init_pair(6, COLOR_CYAN, bg);
  }

  int x, y;
  getmaxyx(stdscr, y, x);

  /*
   * Game window dimensions:
   * - add 2 to the height and width because we display a border around the map
   * - add 2 or 6 in debug mode because we display coordinates
   */
  const int game_height = o.height + 2 + (o.debug ? 2 : 0);
  const int game_width = o.width + 2 + (o.debug ? 6 : 0);
  const int game_y = y > game_height ? (y - game_height) / 2 : 0;
  const int game_x = x > game_width ? (x - game_width) / 2 : 0;
  WINDOW* const game_window = newwin(game_height, game_width, game_y, game_x);
  if (!game_window)
  {
    fprintf(stderr, "newwin: could not create game_window.\n");
    exit(EX_SOFTWARE);
  }
  nodelay(game_window, true);

  const int title_height = 2;
  const int title_width = 24;
  const int title_y = game_y - title_height;
  const int title_x = (x - title_width) / 2;
  WINDOW* const title_window = newwin(title_height, title_width, title_y, title_x);
  if (!title_window)
  {
    fprintf(stderr, "newwin: could not create title_window.\n");
    exit(EX_SOFTWARE);
  }


  /*
   * Don't forget to increase this height if you wish to display more
   * information!
   */
  const int infos_height = 18;
  const int infos_y = game_y;
  const int infos_x = game_x + game_width;
  WINDOW* const infos_window = newwin(infos_height, 0, infos_y, infos_x);
  if (!infos_window)
  {
    fprintf(stderr, "newwin: could not create infos_window.\n");
    exit(EX_SOFTWARE);
  }
  /*
   * 0, 0 for height/width: this window takes the remaining space starting
   * from the point (infos_height, game_width)
   */
  const int debug_y = infos_y + infos_height;
  const int debug_x = infos_x;
  WINDOW* const debug_window = newwin(0, 0, debug_y, debug_x);
  if (!debug_window)
  {
    fprintf(stderr, "newwin: could not create debug_window.\n");
    exit(EX_SOFTWARE);
  }



  const int pause_height = 7;
  const int pause_width = 12;
  const int pause_y = game_y;
  const int pause_x = game_x - pause_width;
  WINDOW* const pause_window = newwin(pause_height, pause_width, pause_y, pause_x);
  if (!pause_window)
  {
    fprintf(stderr, "newwin: could not create pause_window.\n");
    exit(EX_SOFTWARE);
  }

  ui->game_window = game_window;
  ui->debug_window = debug_window;
  ui->infos_window = infos_window;
  ui->pause_window = pause_window;
  ui->title_window = title_window;
  ui->quit = 0;

  return ui;
}

void interface_destroy(interface* const ui)
{
  if (!ui)
    return;

  if (ui->game_window)
  {
    nodelay(ui->game_window, false);
    if(ui->quit != 1) {
      while (wgetch(ui->game_window) != 'q')
      {
        /* Do nothing! We just wait for the user to quit the program. */
      }
    }
    delwin(ui->game_window);
  }

  if (ui->debug_window)
    delwin(ui->debug_window);
  if (ui->infos_window)
    delwin(ui->infos_window);
  if (ui->pause_window)
    delwin(ui->pause_window);
  if (ui->title_window)
    delwin(ui->title_window);
  endwin();

  free(ui);
}

////////////////////////////////////////////////////////////////////////////////
// misc.
////////////////////////////////////////////////////////////////////////////////

void interface_display(interface* const ui, const game* const g)
{
  _display_game(ui->game_window, g);
  _display_infos(ui->infos_window, g);
  _display_debug(ui->debug_window, g);
  _display_pause(ui->pause_window, g);
  _display_pause_options(ui->pause_window, g);
  _display_title(ui->title_window, g);
}



void interface_game_loop(interface* const ui, game* const g)
{
  /*
   * Disclaimer: don't do it this way in the real world!
   * Frames should be refreshed at a constant rate...
   *
   * Use timers, signals or even a dedicated game library!
   */
  spaceship_options options = game_get_options(g);
  const bool old_still_value = options.still;
  const intmax_t laserTime = options.laserTime;
  struct timespec start, current, last;
  int tmp = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  last = start;
  bool debutLaser = false;

  const double constant_delay = game_get_constant_delay(g);
  while (1)
  {
    clock_gettime(CLOCK_MONOTONIC, &current);

    int c = wgetch(ui->game_window);
    if (c == 'q')
      break;

    const double elapsed = _time_difference(start, current);
    const double delay = constant_delay > 0.0 ? constant_delay : _threshold(options, elapsed);
    const double d = _time_difference(last, current);

    game_set_delay(g, delay);
    game_set_elapsed_time(g, elapsed);
    if ((c != ERR) && (!game_is_paused(g)))
    {
      game_process_input(g, c);
      interface_display(ui, g);
      wrefresh(ui->game_window);
    }

    if(game_check_hit(g)) {
      options.still = true;
      game_was_hit(g);
      if(game_get_lives(g) == 0) {
        interface_display(ui, g);
      } else {
        struct timespec st, cu;
        clock_gettime(CLOCK_MONOTONIC, &st);
        cu = st;
        double e = _time_difference(st, cu);
        while(e < 1) {
          clock_gettime(CLOCK_MONOTONIC, &cu);
          e = _time_difference(st, cu);
        }
      }
      options.still = (old_still_value == true) ? true : false;
    }

    bool laser = game_get_laser(g);
    if(laser && game_pause_menu(g) == 0) {
      struct timespec stLa, cuLa;
      if(!debutLaser) {
        clock_gettime(CLOCK_MONOTONIC, &stLa);
        debutLaser = true;
      }
      clock_gettime(CLOCK_MONOTONIC, &cuLa);
      double eL = _time_difference(stLa, cuLa);
      if(eL > laserTime) {
        game_set_laser(g, false);
        debutLaser = false;
      }
    }


    if (((options.still && c == 's') || (!options.still && d > delay)) && (!game_is_paused(g)))
    {
      game_compute_turn(g);
      interface_display(ui, g);
      last = current;
    }

    if (game_pause_menu(g) > 0)
    {
    struct timespec startPause, currentPause;
      if(tmp == 0) {
        tmp = 1;
        clock_gettime(CLOCK_MONOTONIC, &startPause);
        currentPause = startPause;
      }
      interface_game_pause(ui, g);
      if (c != 10)
      {
        options.still = true;
        _display_pause(ui->pause_window, g);
        game_pause_process_input(g, c);
      } else {
        if(game_pause_menu(g) == 1) {
          clock_gettime(CLOCK_MONOTONIC, &currentPause);
          double e1 = _time_difference(startPause, currentPause);
          start.tv_sec += (long int) e1;
          start.tv_nsec += (long int) (e1);
          tmp = 0;
        } else if(game_pause_menu(g) == 2) {
            clear_terrain(g);
            point_list* b = game_get_bullets(g);
            game_set_lives(g, options.lives);
            game_reset_bonus(g);
            clock_gettime(CLOCK_MONOTONIC, &start);
            current = start;
            last = start;
            game_set_delay(g, 0);
            game_set_elapsed_time(g, 0);
            game_reset_ammo(g);
            for(size_t i = 0; i < point_list_get_size(b); ++i) {
              point_list_set_point(b, i, point_invalid());
            }
            game_reset_ship(g);
        } else if(game_pause_menu(g) == 3) {
          game_pause(g, 11);
          c = 1;
          while (c != 10 || ((game_pause_menu(g) != 13) && (game_pause_menu(g) != 14)))
          {
              _display_pause_options(ui->pause_window, g);
              game_pause_process_input(g, c);
              c = wgetch(ui->game_window);
              if(game_pause_menu(g) == 11 && c == 10) {
                game_change_friendly(g);
              } else if(game_pause_menu(g) == 12 && c == 10) {
                game_change_tail(g);
              }
          }
          clock_gettime(CLOCK_MONOTONIC, &currentPause);
          double e1 = _time_difference(startPause, currentPause);
          start.tv_sec += (long int) e1;
          start.tv_nsec += (long int) (e1);
          tmp = 0;
          if(game_pause_menu(g) == 13) {
            /* Back to the game */
          } else if(game_pause_menu(g) == 14) {
            ui->quit = 1;
            break;
          }
        } else {
          ui->quit = 1;
          break;
        }
        options.still = (old_still_value == true) ? true : false;
        _display_game(ui->game_window, g);
        _display_infos(ui->infos_window, g);
        game_pause(g, 0);
        werase(ui->pause_window);
        wrefresh(ui->pause_window);
      }
    }

    if (!game_ship_is_alive(g))
    {
      interface_game_over(ui, options);
      break;
    }
  }
}

void interface_game_over(interface* const ui, const spaceship_options o)
{
  const int x = (o.width + 2 - 11) / 2 + 1 + (o.debug ? 3 : 0);
  const int y = (o.height + 2) / 2 + (o.debug ? 1 : 0);

  wattron(ui->game_window, A_REVERSE | A_BOLD | A_BLINK | COLOR_PAIR(1));
  wmove(ui->game_window, y, x);
  wprintw(ui->game_window, " GAME OVER ");
  wattroff(ui->game_window, A_REVERSE | A_BOLD | A_BLINK | COLOR_PAIR(1));
  wrefresh(ui->game_window);
}

void interface_game_pause(interface* const ui, game* const g) {
  spaceship_options o = game_get_options(g);
  const int x = (o.width + 2 - 13) / 2 + 1 + (o.debug ? 3 : 0);
  const int y = (o.height + 2) / 2 + (o.debug ? 1 : 0);

  wattron(ui->game_window, A_REVERSE | A_BOLD | COLOR_PAIR(4));
  wmove(ui->game_window, y, x);
  wprintw(ui->game_window, " GAME PAUSED ");
  wattroff(ui->game_window, A_REVERSE | A_BOLD | COLOR_PAIR(4));
  wrefresh(ui->game_window);
}

////////////////////////////////////////////////////////////////////////////////
// local functions definitions
////////////////////////////////////////////////////////////////////////////////

static inline chtype _cell_chtype(const cell c)
{
  switch (c)
  {
    case CELL_EMPTY:
      return ' ';
    case CELL_WALL:
      return ACS_BOARD;
    case CELL_AMMO:
      return ACS_DIAMOND;
    case CELL_BONUS:
      return ACS_DIAMOND;
    case CELL_MALUS:
      return ACS_DIAMOND;
    case CELL_SECRET:
      return ACS_DIAMOND;
    case CELL_LIFE:
      return '+';
    case CELL_LASER:
      return 'l';
    case CELL_UNKNOWN:
    default:
      return ACS_VLINE;
  }
}

void _cell_wprint(WINDOW* const window, const cell c, const bool pretty)
{
  cell target = c <= CELL_UNKNOWN ? c : CELL_UNKNOWN;
  const char* const string = cell_strings[target];

  if (c == CELL_AMMO)
    wattron(window, A_DIM);
  if (c == CELL_BONUS)
    wattron(window, COLOR_PAIR(2));
  if (c == CELL_MALUS)
    wattron(window, COLOR_PAIR(1));
  if (c == CELL_SECRET)
    wattron(window, COLOR_PAIR(5));
  if (c == CELL_LIFE)
    wattron(window, A_BOLD | COLOR_PAIR(1));
  if (c == CELL_LASER)
    wattron(window, A_BOLD | COLOR_PAIR(1));

  if (pretty)
    waddch(window, _cell_chtype(c));
  else
    wprintw(window, "%s", string);

  if (c == CELL_AMMO)
    wattroff(window, A_DIM);
  if (c == CELL_BONUS)
    wattroff(window, COLOR_PAIR(2));
  if (c == CELL_MALUS)
    wattroff(window, COLOR_PAIR(1));
  if (c == CELL_SECRET)
    wattroff(window, COLOR_PAIR(5));
  if (c == CELL_LIFE)
    wattroff(window, A_BOLD | COLOR_PAIR(1));
  if (c == CELL_LASER)
    wattroff(window, A_BOLD | COLOR_PAIR(1));
}

double _threshold(const spaceship_options options, const double d)
{
  const int difficulty = options.difficulty;

  switch (difficulty)
  {
    case 0:
      return d > 59.0 ? 0.3 : (1.0 - d / 60.0);
      break;
    case 1:
      return d > 30.0 ? 0.25 : (1.0 - d / 45.0);
      break;
    default:
      return d > 14.0 ? 0.15 : (1.0 - d / 15.0);
      break;
  }
}

double _time_difference(const struct timespec t0, const struct timespec t1)
{
  double difference = difftime(t1.tv_sec, t0.tv_sec);
  if (t1.tv_nsec < t0.tv_nsec)
  {
    long x = 1000000000l - t0.tv_nsec + t1.tv_nsec;
    difference += (double) x / 1e9 - 1.;
  }
  else
  {
    long x = t1.tv_nsec - t0.tv_nsec;
    difference += (double) x / 1e9;
  }

  return difference;
}

void _display_game(WINDOW* const window, const game* const g)
{
  const spaceship_options options = game_get_options(g);
  const bool debug = options.debug;
  const bool pretty = options.pretty;
  const terrain* const map = game_get_map(g);
  const int height = terrain_height(map);
  const int width = terrain_width(map);
  bool laser = game_get_laser(g);

  const int shift_x = debug ? 4 : 1;
  const int shift_y = 1 + (debug ? 1 : 0);

  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Map border. */
  wattron(window, A_DIM | COLOR_PAIR(3));
  wborder(
      window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER,
      ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
  wattroff(window, A_DIM | COLOR_PAIR(3));

  /* Map. */
  for (int l = 0; l < height; ++l)
  {
    wmove(window, l + shift_y, shift_x);
    for(size_t c = 0; c < (size_t) width; ++c)
    {
      const cell current_cell = terrain_get_cell(map, c, (size_t) l);
      _cell_wprint(window, current_cell, pretty);
    }
  }

  /* Ship. */
  const point ship = game_get_ship_position(g);
  const point_list* b = game_get_bullets(g);
  const cell ship_cell = terrain_get_cell(map, (size_t) ship.x, (size_t) ship.y);
  bool ship_dead = ship_cell == CELL_WALL;
  ship_dead |= ((options.friendly) ? point_list_contains(b, ship) : false);
  ship_dead |= ((options.friendly && options.tail && options.pretty) ? point_list_contains(b, (point) { ship.x-1, ship.y }) : false);
  ship_dead |= !game_get_lives(g);
  wmove(window, ship.y + shift_y, ship.x + shift_x);
  wattron(window, A_BOLD | COLOR_PAIR(ship_dead ? 1 : 4));
  if (ship_dead)
    wattron(window, A_BLINK);
  if(game_check_hit(g)) {
    wattron(window, A_BOLD | COLOR_PAIR(1));
  }
  if (pretty)
  {
    waddch(window, ACS_RTEE);
    if (ship.x > 0)
    {
      const cell tail_cell =
          terrain_get_cell(map, (size_t) ship.x - 1, (size_t) ship.y);
      if (tail_cell == CELL_EMPTY)
      {
        wmove(window, ship.y + shift_y, ship.x + shift_x - 1);
        waddch(window, ACS_HLINE);
      }
    }
  }
  else
    wprintw(window, "%s", ship_dead ? "X" : ">");
  wattroff(window, A_BOLD | COLOR_PAIR(ship_dead ? 1 : 4));
  if (ship_dead)
    wattroff(window, A_BLINK);

  if(game_check_hit(g)) {
    wrefresh(window);
    wattroff(window, A_BOLD | COLOR_PAIR(1));
  }

  /* Laser. */
  if(laser) {
    wmove(window, ship.y + shift_y, ship.x + 1 + shift_x);
    wattron(window, A_BOLD | COLOR_PAIR(3));
    for(int m = ship.x + 1 + shift_x; m <= width; m++) {
      column_list* columns = terrain_get_columns(map);
      column* const col = column_list_get_column(columns, (size_t) m-1);
      column_set_cell(col, (size_t) ship.y, CELL_EMPTY);
      waddch(window, ACS_HLINE);
    }
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  }

  /* Bullets. */
  const point_list* const bullets = game_get_bullets(g);
  const size_t count = point_list_get_size(bullets);
  for (size_t i = 0; i < count; ++i)
  {
    const point bullet = point_list_get_point(bullets, i);
    if (point_is_valid(bullet))
    {
      wmove(window, bullet.y + 1 + (debug ? 1 : 0), bullet.x + (debug ? 4 : 1));
      wattron(window, A_BOLD | COLOR_PAIR(3));
      if (pretty)
        waddch(window, ACS_DIAMOND);
      else
        wprintw(window, "*");
      wattroff(window, A_BOLD | COLOR_PAIR(3));
    }
  }

  /* Display coordinates around the map in debug mode. */
  if (debug)
  {
    wattron(window, A_DIM);

    /* Top/bottom coordinates. */
    for(size_t c = 0; c < (size_t) width; ++c)
    {
      const bool is_ten = !(c % 10);
      if (is_ten)
        wattron(window, A_BOLD);
      wmove(window, 1, 4 + (int) c);
      wprintw(window, "%zu", c % 10);
      wmove(window, 2 + height, 4 + (int) c);
      wprintw(window, "%zu", c % 10);
      if (is_ten)
        wattroff(window, A_BOLD);
    }
    /* Left/right coordinates. */
    for (int l = 0; l < height; ++l)
    {
      wmove(window, l + 1 + (debug ? 1 : 0), 1);
      wprintw(window, "%2d ", l);
      wmove(window, l + 1 + (debug ? 1 : 0), width + 4);
      wprintw(window, "%2d ", l);
    }

    wattroff(window, A_DIM);
  }

  /* LAST STEP: refresh the window. */
  wrefresh(window);
}

void _display_debug(WINDOW* const window, const game* const g)
{
  const spaceship_options options = game_get_options(g);
  if (!options.debug)
    return;

  const point ship = game_get_ship_position(g);
  const double delay = game_get_delay(g);
  const int last_input = game_get_last_input(g);
  const size_t max_ammo = game_get_max_ammo(g);
  const size_t fired = game_get_fired_bullets(g);
  const point_list* bullets = game_get_bullets(g);
  const intmax_t bonus = options.bonus;
  const intmax_t malus = options.malus;
  const intmax_t lives = game_get_lives(g);
  const intmax_t laserTime = options.laserTime;

  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Display the section title. */
  wmove(window, 1, 1);
  wattron(window, A_BOLD | A_REVERSE | A_DIM);
  wprintw(window, " DEBUG ");
  wattroff(window, A_BOLD | A_REVERSE | A_DIM);

  /* Display debugging information. */
  wattron(window, A_DIM);
  wprintw(window, "\n");
  wprintw(window, " - Dimensions: %d x %d\n", options.width, options.height);
  wprintw(window, " - Delay: %lf\n", delay);
  wprintw(window, " - Position: (%d, %d)\n", ship.x, ship.y);
  wprintw(window, " - Difficulty: %d\n", options.difficulty);
  if (last_input)
    wprintw(window, " - Last keystroke: '%c' (%d)\n", last_input, last_input);
  wprintw(window, " - Bonus: %"PRIdMAX"\n", bonus);
  wprintw(window, " - Malus: %"PRIdMAX"\n", malus);
  wprintw(window, " - Lives: %zu\n", lives);
  wprintw(window, " - Laser duration: %zu\n", laserTime);
  wprintw(window, " - Max ammo: %zu\n", max_ammo);
  wprintw(window, " - Fired: %zu\n", fired);
  const size_t count = point_list_get_size(bullets);
  for (size_t i = 0; i < count; ++i)
  {
    const point bullet = point_list_get_point(bullets, i);
    wprintw(window, "     (%d, %d)\n", bullet.x, bullet.y);
  }
  wattroff(window, A_DIM);

  /* LAST STEP: refresh the window. */
  wrefresh(window);
}

void _display_pause(WINDOW* const window, const game* const g)
{
  // const spaceship_options options = game_get_options(g);
  if ((game_pause_menu(g) <= 0) || (game_pause_menu(g) > 4))
    return;

  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Display the section title. */
  wmove(window, 1, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, "  PAUSE  ");
  wattroff(window, A_BOLD | A_REVERSE);
  /* Display pause menu information. */
  int choice = game_pause_menu(g);
  wmove(window, 3, 2);
  if(choice == 1)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "CONTINUE");
  if(choice == 1)
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  wmove(window, 4, 2);
  if(choice == 2)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "RESET");
  if(choice == 2)
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  wmove(window, 5, 2);
  if(choice == 3)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "OPTIONS");
  if(choice == 3)
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  wmove(window, 6, 2);
  if(choice == 4)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "QUIT");
  if(choice == 4)
    wattroff(window, A_BOLD | COLOR_PAIR(3));

  wmove(window, 2 + game_pause_menu(g), 0);
  wattron(window, A_BOLD | COLOR_PAIR(3));
  waddch(window, ACS_DIAMOND);
  wattroff(window, A_BOLD | COLOR_PAIR(3));
  /* LAST STEP: refresh the window. */
  wrefresh(window);
}

void _display_title(WINDOW* const window, const game* const g)
{
  const spaceship_options options = game_get_options(g);
  const bool pretty = options.pretty;
  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Display the section title. */
  wattron(window, A_BOLD | COLOR_PAIR(4));
  if(pretty) {
    wmove(window, 0, 0);
    waddch(window, ACS_HLINE);
    waddch(window, ACS_RTEE);
  } else {
    wmove(window, 0, 1);
    wprintw(window, ">");
  }
  wattroff(window, A_BOLD | COLOR_PAIR(4));
  wmove(window, 0, 3);
  wattron(window, A_BOLD);
  wprintw(window, "SPACESHIP-INFINITY");
  wattroff(window, A_BOLD);
  wattron(window, A_BOLD | COLOR_PAIR(4));
  wmove(window, 0, 22);
  if(pretty) {
    waddch(window, ACS_LTEE);
    waddch(window, ACS_HLINE);
  } else {
    wprintw(window, "<");
  }
  wattroff(window, A_BOLD | COLOR_PAIR(4));
  // wmove(window, 1,5);
  // wprintw(window, "Nicolas Laforêt");

  /* LAST STEP: refresh the window. */
  wrefresh(window);
}

void _display_pause_options(WINDOW* const window, const game* const g)
{
  if (game_pause_menu(g) <= 4)
    return;

  const spaceship_options options = game_get_options(g);
  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Display the section title. */
  wmove(window, 1, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " OPTIONS ");
  wattroff(window, A_BOLD | A_REVERSE);
  /* Display pause menu information. */
  int choice = game_pause_menu(g);
  wmove(window, 3, 2);
  wattron(window, COLOR_PAIR((options.friendly)? 2 : 1));
  if(choice == 11)
    wattron(window, A_BOLD);
  wprintw(window, "FRIENDLY");
  wattroff(window, COLOR_PAIR((options.friendly)? 2 : 1));
  if(choice == 11)
    wattroff(window, A_BOLD);
  wmove(window, 4, 2);
  wattron(window, COLOR_PAIR((options.tail)? 2 : 1));
  if(choice == 12)
    wattron(window, A_BOLD);
  wprintw(window, "TAIL");
  wattroff(window, COLOR_PAIR((options.tail)? 2 : 1));
  if(choice == 12)
    wattroff(window, A_BOLD);
  wmove(window, 5, 2);
  if(choice == 13)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "CONTINUE");
  if(choice == 13)
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  wmove(window, 6, 2);
  if(choice == 14)
    wattron(window, A_BOLD | COLOR_PAIR(3));
  wprintw(window, "QUIT");
  if(choice == 14)
    wattroff(window, A_BOLD | COLOR_PAIR(3));

  wmove(window, game_pause_menu(g) - 8, 0);
  int color = (choice == 11 && options.friendly) ? 2 : 1;
  color = (choice == 12 && options.tail) ? 2 : 1;
  if(choice == 11)
    if(options.friendly)
      color = 2;
    else
      color = 1;
  else if (choice == 12)
    if(options.tail)
      color = 2;
    else
      color = 1;
  else
    color = 3;
  wattron(window, A_BOLD | COLOR_PAIR(color));
  waddch(window, ACS_DIAMOND);
  wattroff(window, A_BOLD | COLOR_PAIR(color));
  /* LAST STEP: refresh the window. */
  wrefresh(window);
}

void _display_infos(WINDOW* const window, const game* const g)
{
  const size_t fired = game_get_fired_bullets(g);
  const size_t max_ammo = game_get_max_ammo(g);
  const intmax_t score = game_get_score(g);
  const double elapsed = game_get_elapsed_time(g);
  const spaceship_options options = game_get_options(g);
  const bool pretty = options.pretty;

  /* FIRST STEP: erase the window to get rid of remnant characters. */
  werase(window);

  /* Display the score. */
  wmove(window, 1, 1);
  if (score < 0)
    wattron(window, COLOR_PAIR(1));
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " SCORE ");
  wattroff(window, A_BOLD | A_REVERSE);
  wmove(window, 2, 2);
  wprintw(window, "%"PRIdMAX, score);
  if (score < 0)
    wattroff(window, COLOR_PAIR(1));

  /* Display the elapsed time. */
  wmove(window, 4, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " TIME ");
  wattroff(window, A_BOLD | A_REVERSE);
  wmove(window, 5, 2);
  wprintw(window, "%lf", elapsed);

  /* Display ammo.*/
  wmove(window, 7, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " AMMO ");
  wattroff(window, A_BOLD | A_REVERSE);
  wmove(window, 8, 2);
  /* Display available bullets.*/
  for (size_t i = 0; i < max_ammo - fired; ++i)
  {
    wattron(window, A_BOLD | COLOR_PAIR(3));
    if (pretty)
      waddch(window, ACS_DIAMOND);
    else
      wprintw(window, "*");
    wprintw(window, " ");
    wattroff(window, A_BOLD | COLOR_PAIR(3));
  }
  /* Display in-flight bullets.*/
  for (size_t i = 0; i < fired; ++i)
  {
    wattron(window, A_DIM);
    if (pretty)
      waddch(window, ACS_DIAMOND);
    else
    wprintw(window, "*");
    wattroff(window, A_DIM);
    wprintw(window, " ");
  }

  /* Display lives.*/
  wmove(window, 10, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " LIVES ");
  wattroff(window, A_BOLD | A_REVERSE);
  wattron(window, A_BOLD | COLOR_PAIR(4));
  if (pretty) {
    wmove(window, 11, 1);
    waddch(window, ACS_HLINE);
    waddch(window, ACS_RTEE);
    wattroff(window, A_BOLD | COLOR_PAIR(4));
    wattron(window, A_BOLD);
    wprintw(window, " x %d", game_get_lives(g));
    wattroff(window, A_BOLD);
    }
  else {
    wmove(window, 11, 2);
    wprintw(window, ">");
    wattroff(window, A_BOLD | COLOR_PAIR(4));
    wattron(window, A_BOLD);
    wprintw(window, " x %d", game_get_lives(g));
    wattroff(window, A_BOLD);
  }
  wprintw(window, " ");
  wattroff(window, A_BOLD | COLOR_PAIR(4));

  wmove(window, 13, 1);
  wattron(window, A_BOLD | A_REVERSE);
  wprintw(window, " CONTROLS ");
  wattroff(window, A_BOLD | A_REVERSE);
  wmove(window, 14, 1);
  wattron(window, A_BOLD);
  wprintw(window, "MOVE  :");
  wattroff(window, A_BOLD);
  wprintw(window, " ARROWS (");
  waddch(window, ACS_UARROW);
  waddch(window, ACS_RARROW);
  waddch(window, ACS_DARROW);
  waddch(window, ACS_LARROW);
  wprintw(window, ")");
  wmove(window, 15, 1);
  wattron(window, A_BOLD);
  wprintw(window, "SHOOT :");
  wattroff(window, A_BOLD);
  wprintw(window, " SPACE");
  wmove(window, 16, 1);
  wattron(window, A_BOLD);
  wprintw(window, "PAUSE :");
  wattroff(window, A_BOLD);
  wprintw(window, " P / BACKSPACE");
  wmove(window, 17, 1);
  wattron(window, A_BOLD);
  wprintw(window, "QUIT  :");
  wattroff(window, A_BOLD);
  wprintw(window, " Q");

  /* LAST STEP: refresh the window. */
  wrefresh(window);
}
