#include "chip8.h"

#define ROM_START_ADDRESS 0x200
#define MAX_ROM_SIZE (4096 - ROM_START_ADDRESS)

uint8_t fontSet[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2j
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

SDL_Keycode keymap[16] = {
        SDLK_1,
        SDLK_2,
        SDLK_3,
        SDLK_4,
        SDLK_q,
        SDLK_w,
        SDLK_e,
        SDLK_r,
        SDLK_a,
        SDLK_s,
        SDLK_d,
        SDLK_f,
        SDLK_z,
        SDLK_x,
        SDLK_c,
        SDLK_v
};

void initializeChip(ChipContext* chip) {
    chip->PC = ROM_START_ADDRESS;
    chip->SP = 0;
    chip->I = 0;
    chip->delayTimer = 0;
    chip->soundTimer = 0;

    memset(chip->V, 0, sizeof(chip->V)); 
    memset(chip->memory, 0, sizeof(chip->memory)); 
    memset(chip->frameBuffer, 0, sizeof(chip->frameBuffer)); 
    memset(chip->stack, 0, sizeof(chip->stack));

    memcpy(chip->memory, fontSet, sizeof(fontSet));
}

void executeInstruction(ChipContext* chip, Display* display) {
    uint16_t instruction = (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1]; // Grab instruction
    printf("PC: %04X, I: %04X, Executing instruction: %04X\n", chip->PC, chip->I, instruction);
    chip->PC += 2;
    uint8_t x = (instruction & 0x0F00) >> 8;
    uint8_t y = (instruction & 0x00F0) >> 4;
    uint16_t nnn = (instruction & 0x0FFF);
    uint8_t kk = (instruction & 0x00FF);

    switch((instruction & 0xF000) >> 12) { 

        case 0x0:
            switch (instruction & 0xFFF) {
                case 0x0E0: // 00E0 CLS - Clears display
                    memset(chip->frameBuffer, 0, sizeof(chip->frameBuffer));
                    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
                    SDL_RenderClear(display->renderer);
                    break;
                case 0x0EE: // 00EE RET - Jumps to address on top of the stack
                    chip->SP--;
                    chip->PC = chip->stack[chip->SP];
                    break;
                default: // Usually 0nnn SYS addr - Jumps to a machine code nnn. Generally ignored now.
                    break;
            }
            break;

        case 0x1: // 1nnn JP addr
            chip->PC = nnn;
            break;

        case 0x2: // 2nnn CALL addr - Calls subroutine at nnn
            chip->stack[chip->SP] = chip->PC; // Address to return after subroutine completes
            chip->SP++;
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
            if (chip->V[x] == chip->V[y]) 
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
                case 0x0: // LD
                    chip->V[x] = chip->V[y];
                    break;
                case 0x1: // OR
                    chip->V[x] |= chip->V[y];
                    break;
                case 0x2: // AND
                    chip->V[x] &= chip->V[y];
                    break;
                case 0x3: // XOR 
                    chip->V[x] ^= chip->V[y];
                    break;
                case 0x4: // ADD
                    {
                        uint16_t result = chip->V[x] + chip->V[y];
                        chip->V[x] = (uint8_t) result;
                        chip->V[0xF] = (result > 0xFF) ? 1 : 0; // Set carry
                        break;
                    }
                case 0x5: // SUB
                    chip->V[0xF] = (chip->V[x] > chip->V[y]) ? 1 : 0;
                    chip->V[x] -= chip->V[y];
                    break;
                case 0x6: // SHR Vx {, Vy}
                    chip->V[0xF] = (chip->V[x] & 1);
                    chip->V[x] >>= 1;
                    break;
                case 0x7: // SUBN Vx {, Vy}
                    chip->V[0xF] = (chip->V[y] > chip->V[x]) ? 1 : 0;
                    chip->V[x] = chip->V[y] - chip->V[x];
                    break;
                case 0xE: // SHL Vx {, Vy}
                    chip->V[0xF] = (chip->V[x] >> 7);
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
            {
                uint8_t randomValue = rand() % (256); 
                chip->V[x] = randomValue & kk;
                break;
            }
        case 0xD: // Dxyn DRW Vx, VY, nibble 
            {
                uint8_t n = instruction & 0x000F;  
                uint8_t yStart = chip->V[y];
                uint8_t xStart = chip->V[x];
                chip->V[0xF] = 0;

                for(int row = 0; row < n; row++) {
                    uint8_t pixel = chip->memory[chip->I + row];
                    for (int col = 0; col < 8; col++) {
                        if ((pixel & (0b10000000 >> col))) {
                            uint8_t xcoord = (xStart + col) % DISPLAY_WIDTH;
                            uint8_t ycoord = (yStart + row) % DISPLAY_HEIGHT;
                 
                            if (chip->frameBuffer[ycoord][xcoord]) {
                                chip->V[0xF] = 1; // collision detected
                                drawPixel(ycoord, xcoord, BACKGROUND, display);
                                chip->frameBuffer[ycoord][xcoord] = 0;
                            } 
                            else {
                                drawPixel(ycoord, xcoord, FOREGROUND, display);
                                chip->frameBuffer[ycoord][xcoord] = 1;
                            }
                        }
                    }
                }
            }
            break;
        case 0xE:
            switch (instruction & 0x00FF) {
                case 0x9E: // Ex9E SKP Vx - If key V[x] is pressed skip next
                    {
                        const uint8_t* keyStates = SDL_GetKeyboardState(NULL);
                        SDL_Scancode code = SDL_GetScancodeFromKey(keymap[chip->V[x]]);
                        if (keyStates[code])
                            chip->PC += 2; 
                        break;
                    }
                case 0xA1: // ExA1 SKP Vx - If key V[x] is not pressed skip next 
                    {
                        const uint8_t* keyStates = SDL_GetKeyboardState(NULL);
                        SDL_Scancode code = SDL_GetScancodeFromKey(keymap[chip->V[x]]);    
                        if (!keyStates[code])
                            chip->PC += 2;
                        break;
                    }
            }
            break;
        case 0xF:
            switch (instruction & 0x00FF) {
                case 0x07: // Fx07 LD Vx, DT - load delay timer in to Vx
                    chip->V[x] = chip->delayTimer;
                    break;
                case 0x0A: // LD Vx, K - Wait for a key press, store the value of the key in Vx
                    {
                        const uint8_t* keyStates = SDL_GetKeyboardState(NULL);
                        uint8_t pressed = 0;
                        for (int i = 0; i < 16; i++) {
                            SDL_Scancode code = SDL_GetScancodeFromKey(keymap[i]);
                            if (keyStates[code]) {
                                chip->V[x] = i;
                                pressed = 1;
                                break;
                            }
                        }
                        if (pressed == 0)
                            chip->PC -= 2;
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
                    chip->I = chip->V[x] * 5;
                    break;
                case 0x33: // Fx33 LD B, Vx - Stores decimal digits of Vx at mem[I + 0, 1, 2]
                    chip->memory[chip->I] = chip->V[x] / 100;
                    chip->memory[chip->I + 1] = (chip->V[x] / 10) % 10;
                    chip->memory[chip->I + 2] = chip->V[x] % 10;
                    break;
                case 0x55: // Fx55 LD [I], Vx - Copies V[] into memory at I
                    for(int i = 0; i <= x; i++)
                        chip->memory[chip->I + i] = chip->V[i];
                    break;
                case 0x65: // Fx65 LD Vx, [I] - Copies memory starting at I to V[] 
                    for (int i = 0; i <= x; i++)
                        chip->V[i] = chip->memory[chip->I + i];
                    break;
            }
            break;
    }
}

int loadROM(const char* filename, ChipContext* chip) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open ROM file: %s\n", filename);
        return -1;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    if (fileSize < 0) {
        printf("Failed to determine file size.\n");
        fclose(file);
        return -1;
    }
    // Check if the file size is within the acceptable range
    if (fileSize > MAX_ROM_SIZE) {
        printf("ROM file is too large. Maximum size is %d bytes.\n", MAX_ROM_SIZE);
        fclose(file);
        return -1;
    }

    // Read the ROM file into memory
    size_t bytesRead = fread(chip->memory + ROM_START_ADDRESS, 1, fileSize, file);

    // Print all bytes read as 16-bit hexadecimal instructions
    printf("ROM instructions:\n");
    for (size_t i = 0; i < bytesRead; i += 2) {
        if (i + 1 < bytesRead) {
            uint16_t instruction = (chip->memory[ROM_START_ADDRESS + i] << 8) | chip->memory[ROM_START_ADDRESS + i + 1];
            printf("%04X ", instruction);
        } else {
            // Handle case where there is an odd number of bytes
            uint8_t byte = chip->memory[ROM_START_ADDRESS + i];
            printf("%02X ", byte);
        }

        if ((i / 2 + 1) % 8 == 0) {
            printf("\n"); // Newline for readability, every 8 instructions
        }
    }
    printf("\n");

    printf("%zu \n", bytesRead);
    if (bytesRead != fileSize) {
        printf("Failed to read the entire ROM file.\n");
        fclose(file);
        return -1;
    }

    // Close the file
    fclose(file);
    return 0; // Success
}

void drawPixel(uint8_t y, uint8_t x, enum layer layerName, Display* display) {
    switch (layerName) {
        case BACKGROUND:
            SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
            break;
        case FOREGROUND:
            SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
            break;
        default:
            printf("Layer does not exist.");
            break;
    }

    // Draw scaled up pixel
    SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
    SDL_RenderFillRect(display->renderer, &rect);
}

int initializeGraphics(Display* display, int width, int height) {
    // Initialize the video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initialized: %s\n", SDL_GetError());
        return 1;
    } else {
        printf("SDL video system is ready to go\n");
    }

    // Create the window
    display->window = SDL_CreateWindow("CHIP8",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               width,
                               height,
                               SDL_WINDOW_SHOWN);
                               
    if (!display->window) {
        printf("Window could not be created: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create the renderer
    display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_SOFTWARE);
    if (!display->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        SDL_Quit();
        return 1;
    }

    return 0; // Success
}