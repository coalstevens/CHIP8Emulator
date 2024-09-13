// C Standard Libraries
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <stdio.h>



int main(int argc, char* argv[]) {
    SDL_Window* window = NULL; 

    // Initialize the video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized: %s\n", SDL_GetError());
        return 1;
    } else {
        printf("SDL video system is ready to go\n");
    }

    // Before we create our window, specify OpenGL version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Request a window to be created for our platform
    window = SDL_CreateWindow("C SDL2 Window",
                              20,
                              20,
                              640,
                              480,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (!window) {
        printf("Window could not be created: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // OpenGL setup the graphics context
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("OpenGL context could not be created: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool gameIsRunning = true;
    while (gameIsRunning) {
        glViewport(0, 0, 640, 480);

        SDL_Event event;
        // Start our event loop
        while (SDL_PollEvent(&event)) {
            // Handle each specific event
            if (event.type == SDL_QUIT) {
                gameIsRunning = false;
            }
            if (event.type == SDL_MOUSEMOTION) {
                printf("Mouse has been moved\n");
            }
            if (event.type == SDL_KEYDOWN) {
                printf("A key has been pressed\n");
                if (event.key.keysym.sym == SDLK_0) {
                    printf("0 was pressed\n");
                } else {
                    printf("0 was not pressed\n");
                }
            }

            // Retrieve the state of all of the keys
            // Then we can query the scan code of one or more
            // keys at a time
            const Uint8* state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_RIGHT]) {
                printf("Right arrow key is pressed\n");
            }
        }

        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    // We destroy our window. We are passing in the pointer
    // that points to the memory allocated by the 
    // 'SDL_CreateWindow' function.
    SDL_DestroyWindow(window);
    
    // We safely uninitialize SDL2
    SDL_Quit();
    return 0;
}