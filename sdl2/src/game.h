/*
 * First of Them - SDL2 Port
 * Game State Header
 *
 * Original: YC Hacks 2014 (Pebble)
 * Port: 2026 (SDL2 + AGINUZ Protection)
 *
 * EXACT PORT - matching original Pebble code line by line
 *
 * Copyright (c) 2014, 2026 Rudy Gomez
 */

#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

/* Pebble screen: 144x168, we scale up 4x */
#define PEBBLE_WIDTH   144
#define PEBBLE_HEIGHT  168
#define SCALE_FACTOR   4
#define SCREEN_WIDTH   (PEBBLE_WIDTH * SCALE_FACTOR)
#define SCREEN_HEIGHT  (PEBBLE_HEIGHT * SCALE_FACTOR)

/* Game constants - EXACT from original */
#define MAX_ZOMBIES     100
#define INITIAL_SPEED   5.0f    /* Original: speed = 5 */
#define MAX_SPEED       15.0f   /* Original: if (speed < 15) */
#define BULLET_SPEED    10.0f   /* Original: bulletSpeed = 10 */
#define TIMER_INTERVAL  50      /* Original: timeout_ms = 50 */

/* Bullseye zone - EXACT from original */
#define MARKER_MIN     -20      /* Original: markerMin = -20 */
#define MARKER_MAX      20      /* Original: markerMax = 20 */

/* Zombie position within layer - EXACT from original */
#define ZOMBIE_DRAW_X   100     /* Original: .origin = { 100, 2} */
#define ZOMBIE_DRAW_Y   2

/* Game over threshold - EXACT from original */
#define GAME_OVER_X    -100     /* Original: if (rect.origin.x < -100) */

/* AGINUZ Protection */
#ifdef AGINUZ_SEED
#define AGINUZ_PROTECTED __attribute__((annotate("aginuz_protect")))
#else
#define AGINUZ_PROTECTED
#endif

/* Zombie - tracks the layer offset like original */
typedef struct {
    float layer_x;      /* Original: rect.origin.x of the layer */
    bool active;
} Zombie;

/* AGINUZ Protected game state */
typedef struct AGINUZ_PROTECTED {
    /* Score - cheaters target these */
    int count;          /* Original: count (kill count) */
    int high_score;     /* Original: persist_read_int(HIGH_SCORE_KEY) */

    /* Marker state - EXACT from original */
    float marker_x;     /* Original: markerPos.x */
    float marker_y;     /* Original: markerPos.y (fixed at 75) */
    float speed;        /* Original: speed */
    bool direction;     /* Original: direction (true = right) */

    /* Bullet state - EXACT from original */
    float bullet_x;     /* Original: bulletPos.x */
    float bullet_y;     /* Original: bulletPos.y */
    bool is_shooting;   /* Original: isShooting */

    /* Game state */
    int zombie_count;   /* Original: zombieCount */
    bool is_game_over;  /* Original: isGameOver */
    bool is_running;
} GameState;

typedef struct {
    GameState state;
    Zombie zombies[MAX_ZOMBIES];
} Game;

/* Game functions */
void game_init(Game* game);
void game_update(Game* game);
void game_shoot(Game* game);
void game_reset(Game* game);
bool game_is_in_bullseye(Game* game);

/* For AGINUZ debugging */
void game_print_layout(const Game* game);
uint32_t game_get_seed(void);
bool game_is_protected(void);

#endif /* GAME_H */
