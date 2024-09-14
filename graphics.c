#include "graphics.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include "chip8.h"

int initializeGraphics(SDL_Window** window, SDL_GLContext* context, SDL_Renderer** renderer) {
    // Initialize the video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized: %s\n", SDL_GetError());
        return 1;
    } else {
        printf("SDL video system is ready to go\n");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    *window = SDL_CreateWindow("C SDL2 Window",
                              20,
                              20,
                              640,
                              480,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (!*window) {
        printf("Window could not be created: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    *context = SDL_GL_CreateContext(*window);
    if (!*context) {
        printf("OpenGL context could not be created: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_GL_DeleteContext(*context);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }

    return 0; // Success
}

// Converts frameBuffer to an array of ARGB8888 pixels
void convertFrameBufferToPixels(const u_int8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH], uint32_t* pixels) {
    for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
        for (int x = 0; x < DISPLAY_WIDTH; ++x) {
            uint8_t color = frameBuffer[y][x] ? 0xFF : 0x00; // White or Black
            pixels[y * DISPLAY_WIDTH + x] = (color << 24) | (color << 16) | (color << 8) | 0xFF; // ARGB8888
        }
    }
}

int renderFrameBuffer(SDL_Renderer* renderer, const u_int8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH]) {
    uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    convertFrameBufferToPixels(frameBuffer, pixels);

    SDL_Texture* framebufferTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!framebufferTexture) {
        printf("Failed to create texture: %s\n", SDL_GetError());
        return -1;
    }
    
    SDL_UpdateTexture(framebufferTexture, NULL, pixels, DISPLAY_WIDTH * sizeof(uint32_t));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, framebufferTexture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(framebufferTexture);
    return 0; 
}