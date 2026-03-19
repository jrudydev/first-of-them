/*
 * First of Them - SDL2 Port
 * Game Logic Implementation
 *
 * EXACT PORT of original Pebble code.
 * Comments reference original line numbers.
 *
 * Copyright (c) 2014, 2026 Rudy Gomez
 */

#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* AGINUZ seed (set at compile time) */
#ifndef AGINUZ_SEED
#define AGINUZ_SEED 0
#endif

uint32_t game_get_seed(void) {
    return AGINUZ_SEED;
}

bool game_is_protected(void) {
    return AGINUZ_SEED != 0;
}

/*
 * Original init() lines 347-434
 */
void game_init(Game* game) {
    memset(game, 0, sizeof(Game));

    /* Original line 432: markerPos = GPoint (75, 75) */
    game->state.marker_x = 75.0f;
    game->state.marker_y = 75.0f;

    /* Original line 433: bulletPos = GPoint (10, 9) */
    game->state.bullet_x = 10.0f;
    game->state.bullet_y = 9.0f;

    /* Original line 15-18: initial values */
    game->state.direction = true;       /* direction = true */
    game->state.speed = INITIAL_SPEED;  /* speed = 5 */
    game->state.is_shooting = false;    /* isShooting = false */
    game->state.is_game_over = false;   /* isGameOver = false */
    game->state.is_running = true;

    /* Original line 18: count = 0 */
    game->state.count = 0;

    /* Original lines 370-376: add first zombie */
    /* zombie_layer = layer_create(bounds) - layer starts at 0 */
    game->zombies[0].layer_x = 0.0f;
    game->zombies[0].active = true;
    game->state.zombie_count = 1;
}

/*
 * Original select_click_handler() reset section, lines 298-328
 */
void game_reset(Game* game) {
    /* Original line 300: isGameOver = false */
    game->state.is_game_over = false;

    /* Original lines 304-305 */
    game->state.marker_x = 75.0f;
    game->state.marker_y = 75.0f;
    game->state.bullet_x = 10.0f;
    game->state.bullet_y = 9.0f;

    /* Original lines 307-310 */
    game->state.direction = true;
    game->state.speed = INITIAL_SPEED;
    game->state.count = 0;

    /* Original lines 316-319: clear zombie array */
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        game->zombies[i].active = false;
    }

    /* Original lines 321-327: add first zombie */
    game->state.zombie_count = 0;
    game->zombies[0].layer_x = 0.0f;
    game->zombies[0].active = true;
    game->state.zombie_count = 1;

    game->state.is_shooting = false;
}

/*
 * Original: check if marker is in bullseye zone
 * From select_click_handler() line 290
 */
bool game_is_in_bullseye(Game* game) {
    /* Original: center = grect_center_point(&bounds)
     * Pebble screen is 144x168, so center.x = 72
     */
    float center_x = PEBBLE_WIDTH / 2.0f;  /* 72 */

    /* Original line 290:
     * if (markerPos.x >= center.x + markerMin && markerPos.x <= center.x + markerMax)
     * markerMin = -20, markerMax = 20
     * So: 52 <= markerPos.x <= 92
     */
    return (game->state.marker_x >= center_x + MARKER_MIN &&
            game->state.marker_x <= center_x + MARKER_MAX);
}

/*
 * Original select_click_handler() lines 280-331
 */
void game_shoot(Game* game) {
    if (game->state.is_game_over) {
        /* Original lines 298-328: reset game on press after game over */
        game_reset(game);
        return;
    }

    /* Original lines 290-295 */
    if (game_is_in_bullseye(game)) {
        /* Original line 292: bulletPos = GPoint (10, 9) */
        game->state.bullet_x = 10.0f;
        game->state.bullet_y = 9.0f;
        /* Original line 293: vibes_short_pulse() - we skip vibration */
        /* Original line 294: isShooting = true */
        game->state.is_shooting = true;
    }
}

/*
 * Spawn zombie - from update_marker_layer() lines 177-183 and 190-196
 */
static void spawn_zombie(Game* game) {
    if (game->state.zombie_count >= MAX_ZOMBIES) return;

    /* Original: zombie_layer = layer_create(bounds)
     * New layer starts at position 0
     */
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!game->zombies[i].active) {
            game->zombies[i].layer_x = 0.0f;
            game->zombies[i].active = true;
            game->state.zombie_count++;
            break;
        }
    }
}

/*
 * This combines the update logic from:
 * - update_marker_layer() lines 162-211
 * - update_bullet_layer() lines 213-265
 * - zombie_layer_update_callback() lines 107-134
 */
void game_update(Game* game) {
    if (game->state.is_game_over) return;

    /*
     * UPDATE MARKER - from update_marker_layer() lines 162-211
     */

    /* Original line 168: int posX = (direction) ? markerPos.x + speed : markerPos.x - speed */
    float posX;
    if (game->state.direction) {
        posX = game->state.marker_x + game->state.speed;
    } else {
        posX = game->state.marker_x - game->state.speed;
    }

    /* Original lines 172-184: bounce off left edge, spawn zombie */
    if (posX < 0) {
        posX = 0;
        game->state.direction = true;
        spawn_zombie(game);
    }
    /* Original lines 185-197: bounce off right edge, spawn zombie */
    else if (posX >= PEBBLE_WIDTH) {  /* Original: >= 144 */
        posX = PEBBLE_WIDTH;
        game->state.direction = false;
        spawn_zombie(game);
    }

    /* Original line 200: markerPos = GPoint (posX, markerPos.y) */
    game->state.marker_x = posX;

    /* Original lines 204-205: if (speed < 15) speed += 0.01 */
    if (game->state.speed < MAX_SPEED) {
        game->state.speed += 0.01f;
    }

    /*
     * UPDATE BULLET - from update_bullet_layer() lines 213-265
     */

    /* Original line 216: if (isShooting) */
    if (game->state.is_shooting) {
        /* Original line 218: int posX = bulletPos.x + bulletSpeed */
        game->state.bullet_x += BULLET_SPEED;

        /* Original lines 224-251: check for zombie hit
         * In original, arr[0] is always the CLOSEST zombie (lowest layer_x)
         * We need to find the zombie with the lowest layer_x (closest to player)
         */
        int closest_idx = -1;
        float closest_layer_x = 999999.0f;

        for (int i = 0; i < MAX_ZOMBIES; i++) {
            if (game->zombies[i].active) {
                if (game->zombies[i].layer_x < closest_layer_x) {
                    closest_layer_x = game->zombies[i].layer_x;
                    closest_idx = i;
                }
            }
        }

        if (closest_idx >= 0) {
            /* Zombie screen X = layer_x + 100 (ZOMBIE_DRAW_X)
             * Original line 230: if (posX > center.x)
             * center is the zombie's position
             */
            float zombie_screen_x = game->zombies[closest_idx].layer_x + ZOMBIE_DRAW_X;

            if (game->state.bullet_x > zombie_screen_x) {
                /* Original lines 232-234: kill zombie */
                game->state.is_shooting = false;
                game->state.bullet_x = 10.0f;

                /* Original lines 236-244: remove zombie */
                game->zombies[closest_idx].active = false;
                game->state.zombie_count--;

                /* Original line 246: count++ */
                game->state.count++;

                /* Update high score */
                if (game->state.count > game->state.high_score) {
                    game->state.high_score = game->state.count;
                }
            }
        }

        /* Bullet went off screen */
        if (game->state.bullet_x > PEBBLE_WIDTH) {
            game->state.bullet_x = 10.0f;
            game->state.is_shooting = false;
        }
    }

    /*
     * UPDATE ZOMBIES - from zombie_layer_update_callback() lines 107-134
     * Each zombie's layer moves left by 1 pixel per frame
     */

    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (game->zombies[i].active) {
            /* Original line 114: rect.origin.x = rect.origin.x - 1 */
            game->zombies[i].layer_x -= 1.0f;
        }
    }

    /* Check game over - find closest zombie and check if past threshold
     * Original lines 116-125: if (rect.origin.x < -100) isGameOver = true
     * This happens when zombie screen position (layer_x + 100) < 0
     */
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (game->zombies[i].active) {
            if (game->zombies[i].layer_x < GAME_OVER_X) {
                game->state.is_game_over = true;
                break;
            }
        }
    }
}

void game_print_layout(const Game* game) {
    printf("\n");
    printf("========================================\n");
    printf("AGINUZ Memory Layout Debug\n");
    printf("========================================\n");
    printf("Build Seed: %u\n", game_get_seed());
    printf("Protected:  %s\n", game_is_protected() ? "YES" : "NO");
    printf("----------------------------------------\n");
    printf("GameState Address: %p\n", (void*)&game->state);
    printf("GameState Size:    %zu bytes\n", sizeof(GameState));
    printf("----------------------------------------\n");
    printf("Field Offsets (what cheaters look for):\n");
    printf("  count:          offset=%zu\n", offsetof(GameState, count));
    printf("  high_score:     offset=%zu\n", offsetof(GameState, high_score));
    printf("  marker_x:       offset=%zu\n", offsetof(GameState, marker_x));
    printf("  speed:          offset=%zu\n", offsetof(GameState, speed));
    printf("  is_game_over:   offset=%zu\n", offsetof(GameState, is_game_over));
    printf("========================================\n\n");
}
