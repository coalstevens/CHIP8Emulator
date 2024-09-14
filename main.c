// C Standard Libraries
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <stdio.h>
#include "chip8.h"
#include "graphics.h"

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL; 
    SDL_GLContext context;
    SDL_Renderer* renderer = NULL;
    ChipContext chip;

    initalizeChip(&chip);
    if (initializeGraphics(&window, &context, &renderer) == 1)
        return 1; // Window failed to initalize

    bool gameIsRunning = true;
    while (gameIsRunning) {
        glViewport(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

        // Check keypresses
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) 
                gameIsRunning = false;
            
            if (event.type == SDL_KEYDOWN) 
                handleKeyPress(event.key.keysym.sym, &chip);
        }
        executeCPUCycle(&chip);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderFrameBuffer(renderer, chip.frameBuffer);
        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}