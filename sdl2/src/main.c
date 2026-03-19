/*
 * First of Them - SDL2 Port
 * Main Entry Point
 *
 * Original: YC Hacks 2014 (Pebble Watch)
 * Port: 2026 (SDL2 Desktop + AGINUZ Protection)
 *
 * "From YC Hacks to YC Founder. Full circle."
 *
 * Copyright (c) 2014, 2026 Rudy Gomez
 */

#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "game.h"
#include "renderer.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           FIRST OF THEM - SDL2 Port                       ║\n");
    printf("║                                                           ║\n");
    printf("║   Original: YC Hacks 2014 (Pebble Watch)                  ║\n");
    printf("║   Port: 2026 (SDL2 + AGINUZ Protection)                   ║\n");
    printf("║                                                           ║\n");
    printf("║   From YC Hacks to YC Founder. Full circle.               ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Print AGINUZ protection status */
    printf("[AGINUZ] Build seed: %u\n", game_get_seed());
    printf("[AGINUZ] Protected: %s\n", game_is_protected() ? "YES" : "NO");
    printf("\n");

    printf("Controls:\n");
    printf("  SPACE  - Shoot (when marker is in black zone)\n");
    printf("  R      - Restart\n");
    printf("  F1     - Show memory layout (for CheatEngine demo)\n");
    printf("  ESC    - Quit\n");
    printf("\n");

    /* Initialize game */
    Game game;
    game_init(&game);

    /* Initialize renderer */
    Renderer renderer;
    if (renderer_init(&renderer) != 0) {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    /* Print layout on startup if protected */
    if (game_is_protected()) {
        game_print_layout(&game);
    }

    /* Game loop - 50ms timer like original Pebble */
    bool running = true;
    Uint32 last_time = SDL_GetTicks();
    const Uint32 frame_time = TIMER_INTERVAL;  /* 50ms = 20 FPS like Pebble */

    while (running) {
        /* Handle events */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            running = false;
                            break;

                        case SDLK_SPACE:
                            /* Shoot - the main mechanic */
                            game_shoot(&game);
                            break;

                        case SDLK_r:
                            /* Reset game */
                            game_reset(&game);
                            break;

                        case SDLK_F1:
                            /* Debug: print memory layout */
                            game_print_layout(&game);
                            break;
                    }
                    break;
            }
        }

        /* Update game at fixed interval (like Pebble timer) */
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_time >= frame_time) {
            game_update(&game);
            last_time = current_time;

            /* Print score periodically - Original: "Dead Dead X" */
            static int last_count = -1;
            if (game.state.count != last_count) {
                printf("Dead Dead %d (High Score: %d)\n",
                       game.state.count, game.state.high_score);
                last_count = game.state.count;
            }

            if (game.state.is_game_over) {
                static bool printed = false;
                if (!printed) {
                    printf("\n*** GAME OVER ***\n");
                    printf("Final Count: %d\n", game.state.count);
                    printf("Press SPACE to restart\n\n");
                    printed = true;
                }
                if (!game.state.is_game_over) {
                    printed = false;
                }
            }
        }

        /* Render */
        renderer_draw(&renderer, &game);
    }

    /* Cleanup */
    renderer_cleanup(&renderer);

    printf("\nThanks for playing First of Them!\n");
    printf("Protected by AGINUZ - github.com/jrudydev/aginuz\n\n");

    return 0;
}
