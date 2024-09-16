
#include "chip8.h"
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

int main(int argc, char* argv[]) {
    ChipContext chip;
    Display display;
    srand(time(NULL));
    
    if (argc < 2) {
        printf("Usage: %s <path_to_rom>\n", argv[0]);
        return 1;
    } 

    initializeChip(&chip);
    if (initializeGraphics(&display,  DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE) == 1)
        return 1; // Window failed to initalize

    if (loadROM(argv[1], &chip) != 0) {
        return 1; // ROM loading failed
    }

    bool gameIsRunning = true;
    while (gameIsRunning) {

        // Check keypresses
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) 
                gameIsRunning = false;
        }
        executeInstruction(&chip, &display);

        if (chip.delayTimer > 0)
            chip.delayTimer--;
        if (chip.soundTimer > 0)
            chip.soundTimer--;

        chip.keyState = 0;
        SDL_RenderPresent(display.renderer);
        SDL_Delay(1); // 16ms == 60 fps
    }

    SDL_DestroyRenderer(display.renderer);
    SDL_DestroyWindow(display.window);
    SDL_Quit();
    return 0;
}