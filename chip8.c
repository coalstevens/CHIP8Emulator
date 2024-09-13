#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

typedef struct Chip_Context {
    u_int8_t memory[4 * 1024]; // 4096 bytes of memory
    u_int8_t V[16]; // 16 8-bit registers
    u_int8_t stack[16]; // 16 byte stack

    u_int8_t I;
    u_int8_t PC; // Program Counter
    u_int8_t SP; // Stack Pointer
    u_int8_t delay_reg; // Delay timer
    u_int8_t sound_reg; // sound_reg
    u_int8_t frameBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
    u_int8_t keyMap[4][4];
    // keys
    // 1 2 3 4
    // q w e r
    // a s d f
    // z x c v
} Chip_Context;

void initalizeChip() {
    Chip_Context chip;
    chip.PC = 0x200; // Starts at address 0x200
    chip.SP = 0;
    chip.I = 0;
}
void emulateCPUCycle() {
    Chip_Context chip;
    u_int16_t instruction = (chip.memory[chip.PC] << 8) | chip.memory[chip.PC + 1]; // Grab two bytes of mem
    u_int8_t x = instruction & 0x0F00 >> 8;
    u_int8_t y = instruction & 0x00F0 >> 4;
    switch((instruction & 0xF000) >> 12) { 
        case 0x0:
            switch (instruction & 0xFFF) {
                case 0x0E0: // 00E0 CLS - Clears display
                    memset(chip.frameBuffer, 0, sizeof(chip.frameBuffer));
                    break;
                case 0x0EE: // 00EE RET - Jumps to address on top of the stack
                    chip.PC = chip.memory[chip.SP];
                    chip.SP--;
                    break;
                default: // 0nnn SYS addr - Jumps to a machine code nnn. Generally ignored now.
                    break;
            }
            break;
        case 0x1: // 1nnn JP addr - Jumps to nnn
            chip.PC = chip.memory[instruction & 0xFFF];
            break;
        case 0x2: // 2nnn CALL addr - Calls subroutine at nnn
            chip.SP++;
            chip.stack[chip.SP] = chip.PC; // Address to return after subroutine completes
            chip.PC = instruction & 0xFFF;
            break;
        case 0x3: // 3xkk SE Vx, byte - Skips next instruction if Vx = kk
            if (chip.V[x] == instruction && 0xFF) 
                chip.PC += 2;
            break;
        case 0x4: // 4xkk SNE Vx, byte - Skips next instruction if Vx != kk
            if (chip.V[x] != instruction && 0xFF) 
                chip.PC += 2;
            break;
        case 0x5: // 5xy0 SE, Vx, Vy - Skips next instruction if Vx = Vy
            if (chip.V[x] != chip.V[(instruction & 0x0F0) >> 4]) 
                chip.PC += 2; 
            break;
        case 0x6: // 6xkk LD Vx, byte - Loads kk into Vx
            chip.V[(instruction & 0xF00 >> 8)] = instruction & 0xFF;
            break;
        case 0x7: // 7xkk ADD Vx, byte - Vx += kk
            chip.V[(instruction & 0xF00 >> 8)] += instruction & 0xFF;
            break;
        case 0x8: // 8xyk Performs an operation between Vx and Vy
            
            switch (instruction & 0xF) {
                case 0: // LD
                    chip.V[x] = chip.V[y];
                    break;
                case 1: // OR
                    chip.V[x] |= chip.V[y];
                    break;
                case 2: // AND
                    chip.V[x] &= chip.V[y];
                    break;
                case 3: // XOR 
                    chip.V[x] ^= chip.V[y];
                    break;
                case 4: // ADD
                    u_int16_t result = chip.V[x] + chip.V[y];
                    chip.V[x] = (u_int8_t) result;
                    chip.V[0xF] = (result > 0xFF) ? 1 : 0;
                    break;
                case 5: // SUB
                    chip.V[0xF] = (chip.V[x] > chip.V[y]) ? 1 : 0;
                    chip.V[x] -= chip.V[y];
                    break;
                case 6: // SHR Vx {, Vy}
                    chip.V[0xF] = (chip.V[x] & 1) ? 1 : 0;
                    chip.V[x] >>= 1;
                    break;
                case 7: // SUBN Vx {, Vy}
                    chip.V[0xF] = (chip.V[y] > chip.V[x]) ? 1 : 0;
                    chip.V[x] = chip.V[y] - chip.V[x];
                    break;
                case 0xE: // SHL Vx {, Vy}
                    chip.V[0xF] = (chip.V[x] & (1 << 7)) ? 1 : 0;
                    chip.V[x] <<= 1;
                    break;
            }
            break;
        case 0x9: // 4xkk SNE Vx, byte - Skips next instruction if Vx != kk
            if (chip.V[x] != chip.V[(instruction & 0xFF)]) 
                chip.PC += 2;
            break;
        case 0xA: // Annn LD I, addr - Loads address into I
            chip.I = (instruction & 0xFFF);
            break;
        case 0xB: // Bnnn JP V0, addr - Jump to nnn + V0
            chip.PC = (instruction & 0xFFF) + chip.V[0];
            break;
        case 0xC: // Cxkk RND Vx, Byte 0 - Vx = a random 8-bit value & kk
            u_int8_t randomValue = rand() % (255 + 1); 
            chip.V[x] = randomValue & (instruction * 0xFF);
            break;
        case 0xD: // Dxyn DRW Vx, VY, nibble 
            int n = instruction & 0x000F;  
            int frameRow = chip.V[x];
            
            for(int spriteByte = 0; spriteByte < n; spriteByte++, frameRow++) {
                int frameCol = chip.V[y];
                for (int i = 0; i < 8; i++, frameCol++) {
                    u_int8_t pixel = (chip.memory[chip.I + spriteByte] >> (7-i)) & 1;
                    if (pixel == 1) {
                        if (frameRow >= DISPLAY_HEIGHT) frameRow = 0; // Wrap around top if needed
                        if (frameCol >= DISPLAY_WIDTH) frameCol = 0;  // Wrap around left if needed
                        
                        if (chip.frameBuffer[frameRow][frameCol] == 1) chip.V[0xF] = 1; // collision detected

                        chip.frameBuffer[frameRow][frameCol] ^= 1;
                    }
                }
            }
            break;
        case 0xE:
            switch (instruction & 0x00FF) {
                case 0x9E:                   
                    break;
                case 0xA1:    
                    break;
            }
            break;
        case 0xF:
            switch (instruction & 0x00FF) {
                case 0x07:
                case 0x0A:
                case 0x15:
                case 0x18:
                case 0x1E:
                case 0x29:
                case 0x33:
                case 0x55:
                case 0x65:
            }
            break;
    }
}