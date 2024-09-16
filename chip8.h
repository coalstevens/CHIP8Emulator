#ifndef CHIP8_H
#define CHIP8_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define SCALE 10

enum layer
{
  FOREGROUND,
  BACKGROUND
};

typedef struct Display
{
  SDL_Renderer *renderer;
  SDL_Window *window;
  enum layer drawLayer;
} Display;

typedef struct ChipContext
{
  uint8_t memory[4 * 1024]; // 4096 bytes of memory
  uint8_t V[16];            // 16 8-bit registers
  uint16_t stack[16];       // 16 byte stack

  uint16_t I;
  uint16_t PC;        // Program Counter
  uint8_t SP;         // Stack Pointer
  uint8_t delayTimer; // Delay timer
  uint8_t soundTimer; // Sound register
  uint8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} ChipContext;

void initializeChip(ChipContext *chip);
int initializeGraphics(Display *display, const int width, const int height);
int loadROM(const char *filename, ChipContext *chip);
void executeCPUCycle(ChipContext *chip, Display *display);
void setDrawLayer(Display *display, const enum layer layerName);
void drawPixel(Display *display, const uint8_t x, const uint8_t y);

#endif // CHIP8_H