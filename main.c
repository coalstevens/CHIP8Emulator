
#include "chip8.h"
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define CPU_FREQUENCY 480
#define TIMER_FREQUENCY 60

int main(int argc, char *argv[])
{
    ChipContext chip;
    Display display;
    const uint32_t delay = 1000 / CPU_FREQUENCY;
    const uint32_t timerInterval = 1000 / TIMER_FREQUENCY;
    uint32_t counter;
    srand(time(NULL));

    if (argc < 2)
    {
        printf("Usage: %s <path_to_rom>\n", argv[0]);
        return 1;
    }

    initializeChip(&chip);

    if (initializeGraphics(&display, DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE) == 1)
        return 1; // Window failed to initalize

    if (loadROM(argv[1], &chip) != 0)
    {
        return 1; // ROM loading failed
    }

    bool gameIsRunning = true;
    while (gameIsRunning)
    {

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                gameIsRunning = false;
        }

        executeCPUCycle(&chip, &display);

        // Handle timers
        if (counter >= timerInterval)
        {
            counter = 0;
            if (chip.delayTimer > 0)
                chip.delayTimer--;
            if (chip.soundTimer > 0)
                chip.soundTimer--;
        }

        SDL_RenderPresent(display.renderer);
        SDL_Delay(delay);
        counter += delay;
    }

    SDL_DestroyRenderer(display.renderer);
    SDL_DestroyWindow(display.window);
    SDL_Quit();
    return 0;
}