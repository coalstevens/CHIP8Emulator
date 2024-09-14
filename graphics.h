#ifndef GRAPHICS
#define GRAPHICS    

int initializeGraphics(SDL_Window** window, SDL_GLContext* context, SDL_Renderer** renderer);
int renderFrameBuffer(SDL_Renderer* renderer, const u_int8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH]);
void convertFrameBufferToPixels(const u_int8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH], uint32_t* pixels);

#endif // GRAPHICS