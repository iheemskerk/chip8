#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "chip8.h"

using namespace std;

Chip8::Chip8() :
    registers(),
    keyStates(),
    memory(),
    stack()
{
    opcode = 0;
    i = 0;
    programCounter = PC_START;
    stackPointer = -1;
    paused = false;
    updateScreen = true;

    // init map to all zeroes
    for (int h = 0; h < MAP_HEIGHT; h++) {
        for (int w = 0; w < MAP_WIDTH; w++) {
            map[w][h] = 0;
        }
    }

    loadFont();
}

void Chip8::loadFont() {
    unsigned char font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
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

    // load fonts into memory
    for (int x = 0; x < 80; x++) {
        memory[x] = font[x];
    }
}

void Chip8::loadRom(string rom) {
    ifstream is(rom, ifstream::binary);

    if (is) {
        // load rom into temp buffer
        is.seekg(0, is.end);
        romSize = is.tellg();
        is.seekg(0, is.beg);
        char *buffer = new char[romSize];
        is.read(buffer, romSize);

        // load rom into memory
        for (int x = 0; x < romSize; x++) {
            memory[x+PC_START] = buffer[x];
        }

        delete[] buffer;
    }
}

void Chip8::draw(int x, int y, int height) {
    uint8_t bits[SPRITE_WIDTH];

    // set sprite in map
    for (int h = 0; h < height; h++) {
        for (int m = 0; m < SPRITE_WIDTH; ++m) {
            bits[SPRITE_WIDTH-1-m] = (memory[i+h] >> m) & 1;
        }

        for (int w = 0; w < SPRITE_WIDTH; w++) {
            int sprite_h = y + h;
            int sprite_w = x + w;

            if (sprite_w < MAP_WIDTH && sprite_h < MAP_HEIGHT) {
                uint8_t prev = map[sprite_w][sprite_h];
                map[sprite_w][sprite_h] ^= bits[w];

                if (prev == 1 && map[sprite_w][sprite_h] == 0)
                    registers[0xf] = 1;
            }
        }
    }
}


void Chip8::execute() {
    bool increment = true;
    opcode = (memory[programCounter] << 8) | memory[programCounter+1];

    printf("0x%04x    0x%04x\n", programCounter, opcode);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00e0:
                    cout << "clearing screen..." << endl;
                    for (int h = 0; h < MAP_HEIGHT; h++) {
                        for (int w = 0; w < MAP_WIDTH; w++) {
                            map[w][h] = 0;
                        }
                    }
                    updateScreen = true;
                    break;
                case 0x00ee:
                    programCounter = stack[stackPointer--];
                    break;
                default:
                    printf("  %x not implemented...\n", opcode);
                    exit(0);
            }
            break;
        case 0x1000:
            programCounter = opcode & 0x0FFF;
            increment = false;
            break;
        case 0x2000:
            stack[++stackPointer] = programCounter;
            programCounter = opcode & 0x0FFF;
            increment = false;
            break;
        case 0x3000:
            if (registers[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                programCounter += 2;
            }
            break;
        case 0x4000:
            if (registers[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                programCounter += 2;
            }
            break;
        case 0x5000:
            if (registers[(opcode & 0x0F00) >> 8] == registers[(opcode & 0x00F0) >> 4]) {
                programCounter += 2;
            }
            break;
        case 0x6000:
            registers[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            break;
        case 0x7000:
            registers[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    registers[(opcode & 0x0F00) >> 8] = registers[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0001:
                    registers[(opcode & 0x0F00) >> 8] |= registers[(opcode & 0x00F0)] >> 4;
                    break;
                case 0x0002:
                    registers[(opcode & 0x0F00) >> 8] &= registers[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0003:
                    registers[(opcode & 0x0F00) >> 8] ^= registers[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0004:
                    registers[0xF] = registers[(opcode & 0x0F00) >> 8] + registers[(opcode & 0x00F0) >> 4] > 0xFF;
                    registers[(opcode & 0x0F00) >> 8] += registers[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0005:
                    registers[0xf] = registers[(opcode & 0x0F00) >> 8] - registers[(opcode & 0x00F0) >> 4] < 0x0;
                    registers[(opcode & 0x0F00) >> 8] -= registers[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0006:
                    registers[0xf] = registers[(opcode & 0x0F00) >> 8] & 1;
                    registers[(opcode & 0x0F00) >> 8] = registers[(opcode & 0x0F00) >> 8] >> 1;
                    break;
                case 0x0007:
                    printf("  todo - %x not implemented...\n", opcode);
                    exit(0);
                    break;
                case 0x000e:
                    printf("  todo - %x not implemented...\n", opcode);
                    exit(0);
                    break;
                default:
                    printf("  %x not implemented...\n", opcode);
                    exit(0);
            }
            break;
        case 0x9000:
            if (registers[(opcode & 0x0F00) >> 8] != registers[(opcode & 0x00F0) >> 4]) {
                programCounter += 2;
            }
            break;
        case 0xa000:
            i = opcode & 0x0FFF;
            break;
        case 0xb000:
            programCounter = registers[0] + (opcode & 0x0FFF);
            increment = false;
            break;
        case 0xc000:
            registers[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            break;
        case 0xd000:
            draw(registers[(opcode & 0x0F00) >> 8], registers[(opcode & 0x00F0) >> 4], opcode & 0x000F);
            updateScreen = true;
            break;
        case 0xe000:
            switch (opcode & 0x00FF) {
                case 0x009e:
                    if (keyStates[registers[(opcode & 0x0F00) >> 8]]) {
                        programCounter += 2;
                    }
                    break;
                case 0x00a1:
                    if (!keyStates[registers[(opcode & 0x0F00) >> 8]]) {
                        programCounter += 2;
                    }
                    break;
                default:
                    printf("  %x not implemented...\n", opcode);
                    exit(0);
            }
            break;
        case 0xf000:
            switch (opcode & 0x00FF) {
                case 0x0007:
                    registers[(opcode & 0x0F00) >> 8] = delayTimer;
                    break;
                case 0x000A:
                    paused = true;
                    setRegister = (opcode & 0x0F00) >> 8;
                    printf("get key press...\n");
                    break;
                case 0x0015:
                    delayTimer = (opcode & 0x0F00) >> 8;
                    break;
                case 0x0018:
                    soundTimer = (opcode & 0x0F00) >> 8;
                    break;
                case 0x001e:
                    i += registers[(opcode & 0x0F00) >> 8];
                    break;
                case 0x0029:
                    i = registers[(opcode & 0x0F00) >> 8] * 5;
                    break;
                case 0x0033:
                    memory[i] = registers[(opcode & 0x0F00) >> 8] / 100;
                    memory[i+1] = (registers[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[i+2] = (registers[(opcode & 0x0F00) >> 8] % 100) % 10;
                    break;
                case 0x0055:
                    for (int x = 0; x <= (opcode & 0x0F00) >> 8; x++) {
                        memory[i+x] = registers[x];
                    }
                    break;
                case 0x0065:
                    for (int x = 0; x <= (opcode & 0x0F00) >> 8; x++) {
                        registers[x] = memory[i + x];
                    }
                    break;
                default:
                    printf("  %x not implemented...\n", opcode);
                    exit(0);
            }
            break;
        default:
            printf("  %x not implemented...\n", opcode);
            exit(0);
    }

    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
    if (increment) programCounter += 2;
}

void Chip8::printRom() {
    for (int x = PC_START; x < PC_START+romSize; x += 2) {
        opcode = (memory[x] << 8) | memory[x+1];
        printf("0x%04x: 0x%04x\n", x, opcode);
    }
}

void Chip8::printRegisters() {
    printf("\nRegisters\n");
    printf("PC: 0x%x\n", programCounter);
    printf("I: 0x%x\n", i);
    printf("SP: 0x%x\n\n", stackPointer);

    for (int x = 0; x < NUM_REGISTERS; x++) {
        printf("V%X: 0x%x\n", x, registers[x]);
    }
}

void Chip8::printMemory() {
    for (int x = 0; x < MEMORY_SIZE; x++) {
        printf("0x%x: 0x%x ", x, memory[x]);

        if (x % 15 == 0) printf("\n");
    }

    printf("\n");
}

void Chip8::printStack() {
    printf("\nsp: %d\n", stackPointer);

    for (int x = 0; x < STACK_SIZE; x++) {
        printf("%d: 0x%x\n", x, stack[x]);
    }
}

void Chip8::printMap() {
    for (int h = 0; h < MAP_HEIGHT; h++) {
        for (int w = 0; w < MAP_WIDTH; w++) {
            if (map[w][h]) printf("#");
            else printf("-");
        }
        printf("\n");
    }
}

void Chip8::printKeys() {
    for (int x = 0; x < NUM_KEYS; x++) {
        printf("%d: 0x%x\n", x, keyStates[x]);
    }
}