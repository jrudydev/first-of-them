/*
 * First of Them - SDL2 Port
 * Renderer Implementation
 *
 * EXACT PORT - matches original Pebble layer order and positions.
 * Original layer order (lines 356-403):
 *   1. background_layer
 *   2. player_layer
 *   3. zombie_layer  (ON TOP of player - this is correct!)
 *   4. square_layer (timing bar)
 *   5. bullseye_layer
 *   6. marker_layer
 *   7. bullet_layer
 *   8. text layers
 *
 * Copyright (c) 2014, 2026 Rudy Gomez
 */

#include "renderer.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

/* Colors (Pebble was black & white) */
#define COLOR_WHITE     255, 255, 255, 255
#define COLOR_BLACK     0, 0, 0, 255

/* Original GPath definitions for reference:
 *
 * SQUARE_POINTS: {-73, 65} to {72, 75} - the timing bar
 * BULLSEYE_POINTS: {-20, 65} to {20, 75} - black target zone
 * MARKER_POINTS: triangle at y=55-65
 * BULLET_POINTS: 2x2 square
 */

static SDL_Texture* load_texture(SDL_Renderer* renderer, const char* path,
                                  int* width, int* height) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "Failed to load %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    *width = surface->w;
    *height = surface->h;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    /* Enable alpha blending on this texture
     * Original Pebble used GCompOpAnd for transparency
     */
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    return texture;
}

int renderer_init(Renderer* r) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "SDL_image init failed: %s\n", IMG_GetError());
        return -1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf init failed: %s\n", TTF_GetError());
        return -1;
    }

    r->window = SDL_CreateWindow(
        "First of Them - AGINUZ Protected",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!r->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return -1;
    }

    r->renderer = SDL_CreateRenderer(r->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Enable alpha blending for PNG transparency
     * Original used GCompOpAnd for compositing
     */
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_BLEND);

    /* Load font - try common system font paths
     * Original Pebble used Gothic 14 or similar
     * Font size: 14 on Pebble * SCALE_FACTOR = 56 on SDL
     */
    r->font = NULL;
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "../resources/fonts/font.ttf",
        NULL
    };

    int font_size = 14 * SCALE_FACTOR;  /* Scale up like everything else */
    for (int i = 0; font_paths[i] != NULL; i++) {
        r->font = TTF_OpenFont(font_paths[i], font_size);
        if (r->font) {
            printf("Loaded font: %s\n", font_paths[i]);
            break;
        }
    }

    if (!r->font) {
        fprintf(stderr, "Warning: Could not load any font, text will not be displayed\n");
    }

    /* Load textures from original assets */
    r->player_texture = load_texture(r->renderer, "../resources/images/player.png",
                                      &r->player_width, &r->player_height);
    r->zombie_texture = load_texture(r->renderer, "../resources/images/zombie.png",
                                      &r->zombie_width, &r->zombie_height);
    r->background_texture = load_texture(r->renderer, "../resources/images/background.png",
                                          &r->bg_width, &r->bg_height);

    return 0;
}

void renderer_cleanup(Renderer* r) {
    if (r->font) TTF_CloseFont(r->font);
    if (r->player_texture) SDL_DestroyTexture(r->player_texture);
    if (r->zombie_texture) SDL_DestroyTexture(r->zombie_texture);
    if (r->background_texture) SDL_DestroyTexture(r->background_texture);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

/*
 * Original update_square_layer() lines 147-152
 * Draws the white timing bar
 */
static void draw_square_layer(SDL_Renderer* renderer) {
    /* Original SQUARE_POINTS centered at screen center (72, 84)
     * Points: {-73, 65}, {-73, 75}, {72, 75}, {72, 65}
     * After centering: x from -1 to 144, y from 149 to 159
     */
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    /* White fill */
    SDL_SetRenderDrawColor(renderer, COLOR_WHITE);
    SDL_Rect bar = {
        (int)((center_x + (-73 * SCALE_FACTOR))),
        (int)((center_y + (65 * SCALE_FACTOR))),
        (int)((72 - (-73)) * SCALE_FACTOR),  /* width: 145 */
        (int)((75 - 65) * SCALE_FACTOR)       /* height: 10 */
    };
    SDL_RenderFillRect(renderer, &bar);

    /* Black outline */
    SDL_SetRenderDrawColor(renderer, COLOR_BLACK);
    SDL_RenderDrawRect(renderer, &bar);
}

/*
 * Original update_bullseye_layer() lines 154-160
 * Draws the black target zone in center of timing bar
 */
static void draw_bullseye_layer(SDL_Renderer* renderer) {
    /* Original BULLSEYE_POINTS centered at screen center
     * Points: {-20, 65}, {-20, 75}, {20, 75}, {20, 65}
     */
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    SDL_SetRenderDrawColor(renderer, COLOR_BLACK);
    SDL_Rect bullseye = {
        (int)(center_x + (-20 * SCALE_FACTOR)),
        (int)(center_y + (65 * SCALE_FACTOR)),
        (int)((20 - (-20)) * SCALE_FACTOR),  /* width: 40 */
        (int)((75 - 65) * SCALE_FACTOR)       /* height: 10 */
    };
    SDL_RenderFillRect(renderer, &bullseye);
}

/*
 * Original update_marker_layer() drawing code lines 207-210
 * Draws the triangle marker that moves along the timing bar
 */
static void draw_marker_layer(SDL_Renderer* renderer, Game* game) {
    /* Original MARKER_POINTS: triangle
     * {-10, 55}, {0, 65}, {10, 55}
     * Moved to markerPos which starts at (75, 75) but y is fixed
     */
    int center_y = SCREEN_HEIGHT / 2;

    /* marker_x is in Pebble coordinates (0-144), scale it */
    int marker_screen_x = (int)(game->state.marker_x * SCALE_FACTOR);

    SDL_SetRenderDrawColor(renderer, COLOR_BLACK);

    /* Draw triangle as filled polygon (simplified to filled rect for now) */
    /* Original triangle points down into the bar */
    int x1 = marker_screen_x - (10 * SCALE_FACTOR);
    int x2 = marker_screen_x;
    int x3 = marker_screen_x + (10 * SCALE_FACTOR);
    int y1 = center_y + (55 * SCALE_FACTOR);
    int y2 = center_y + (65 * SCALE_FACTOR);

    /* Draw triangle using lines */
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x3, y1);
    SDL_RenderDrawLine(renderer, x3, y1, x1, y1);

    /* Fill (approximate with small rectangle) */
    SDL_Rect marker = {
        marker_screen_x - (5 * SCALE_FACTOR),
        center_y + (57 * SCALE_FACTOR),
        10 * SCALE_FACTOR,
        6 * SCALE_FACTOR
    };
    SDL_RenderFillRect(renderer, &marker);
}

/*
 * Draw centered text at specified Y position
 * Original text_layer_set_text_alignment was GTextAlignmentCenter
 */
static void draw_text_centered(Renderer* r, const char* text, int y) {
    if (!r->font || !text) return;

    SDL_Color black = {0, 0, 0, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(r->font, text, black);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(r->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    /* Center horizontally */
    SDL_Rect dst = {
        (SCREEN_WIDTH - surface->w) / 2,
        y,
        surface->w,
        surface->h
    };

    SDL_RenderCopy(r->renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

/*
 * Draw the three text layers from original
 * Lines 408-424 in original
 */
static void draw_text_layers(Renderer* r, Game* game) {
    char buffer[64];

    /* Original text_layer at y=62: "Dead Dead X"
     * Line 409: text_layer_set_text(text_layer, "Dead Dead");
     * Updated on kill (line 249): "Dead Dead %u", count
     */
    snprintf(buffer, sizeof(buffer), "Dead Dead %d", game->state.count);
    draw_text_centered(r, buffer, 62 * SCALE_FACTOR);

    /* Original msg_layer at y=85: "Press Select Button" or "Game Over"
     * Line 414: text_layer_set_text(msg_layer, "Press Select Button");
     * On game over (line 124): "Game Over"
     */
    if (game->state.is_game_over) {
        draw_text_centered(r, "Game Over", 85 * SCALE_FACTOR);
    } else {
        draw_text_centered(r, "Press Space to Shoot", 85 * SCALE_FACTOR);
    }

    /* Original high_score_layer at y=109: "High Score: X"
     * Lines 419-422
     */
    snprintf(buffer, sizeof(buffer), "High Score: %d", game->state.high_score);
    draw_text_centered(r, buffer, 109 * SCALE_FACTOR);
}

/*
 * Original update_bullet_layer() drawing code lines 261-264
 * BULLET_POINTS: 2x2 square at bulletPos
 */
static void draw_bullet_layer(SDL_Renderer* renderer, Game* game) {
    /* Original: bullet drawn at bulletPos (10, 9) when not shooting,
     * moves right when shooting
     */
    SDL_SetRenderDrawColor(renderer, COLOR_BLACK);

    /* bullet_x/y are in Pebble coordinates, scale them */
    int bullet_screen_x = (int)(game->state.bullet_x * SCALE_FACTOR);
    int bullet_screen_y = (int)(game->state.bullet_y * SCALE_FACTOR);

    SDL_Rect bullet = {
        bullet_screen_x - (1 * SCALE_FACTOR),
        bullet_screen_y - (1 * SCALE_FACTOR),
        2 * SCALE_FACTOR,
        2 * SCALE_FACTOR
    };
    SDL_RenderFillRect(renderer, &bullet);
}

void renderer_draw(Renderer* r, Game* game) {
    /* Clear screen (white background like Pebble) */
    SDL_SetRenderDrawColor(r->renderer, COLOR_WHITE);
    SDL_RenderClear(r->renderer);

    /*
     * LAYER ORDER - EXACT from original init() lines 356-403
     * This order is CRITICAL - zombies render ON TOP of player!
     */

    /* 1. background_layer (lines 357-361)
     * Original: graphics_draw_bitmap_in_rect at origin {0, 18}
     */
    if (r->background_texture) {
        SDL_Rect bg_rect = {
            0,
            18 * SCALE_FACTOR,
            r->bg_width * SCALE_FACTOR,
            r->bg_height * SCALE_FACTOR
        };
        SDL_RenderCopy(r->renderer, r->background_texture, NULL, &bg_rect);
    }

    /* 2. player_layer (lines 363-368)
     * Original: graphics_draw_bitmap_in_rect at origin {0, 0}
     */
    if (r->player_texture) {
        SDL_Rect player_rect = {
            0,
            0,
            r->player_width * SCALE_FACTOR,
            r->player_height * SCALE_FACTOR
        };
        SDL_RenderCopy(r->renderer, r->player_texture, NULL, &player_rect);
    }

    /* 3. zombie_layer (lines 370-378) - RENDERS ON TOP OF PLAYER
     * Original: graphics_draw_bitmap_in_rect at origin {100, 2} within layer
     * Layer frame moves left (layer_x decreases from 0)
     * Zombie screen position = layer_x + 100
     */
    if (r->zombie_texture) {
        for (int i = 0; i < MAX_ZOMBIES; i++) {
            if (game->zombies[i].active) {
                /* Original line 131: .origin = { 100, 2}
                 * Zombie is drawn at (100, 2) within its layer
                 * Layer position is layer_x
                 */
                int zombie_screen_x = (int)((game->zombies[i].layer_x + ZOMBIE_DRAW_X) * SCALE_FACTOR);
                int zombie_screen_y = ZOMBIE_DRAW_Y * SCALE_FACTOR;

                SDL_Rect zombie_rect = {
                    zombie_screen_x,
                    zombie_screen_y,
                    r->zombie_width * SCALE_FACTOR,
                    r->zombie_height * SCALE_FACTOR
                };
                SDL_RenderCopy(r->renderer, r->zombie_texture, NULL, &zombie_rect);
            }
        }
    }

    /* 4. square_layer - timing bar (lines 380-385) */
    draw_square_layer(r->renderer);

    /* 5. bullseye_layer - black target zone (lines 387-392) */
    draw_bullseye_layer(r->renderer);

    /* 6. marker_layer - moving triangle (lines 394-399) */
    draw_marker_layer(r->renderer, game);

    /* 7. bullet_layer (lines 401-406) */
    draw_bullet_layer(r->renderer, game);

    /* 8. Text layers (lines 408-424)
     * text_layer at y=62: "Dead Dead X"
     * msg_layer at y=85: "Press Select Button" / "Game Over"
     * high_score_layer at y=109: "High Score: X"
     */
    draw_text_layers(r, game);

    SDL_RenderPresent(r->renderer);
}
