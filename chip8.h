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


enum layer {
  FOREGROUND,
  BACKGROUND
};

typedef struct Display {
    SDL_Renderer* renderer;
    SDL_Window* window;
} Display;

typedef struct ChipContext {
    uint8_t memory[4 * 1024]; // 4096 bytes of memory
    uint8_t V[16]; // 16 8-bit registers
    uint16_t stack[16]; // 16 byte stack

    uint16_t I;
    uint16_t PC; // Program Counter
    uint8_t SP; // Stack Pointer
    uint8_t delayTimer; // Delay timer
    uint8_t soundTimer; // Sound register
    uint8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    uint16_t keyState;
    // mapping 16 keys to bits of keyMap
    // 1 2 3 4
    // q w e r
    // a s d f
    // z x c v
} ChipContext;

void initializeChip(ChipContext* chip);
void executeInstruction(ChipContext* chip, Display* display);
//int handleKeyPress(SDL_Keycode keyCode, ChipContext* chip);
//int updateKeyState(SDL_Keycode keyCode, ChipContext* chip, uint8_t pressed);
int loadROM(const char* filename, ChipContext* chip);
void drawPixel(uint8_t x, uint8_t y, enum layer layerName, Display* display);
int initializeGraphics(Display* display, int width, int height);

#endif // CHIP8_H