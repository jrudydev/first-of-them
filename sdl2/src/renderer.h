/*
 * First of Them - SDL2 Port
 * Renderer Header
 *
 * Copyright (c) 2026 Rudy Gomez
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "game.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* player_texture;
    SDL_Texture* zombie_texture;
    SDL_Texture* background_texture;
    int player_width, player_height;
    int zombie_width, zombie_height;
    int bg_width, bg_height;

    /* Font for text rendering - Original used Pebble system font */
    TTF_Font* font;
} Renderer;

int renderer_init(Renderer* r);
void renderer_cleanup(Renderer* r);
void renderer_draw(Renderer* r, Game* game);

#endif /* RENDERER_H */
