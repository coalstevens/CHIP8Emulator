#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

void initializeChip(ChipContext* chip) {
    ChipContext chip;
    chip->PC = 0x200; // Starts at address 0x200
    chip->SP = 0;
    chip->I = 0;
    memset(chip->V, 0, sizeof(chip->V)); 
    memset(chip->memory, 0, sizeof(chip->memory)); 
    memset(chip->frameBuffer, 0, sizeof(chip->frameBuffer)); 
    memset(chip->stack, 0, sizeof(chip->stack));
    chip->delayTimer = 0;
    chip->soundTimer = 0;
}

void executeCPUCycle(ChipContext* chip) {
    u_int16_t instruction = (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1]; // Grab instruction
    chip->PC += 2; // Move PC to next instruction
    u_int8_t x = (instruction & 0x0F00) >> 8;
    u_int8_t y = (instruction & 0x00F0) >> 4;
    u_int8_t nnn = (instruction & 0x0FFF);
    u_int8_t kk = (instruction & 0x00FF);

    switch((instruction & 0xF000) >> 12) { 
        case 0x0:
            switch (instruction & 0xFFF) {
                case 0x0E0: // 00E0 CLS - Clears display
                    memset(chip->frameBuffer, 0, sizeof(chip->frameBuffer));
                    break;
                case 0x0EE: // 00EE RET - Jumps to address on top of the stack
                    chip->PC = chip->memory[chip->SP];
                    chip->SP--;
                    break;
                default: // Usually 0nnn SYS addr - Jumps to a machine code nnn. Generally ignored now.
                    break;
            }
            break;
        case 0x1: // 1nnn JP addr
            chip->PC = nnn;
            break;
        case 0x2: // 2nnn CALL addr - Calls subroutine at nnn
            chip->SP++;
            chip->stack[chip->SP] = chip->PC; // Address to return after subroutine completes
            chip->PC = nnn;
            break;
        case 0x3: // 3xkk SE Vx, byte - Skips next instruction if Vx = kk
            if (chip->V[x] == kk) 
                chip->PC += 2;
            break;
        case 0x4: // 4xkk SNE Vx, byte - Skips next instruction if Vx != kk
            if (chip->V[x] != kk) 
                chip->PC += 2;
            break;
        case 0x5: // 5xy0 SE, Vx, Vy - Skips next instruction if Vx = Vy
            if (chip->V[x] != chip->V[y]) 
                chip->PC += 2; 
            break;
        case 0x6: // 6xkk LD Vx, byte
            chip->V[x] = kk;
            break;
        case 0x7: // 7xkk ADD Vx, byte
            chip->V[x] += kk;
            break;
        case 0x8: // 8xyk Performs an operation between Vx and Vy
            switch (instruction & 0xF) {
                case 0: // LD
                    chip->V[x] = chip->V[y];
                    break;
                case 1: // OR
                    chip->V[x] |= chip->V[y];
                    break;
                case 2: // AND
                    chip->V[x] &= chip->V[y];
                    break;
                case 3: // XOR 
                    chip->V[x] ^= chip->V[y];
                    break;
                case 4: // ADD
                    u_int16_t result = chip->V[x] + chip->V[y];
                    chip->V[x] = (u_int8_t) result;
                    chip->V[0xF] = (result > 0xFF) ? 1 : 0;
                    break;
                case 5: // SUB
                    chip->V[0xF] = (chip->V[x] > chip->V[y]) ? 1 : 0;
                    chip->V[x] -= chip->V[y];
                    break;
                case 6: // SHR Vx {, Vy}
                    chip->V[0xF] = (chip->V[x] & 1) ? 1 : 0;
                    chip->V[x] >>= 1;
                    break;
                case 7: // SUBN Vx {, Vy}
                    chip->V[0xF] = (chip->V[y] > chip->V[x]) ? 1 : 0;
                    chip->V[x] = chip->V[y] - chip->V[x];
                    break;
                case 0xE: // SHL Vx {, Vy}
                    chip->V[0xF] = (chip->V[x] & (1 << 7)) ? 1 : 0;
                    chip->V[x] <<= 1;
                    break;
            }
            break;
        case 0x9: // 9xy0 SNE Vx, Vy - Skips next instruction if Vx != Vy
            if (chip->V[x] != chip->V[y]) 
                chip->PC += 2;
            break;
        case 0xA: // Annn LD I, addr - Loads address into I
            chip->I = nnn;
            break;
        case 0xB: // Bnnn JP V0, addr - Jump to nnn + V0
            chip->PC = nnn + chip->V[0];
            break;
        case 0xC: // Cxkk RND Vx, Byte 0 - Vx = a random 8-bit value & kk
            u_int8_t randomValue = rand() % (256); 
            chip->V[x] = randomValue & kk;
            break;
        case 0xD: // Dxyn DRW Vx, VY, nibble 
            int n = instruction & 0x000F;  
            int frameRow = chip->V[x];
            
            for(int spriteByte = 0; spriteByte < n; spriteByte++, frameRow++) {
                int frameCol = chip->V[y];
                for (int i = 0; i < 8; i++, frameCol++) {
                    u_int8_t pixel = (chip->memory[chip->I + spriteByte] >> (7 - i)) & 1;
                    if (pixel == 1) {
                        if (frameRow >= DISPLAY_HEIGHT) frameRow = 0; // Wrap around top if needed
                        if (frameCol >= DISPLAY_WIDTH) frameCol = 0;  // Wrap around left if needed
                        
                        if (chip->frameBuffer[frameRow][frameCol] == 1) chip->V[0xF] = 1; // collision detected

                        chip->frameBuffer[frameRow][frameCol] ^= 1;
                    }
                }
            }
            break;
        case 0xE:
            switch (instruction & 0x00FF) {
                case 0x9E: // Ex9E SKP Vx - If key V[x] is pressed skip next         
                    if(((chip->keyState >> chip->V[x]) & 1) == 1) 
                        chip->PC += 2;  
                    break;
                case 0xA1: // ExA1 SKP Vx - If key V[x] is not pressed skip next     
                    if(((chip->keyState >> chip->V[x]) & 1) == 0) 
                        chip->PC += 2;
                    break;
            }
            break;
        case 0xF:
            switch (instruction & 0x00FF) {
                case 0x07: // Fx07 LD Vx, DT - load delay timer in to Vx
                    chip->V[x] = chip->delayTimer;
                    break;
                case 0x0A: // LD Vx, K - Wait for a key press, store the value of the key in Vx
                    SDL_Event event;
                    while(1) {
                        SDL_WaitEvent(&event);
                        if (event.type == SDL_KEYDOWN) {
                            int keyPressed = handleKeyPress(event.key.keysym.sym, chip);
                            if (keyPressed != -1) {
                                chip->V[x] = keyPressed;
                                break;
                            }
                        }
                    }
                    break;
                case 0x15: // Fx15 LD DT, Vx
                    chip->delayTimer = chip->V[x];
                    break;
                case 0x18: // Fx18 LD ST, Vx
                    chip->soundTimer = chip->V[x];
                    break;
                case 0x1E: // Fx1E ADD I, Vx - I 
                    chip->I += chip->V[x];
                    break;
                case 0x29: // Fx29 LD F, Vx
                    chip->I = fontSet[chip->V[x] * 5];
                    break;
                case 0x33: // Fx33 LD B, Vx - Stores decimal digits of Vx at mem[I + 0, 1, 2]
                    chip->memory[chip->I] = chip->V[x] / 100;
                    chip->memory[chip->I + 1] = (chip->V[x] / 10) % 10;
                    chip->memory[chip->I + 2] = chip->V[x] % 10;
                    break;
                case 0x55: // Fx55 LD [I], Vx - Copies V[] into memory at I
                    for(int i = 0; i < 16; i++)
                        chip->memory[chip->I + i] = chip->V[i];
                    break;
                case 0x65: // Fx65 LD Vx, [I] - Copies memory starting at I to V[] 
                    for (int i = 0; i < 16; i++)
                        chip->V[i] = chip->memory[chip->I + i];
                    break;
            }
            break;
    }
}

int handleKeyPress(SDL_KeyCode keyCode, ChipContext* chip) {
    switch (keyCode) {
        case SDLK_1:
            chip->keyState |= 1 << 0x0;
            return 0x0;
        case SDLK_2:
            chip->keyState |= 1 << 0x1;
            return 0x1;
        case SDLK_3:
            chip->keyState |= 1 << 0x2;
            return 0x2;
        case SDLK_4:
            chip->keyState |= 1 << 0x3;
            return 0x3;
        case SDLK_q:
            chip->keyState |= 1 << 0x4;
            return 0x4;
        case SDLK_w:
            chip->keyState |= 1 << 0x5;
            return 0x5;
        case SDLK_e:
            chip->keyState |= 1 << 0x6;
            return 0x6;
        case SDLK_r:
            chip->keyState |= 1 << 0x7;
            return 0x7;
        case SDLK_a:
            chip->keyState |= 1 << 0x8;
            return 0x8;
        case SDLK_s:
            chip->keyState |= 1 << 0x9;
            return 0x9;
        case SDLK_d:
            chip->keyState |= 1 << 0xA;
            return 0xA;
        case SDLK_f:
            chip->keyState |= 1 << 0xB;
            return 0xB;
        case SDLK_z:
            chip->keyState |= 1 << 0xC;
            return 0xC;
        case SDLK_x:
            chip->keyState |= 1 << 0xD;
            return 0xD;
        case SDLK_c:
            chip->keyState |= 1 << 0xE;
            return 0xE;
        case SDLK_v:
            chip->keyState |= 1 << 0xF;
            return 0xF;
        default:
            return -1;
    }
} 