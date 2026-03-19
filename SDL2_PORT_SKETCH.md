# First of Them - SDL2 Port Sketch

## File Structure

```
first-of-them-sdl/
├── CMakeLists.txt
├── src/
│   ├── main.c              # Entry point, SDL init, game loop
│   ├── game_state.h        # THE MUTATION TARGET
│   ├── game_state.c        # State management
│   ├── renderer.h          # Drawing functions
│   ├── renderer.c          # SDL2 rendering
│   ├── input.h             # Input handling
│   └── input.c             # Keyboard events
├── assets/
│   ├── player.bmp
│   ├── zombie.bmp
│   └── background.bmp
├── mutator/
│   ├── mutate.py           # Struct shuffler
│   └── templates/
│       └── game_state.h.template
└── builds/
    ├── unmutated/
    └── mutated/
```

---

## game_state.h - THE MUTATION TARGET

```c
#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// ============================================================
// THIS STRUCT IS THE MUTATION TARGET
// The field order will be shuffled by the mutation tool
// ============================================================

typedef struct GameState {
    // Original fields from Pebble version
    int score;              // offset +0  (will change after mutation)
    int highScore;          // offset +4
    float speed;            // offset +8
    float bulletSpeed;      // offset +12
    int zombieCount;        // offset +16
    bool isGameOver;        // offset +20
    bool isShooting;        // offset +21
    bool direction;         // offset +22

    // NEW: Added for mutation demo (CheatEngine targets)
    int health;             // offset +24 - CHEATERS LOOK FOR THIS
    int maxHealth;          // offset +28
    int ammo;               // offset +32 - AND THIS
    int maxAmmo;            // offset +36

    // Position data
    float markerX;          // offset +40
    float markerY;          // offset +44
    float bulletX;          // offset +48
    float bulletY;          // offset +52

    // Bounds
    int markerMin;          // offset +56
    int markerMax;          // offset +60
} GameState;

// Accessors (these get rewritten by mutation tool)
void game_state_init(GameState* state);
void game_state_reset(GameState* state);
int game_state_get_score(GameState* state);
void game_state_add_score(GameState* state, int points);
int game_state_get_health(GameState* state);
void game_state_take_damage(GameState* state, int damage);
int game_state_get_ammo(GameState* state);
void game_state_use_ammo(GameState* state);

#endif // GAME_STATE_H
```

---

## game_state.c

```c
#include "game_state.h"

void game_state_init(GameState* state) {
    state->score = 0;
    state->highScore = 0;
    state->speed = 5.0f;
    state->bulletSpeed = 10.0f;
    state->zombieCount = 0;
    state->isGameOver = false;
    state->isShooting = false;
    state->direction = true;

    // Mutation demo fields
    state->health = 100;
    state->maxHealth = 100;
    state->ammo = 30;
    state->maxAmmo = 30;

    // Positions
    state->markerX = 320.0f;  // Center of 640px window
    state->markerY = 400.0f;
    state->bulletX = 50.0f;
    state->bulletY = 240.0f;

    state->markerMin = -40;
    state->markerMax = 40;
}

void game_state_reset(GameState* state) {
    state->score = 0;
    state->speed = 5.0f;
    state->zombieCount = 0;
    state->isGameOver = false;
    state->health = state->maxHealth;
    state->ammo = state->maxAmmo;
}

int game_state_get_score(GameState* state) {
    return state->score;
}

void game_state_add_score(GameState* state, int points) {
    state->score += points;
    if (state->score > state->highScore) {
        state->highScore = state->score;
    }
}

int game_state_get_health(GameState* state) {
    return state->health;
}

void game_state_take_damage(GameState* state, int damage) {
    state->health -= damage;
    if (state->health <= 0) {
        state->health = 0;
        state->isGameOver = true;
    }
}

int game_state_get_ammo(GameState* state) {
    return state->ammo;
}

void game_state_use_ammo(GameState* state) {
    if (state->ammo > 0) {
        state->ammo--;
    }
}
```

---

## main.c

```c
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "game_state.h"
#include "renderer.h"
#include "input.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FPS 60
#define FRAME_DELAY (1000 / FPS)

// Global game state - THIS IS WHAT CHEATENGINE FINDS
static GameState g_gameState;

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "First of Them - AGUNIZ PoC",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize game state
    game_state_init(&g_gameState);

    // Load assets
    renderer_init(renderer);

    // Game loop
    bool running = true;
    Uint32 frameStart;
    int frameTime;

    while (running) {
        frameStart = SDL_GetTicks();

        // Handle input
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            handle_input(&event, &g_gameState);
        }

        // Update game logic
        if (!g_gameState.isGameOver) {
            update_marker(&g_gameState);
            update_bullet(&g_gameState);
            update_zombies(&g_gameState);

            // Passive damage over time (for demo)
            // Every 2 seconds, lose 1 health
            static Uint32 lastDamage = 0;
            if (SDL_GetTicks() - lastDamage > 2000) {
                game_state_take_damage(&g_gameState, 1);
                lastDamage = SDL_GetTicks();
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        render_game(renderer, &g_gameState);
        render_hud(renderer, &g_gameState);  // Shows health, ammo, score

        SDL_RenderPresent(renderer);

        // Cap framerate
        frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }

    // Cleanup
    renderer_cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
```

---

## input.c

```c
#include "input.h"
#include <SDL2/SDL.h>

void handle_input(SDL_Event* event, GameState* state) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_SPACE:
                // Shoot - equivalent to Pebble SELECT button
                if (!state->isGameOver && state->ammo > 0) {
                    // Check if marker is in the bullseye zone
                    float center = 320.0f;  // Window center
                    if (state->markerX >= center + state->markerMin &&
                        state->markerX <= center + state->markerMax) {
                        state->isShooting = true;
                        game_state_use_ammo(state);
                    }
                }
                break;

            case SDLK_r:
                // Reset game
                if (state->isGameOver) {
                    game_state_reset(state);
                }
                break;

            case SDLK_h:
                // DEBUG: Add health (for testing CheatEngine)
                state->health += 10;
                if (state->health > state->maxHealth) {
                    state->health = state->maxHealth;
                }
                break;

            case SDLK_ESCAPE:
                // Could trigger quit
                break;
        }
    }
}
```

---

## renderer.c (simplified)

```c
#include "renderer.h"
#include <SDL2/SDL.h>
#include <stdio.h>

static SDL_Texture* playerTexture = NULL;
static SDL_Texture* zombieTexture = NULL;

void renderer_init(SDL_Renderer* renderer) {
    // Load textures (simplified - use SDL_image for PNG)
    SDL_Surface* surface;

    // For now, we'll draw shapes instead of loading images
    // This keeps the PoC simple
}

void renderer_cleanup(void) {
    if (playerTexture) SDL_DestroyTexture(playerTexture);
    if (zombieTexture) SDL_DestroyTexture(zombieTexture);
}

void render_game(SDL_Renderer* renderer, GameState* state) {
    // Draw timing bar (white rectangle)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect timingBar = { 50, 400, 540, 30 };
    SDL_RenderFillRect(renderer, &timingBar);

    // Draw bullseye zone (black rectangle in center)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect bullseye = { 280, 400, 80, 30 };  // Center zone
    SDL_RenderFillRect(renderer, &bullseye);

    // Draw marker (triangle - simplified as rect)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect marker = { (int)state->markerX - 10, 380, 20, 20 };
    SDL_RenderFillRect(renderer, &marker);

    // Draw player (left side)
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect player = { 30, 200, 40, 80 };
    SDL_RenderFillRect(renderer, &player);

    // Draw bullet if shooting
    if (state->isShooting) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_Rect bullet = { (int)state->bulletX, 240, 10, 5 };
        SDL_RenderFillRect(renderer, &bullet);
    }

    // Draw zombies (right side, moving left)
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    // Simplified: just draw one zombie
    SDL_Rect zombie = { 550, 200, 40, 80 };
    SDL_RenderFillRect(renderer, &zombie);

    // Game over text
    if (state->isGameOver) {
        // Would use SDL_ttf for text
        // For now, just darken screen
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_Rect overlay = { 0, 0, 640, 480 };
        SDL_RenderFillRect(renderer, &overlay);
    }
}

void render_hud(SDL_Renderer* renderer, GameState* state) {
    // HUD background
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect hudBg = { 0, 0, 640, 50 };
    SDL_RenderFillRect(renderer, &hudBg);

    // Health bar
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect healthBg = { 10, 10, 200, 20 };
    SDL_RenderFillRect(renderer, &healthBg);

    float healthPercent = (float)state->health / state->maxHealth;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect healthFill = { 10, 10, (int)(200 * healthPercent), 20 };
    SDL_RenderFillRect(renderer, &healthFill);

    // Ammo bar
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect ammoBg = { 220, 10, 100, 20 };
    SDL_RenderFillRect(renderer, &ammoBg);

    float ammoPercent = (float)state->ammo / state->maxAmmo;
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_Rect ammoFill = { 220, 10, (int)(100 * ammoPercent), 20 };
    SDL_RenderFillRect(renderer, &ammoFill);

    // Score would be rendered with SDL_ttf
    // For PoC, the health/ammo bars are the main mutation targets
}

void update_marker(GameState* state) {
    // Move marker left/right
    if (state->direction) {
        state->markerX += state->speed;
        if (state->markerX >= 590) {
            state->direction = false;
            state->zombieCount++;
        }
    } else {
        state->markerX -= state->speed;
        if (state->markerX <= 50) {
            state->direction = true;
            state->zombieCount++;
        }
    }

    // Increase speed over time
    if (state->speed < 15.0f) {
        state->speed += 0.001f;
    }
}

void update_bullet(GameState* state) {
    if (state->isShooting) {
        state->bulletX += state->bulletSpeed;

        // Check if bullet hit zombie (simplified)
        if (state->bulletX >= 550) {
            // Hit!
            game_state_add_score(state, 1);
            state->isShooting = false;
            state->bulletX = 50;
        }

        // Bullet went off screen
        if (state->bulletX >= 640) {
            state->isShooting = false;
            state->bulletX = 50;
        }
    }
}

void update_zombies(GameState* state) {
    // Simplified zombie logic
    // In full version, track array of zombie positions
}
```

---

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(FirstOfThem C)

set(CMAKE_C_STANDARD 99)

# Find SDL2
find_package(SDL2 REQUIRED)

# Option for mutation - pass different header
set(GAME_STATE_HEADER "${CMAKE_SOURCE_DIR}/src/game_state.h"
    CACHE FILEPATH "Path to game_state.h (for mutation)")

add_executable(FirstOfThem
    src/main.c
    src/game_state.c
    src/renderer.c
    src/input.c
)

target_include_directories(FirstOfThem PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(FirstOfThem ${SDL2_LIBRARIES})

# Copy to builds folder
add_custom_command(TARGET FirstOfThem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:FirstOfThem>
    ${CMAKE_SOURCE_DIR}/builds/
)
```

---

## mutator/mutate.py (simplified)

```python
#!/usr/bin/env python3
"""
AGUNIZ Mutation Tool - Proof of Concept
Shuffles GameState struct fields to break CheatEngine
"""

import random
import sys
import os

TEMPLATE = '''#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// ============================================================
// MUTATED STRUCT - Seed: {seed}
// Original field order has been shuffled
// CheatEngine offsets are now INVALID
// ============================================================

typedef struct GameState {{
{fields}
}} GameState;

// Field offset map (for server validation)
// {offset_map}

#endif // GAME_STATE_H
'''

# Original fields with their types
FIELDS = [
    ('int', 'score'),
    ('int', 'highScore'),
    ('float', 'speed'),
    ('float', 'bulletSpeed'),
    ('int', 'zombieCount'),
    ('bool', 'isGameOver'),
    ('bool', 'isShooting'),
    ('bool', 'direction'),
    ('int', 'health'),
    ('int', 'maxHealth'),
    ('int', 'ammo'),
    ('int', 'maxAmmo'),
    ('float', 'markerX'),
    ('float', 'markerY'),
    ('float', 'bulletX'),
    ('float', 'bulletY'),
    ('int', 'markerMin'),
    ('int', 'markerMax'),
]

def mutate(seed: int):
    """Generate a mutated game_state.h with shuffled fields"""
    random.seed(seed)

    # Shuffle field order
    shuffled = FIELDS.copy()
    random.shuffle(shuffled)

    # Add decoy fields
    decoy_count = random.randint(1, 3)
    for i in range(decoy_count):
        pos = random.randint(0, len(shuffled))
        decoy_type = random.choice(['int', 'float', 'bool'])
        shuffled.insert(pos, (decoy_type, f'_decoy_{i}'))

    # Generate field declarations
    fields_str = ""
    offset = 0
    offset_map = {}

    for field_type, field_name in shuffled:
        # Calculate size
        if field_type == 'bool':
            size = 1
        elif field_type == 'int' or field_type == 'float':
            size = 4
        else:
            size = 4

        # Alignment (simplified)
        if size == 4 and offset % 4 != 0:
            padding = 4 - (offset % 4)
            offset += padding

        fields_str += f"    {field_type} {field_name};\n"

        if not field_name.startswith('_decoy'):
            offset_map[field_name] = offset

        offset += size

    # Generate header
    header = TEMPLATE.format(
        seed=seed,
        fields=fields_str.rstrip(),
        offset_map=str(offset_map)
    )

    return header, offset_map

def main():
    if len(sys.argv) < 2:
        print("Usage: python mutate.py <seed> [output_path]")
        print("Example: python mutate.py 12345 src/game_state.h")
        sys.exit(1)

    seed = int(sys.argv[1])
    output_path = sys.argv[2] if len(sys.argv) > 2 else 'game_state_mutated.h'

    header, offset_map = mutate(seed)

    with open(output_path, 'w') as f:
        f.write(header)

    print(f"Generated mutated header with seed {seed}")
    print(f"Output: {output_path}")
    print(f"\nNew offsets:")
    for field, offset in offset_map.items():
        print(f"  {field}: +{offset}")

if __name__ == '__main__':
    main()
```

---

## Demo Script

```
1. Build unmutated version:
   $ mkdir build && cd build
   $ cmake ..
   $ cmake --build .
   $ cp FirstOfThem.exe ../builds/unmutated/

2. Open in CheatEngine:
   - Attach to FirstOfThem.exe
   - Search for health value (100)
   - Take damage, search again
   - Find address: 0x00XXXXXX
   - Modify to 999 → WORKS

3. Generate mutated build:
   $ python mutator/mutate.py 12345 src/game_state.h
   $ cmake --build . --clean-first
   $ cp FirstOfThem.exe ../builds/mutated/

4. Open mutated in CheatEngine:
   - Same search process
   - Address is DIFFERENT
   - Old cheat table FAILS
   - "The game is the same. The memory is not."

5. Show multiple seeds:
   $ python mutator/mutate.py 11111
   $ python mutator/mutate.py 22222
   $ python mutator/mutate.py 33333
   - Each has different offsets
   - "Every player. Every session. Unique layout."
```

---

## Summary

| Component | Lines (est) | Time |
|-----------|-------------|------|
| game_state.h/c | ~100 | Day 1 |
| main.c | ~100 | Day 1 |
| renderer.c | ~150 | Day 2 |
| input.c | ~50 | Day 2 |
| mutate.py | ~100 | Day 3 |
| CMakeLists.txt | ~30 | Day 1 |
| Testing + Polish | - | Day 4 |

**Total: ~530 lines, 4 days**

---

*Ready to build.*
