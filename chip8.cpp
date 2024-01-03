#include "chip8.h"

chip8::chip8()
{

}

void chip8::initialize()
{
    PC = 0x200; //Set Program counter to location 0x200 as per Chip8 Specs
    SP = 0; //Set stack pointer to Zero (0)
    I = 0;  //Set Index register to Zero (0)

    //clear graphics array
    for(uint16_t i=0; i < (32*64); ++i) { gfx[i] = 0; }

    //clear stack, V registers and key array
    for(uint8_t i=0; i < 16; ++i) { stack[i] = V[i] = key[i] = 0; }

    //Store Font Set in Specified Location
    for(uint8_t i=0; i<80; i++) { rom[i+80] = chip8_fontset[i]; }

    drawFlag = true;    //Enable draw flag to draw on Screen
    delayTimer = 255;   //set delay timer to max value
    soundTimer = 255;   //set sound timer to max value
    srand(time(NULL));  //set srand time to null for random number generator
}

void chip8::loadProgram(string path)
{
    ifstream file;
    ifstream::pos_type size;

    //open file as input file in binary mode & set the pointer to end of file (ios::ate)
    file.open(path, ios::in | ios::binary | ios::ate);

    if(file.is_open())
    {
        char *memblock; //decalre memory to store rom file data temporarily

        size = file.tellg();    //Get the total size of the rom file

        //Check if ROM size can fit in the ROM memory or not, return if too big rom found.
        if(size > MAX_ROM_SIZE) { fscanf(stderr, "%c", "File size is too big."); return; }

        //Go back to the begining of the file before copying the file into memory block
        file.seekg(0, ios::beg);

        //allocate memory to given ROM file size
        memblock = new char [size];

        //read ROM file and copy into memblock
        file.read(memblock, size);

        //Copy the memory block into CPU ROM location, starting at 0x200
        for(uint16_t i = 0; i < size; i++)
        {
            rom[i + 0x200] = memblock[i];

#ifdef TERMINAL_DEBUG_ENABLED
            printf("%X ", rom[i + 0x200]);
#endif
        }

        //free memblock
        delete [] memblock;

        //close ROM file
        file.close();
    }
}

void chip8::execute()
{
    //declare 2Byte variable to store opcode
    uint16_t opCode = 0;

    //Fetch opcode
    opCode = (rom[PC] << 8) | rom[PC+1];

    switch(opCode & 0xF000)
    {
    // SYS addr 0nnn
    case 0x0000:
    {
       switch (opCode & 0x000F)
       {
       //cls clear the screen
       case 0x0000:
       {
           clearScreen();
           drawFlag = true;
           PC += 2;
       } break;

           //RET Return from a subroutine.
       case 0x000E:
       {
           PC = stack[--SP];
           PC += 2;
       } break;

       default:
           printf("Unknown OpCode %X", opCode);
           break;
       }
    } break;

        // 1nnn - JP addr
    case 0x1000:
    {
        PC = opCode & 0x0FFF;
    } break;

        // 2nnn - CALL addr
    case 0x2000:
    {
        stack[SP++] = PC;
        PC = opCode & 0x0FFF;
    } break;

        // 3xkk - SE Vx, byte
    case 0x3000:
    {
        //Skip next instruction if Vx = kk.
        uint8_t data = (opCode & 0x0FFF) & 0xFF;
        uint8_t x = ((opCode & 0x0FFF) >> 8) & 0x0F;
        if(V[x] == data) { PC += 4; }
        else {PC += 2;}
    } break;

        //4xkk - SNE Vx, byte
    case 0x4000:
    {
        //Skip next instruction if Vx != kk.
        uint8_t data = (opCode & 0x0FFF) & 0xFF;
        uint8_t x = ((opCode & 0x0FFF) >> 8) & 0x0F;
        if(V[x] != data) { PC = PC + 4; }   //skip if equal
        else {PC += 2;}
    } break;

        //5xy0 - SE Vx, Vy
    case 0x5000:
    {
        //Skip next instruction if Vx = Vy
        uint8_t y = ((opCode & 0x0FFF) & 0xF0) >> 4;
        uint8_t x = ((opCode & 0x0FFF) >> 8) & 0x0F;
        if(V[x] == V[y]){ PC = PC + 4; }   //skip if equal
        else {PC += 2;}
    } break;

        //6xkk - LD Vx, byte
    case 0x6000:
    {
        //Set Vx = kk.
        uint8_t data = (opCode & 0x00FF);
        uint8_t x = ((opCode & 0x0F00) >> 8);
        V[x] = data;
        PC += 2;
    } break;

        //7xkk - ADD Vx, byte
    case 0x7000:
    {
        //Set Vx = Vx + kk.
        uint8_t data = (opCode & 0x0FFF) & 0xFF;
        uint8_t x = ((opCode & 0x0FFF) >> 8) & 0x0F;
        V[x] = V[x] + data;
        PC += 2;
    } break;

        //8000
    case 0x8000:
    {
        uint8_t x = ((opCode & 0x0F00) >> 8);
        uint8_t y = (opCode & 0x00F0) >> 4;

        switch(opCode & 0x000F)
        {
        //8xy0 - LD Vx, Vy
        case 0:
        {
            //Set Vx = Vy.
            V[x] = V[y];
            PC += 2;
        } break;

            //8xy1 - OR Vx, Vy
        case 1:
        {
            //Set Vx = Vx OR Vy.
            V[x] = V[x] | V[y];
            PC += 2;
        } break;

            //8xy2 - AND Vx, Vy
        case 2:
        {
            //Set Vx = Vx AND Vy.
            V[x] = V[x] & V[y];
            PC += 2;
        } break;

            //8xy3 - XOR Vx, Vy
        case 3:
        {
            //Set Vx = Vx XOR Vy
            V[x] = V[x] ^ V[y];
            PC += 2;
        } break;

            //8xy4 - ADD Vx, Vy
        case 4:
        {
            //Set Vx = Vx + Vy, set VF = carry.
            uint16_t result = V[x] + V[y];
            if(result > 255) V[0xF] = 1;
            else V[0xF] = 0;

            V[x] = result & 0xFF;
            PC += 2;
        } break;

            //8xy5 - SUB Vx, Vy
        case 5:
        {
            //Set Vx = Vx - Vy, set VF = NOT borrow.
            if(V[x] > V[y]) V[0xF] = 1;
            else V[0xF] = 0;

            V[x] = V[x] - V[y];
            PC += 2;
        } break;

            //8xy6 - SHR Vx {, Vy}
        case 6:
        {
            //Set Vx = Vx SHR 1.
            V[0xF] = V[x] & 1;
            V[x] = V[x] >> 1;
            PC += 2;
        } break;

            //8xy7 - SUBN Vx, Vy
        case 7:
        {
            //Set Vx = Vy - Vx, set VF = NOT borrow.
            if(V[y] > V[x]) V[0xF] = 1;
            else V[0xF] = 0;

            V[x] = V[y] - V[x];
            PC += 2;
        } break;

            //8xyE - SHL Vx {, Vy}
        case 0xE:
        {
            //8xyE - SHL Vx {, Vy}
            V[0xF] = V[x] >> 7;
            V[x] = V[x] << 1;
            PC += 2;
        } break;

        default:
            printf("Unknown opCode %X", opCode);
            break;
        }
    } break;

        //9xy0 - SNE Vx, Vy
    case 0x9000:
    {
        //Skip next instruction if Vx != Vy.
        uint8_t y = ((opCode & 0x0FFF) & 0xF0) >> 4;
        uint8_t x = ((opCode & 0x0FFF) >> 8) & 0x0F;
        if(V[x] != V[y]) { PC += 4; }
        else {PC += 2;}
    } break;

        //Annn - LD I, addr
    case 0xA000:
    {
        //Set I = nnn.
        I = opCode & 0x0FFF;
        PC += 2;
    } break;

        //Bnnn - JP V0, addr
    case 0xB000:
    {
        //Jump to location nnn + V0.
        PC = (opCode & 0x0FFF) + V[0];
    } break;

        //Cxkk - RND Vx, byte
    case 0xC000:
    {
        //Set Vx = random byte AND kk
        uint8_t x = ((opCode & 0x0F00) >> 8);
        uint8_t data = (opCode & 0x00FF);
        V[x] = (rand()%0xFF) & data;
        PC += 2;
    } break;

        //Dxyn - DRW Vx, Vy, nibble
    case 0xD000:
    {
        uint8_t x = V[((opCode & 0x0F00) >> 8)];
        uint8_t y = V[((opCode & 0x00F0) >> 4)];
        uint8_t height = (opCode & 0x000F);
        uint8_t pixels;

        V[0xF] = 0;

        for(uint8_t ycol = 0; ycol < height; ycol++)
        {
            pixels = rom[I + ycol];
            for(uint8_t xrow = 0; xrow < 8; xrow++)
            {
                if((pixels & (0x80 >> xrow)) !=0)
                {
                    if(gfx[(x + xrow + ((y + ycol) * 64))] == 1)
                    { V[0xF] = 1; }
                    gfx[(x + xrow + ((y + ycol) * 64))] ^= 1;
                }
            }
        }
        drawFlag = true;
        PC += 2;
    } break;

        //Exnn
    case 0xE000:
    {
        switch (opCode & 0x00FF) {
            //Ex9E - SKP Vx
        case 0x009E:
        {
            //Skip next instruction if key with the value of Vx is pressed.
            if(key[V[(opCode & 0x0F00) >> 8]] != 0) {PC += 4;}
            else {PC += 2;}
        } break;

            //ExA1 - SKNP Vx
        case 0x00A1:
        {
            //Skip next instruction if key with the value of Vx is not pressed.

            if(key[V[(opCode & 0x0F00) >> 8]] == 0) {PC += 4;}
            else {PC += 2;}
        } break;
        default:
            printf("Unknown opcode %X\n", opCode);
            break;
        }
    } break;

        //Fx
    case 0xF000:
    {
        switch(opCode & 0x00FF)
        {
            //Fx07 - LD Vx, DT
        case 0x07:
        {
            //Set Vx = delay timer value.
            V[((opCode & 0x0F00) >> 8)] = delayTimer;
            PC += 2;
        } break;

            //Fx0A - LD Vx, K
        case 0x0A:
        {
            //Wait for a key press, store the value of the key in Vx.
            bool keypress = false;
            for(uint8_t i = 0; i < 16; ++i)
            {
                if(key[i] != 0)
                {
                    V[((opCode & 0x0F00) >> 8)] = i;
                    keypress = true;
                }
            }
            //if not pressed any key then return back and try again in next cycle
            if(!keypress)
                return;
            PC += 2;

        } break;

            //Fx15 - LD DT, Vx
        case 0x15:
        {
            //Set delay timer = Vx
            delayTimer = V[((opCode & 0x0F00) >> 8)];
            PC += 2;
        } break;

            //Fx18 - LD ST, Vx
        case 0x18:
        {
            //Set sound timer = Vx.
            soundTimer = V[((opCode & 0x0F00) >> 8)];
            PC += 2;
        } break;

            //Fx1E - ADD I, Vx
        case 0x1E:
        {
            //Set I = I + Vx.
            if(I + V[((opCode & 0x0F00) >> 8)] > 0xFFF)
            {
                V[0xF] = 1;
            }
            else
            {
                V[0xF] = 0;
            }
            I = I + V[((opCode & 0x0F00) >> 8)];
            PC += 2;
        } break;

            //Fx29 - LD F, Vx
        case 0x29:
        {
            //Set I = location of sprite for digit Vx
            I = V[((opCode & 0x0F00) >> 8)] * 0x5;
            PC += 2;
        } break;

            //Fx33 - LD B, Vx
        case 0x33:
        {
            rom[I] = V[(opCode & 0x0F00) >> 8] / 100;
            rom[I + 1] = (V[(opCode & 0x0F00) >> 8] / 10) % 10;
            rom[I + 2] = (V[(opCode & 0x0F00) >> 8] % 100) % 10;
            PC += 2;
        } break;

            //Fx55 - LD [I], Vx
        case 0x55:
        {
            //Store registers V0 through Vx in memory starting at location I.
            uint8_t n = (opCode & 0x0F00) >> 8;
            for(uint8_t i=0; i <= n; i++)
            {
                rom[I + i] = V[i];
            }
            I += n + 1;
            PC += 2;
        } break;

            //Fx65 - LD Vx, [I]
        case 0x65:
        {
            //Read registers V0 through Vx from memory starting at location I
            uint8_t n = (opCode & 0x0F00) >> 8;
            for(uint8_t i=0; i <= n; i++)
            {
                V[i] = rom[I + i];
            }
            I += n + 1;
            PC += 2;
        } break;

        default:
            printf("Unknown opcode %X\n",opCode);
            break;
        }
    } break;

    default:
        printf("Unknown opcode %X\n",opCode);
        break;
    }

    //decrement delay timer if greater than Zero (0)
    if(delayTimer > 0) { --delayTimer; }

    //Decrement sound timer if greater than Zero (0)
    if(soundTimer > 0)
    {
        if(soundTimer == 1)
        {
#ifdef TERMINAL_DEBUG_ENABLED
            printf("BEEP\n");
#endif
        }
        --soundTimer;
    }

    return;
}

#ifdef TERMINAL_DEBUG_ENABLED
void chip8::drawGraphics()
{
    system("cls");
    for(int y = 0; y < 32; ++y)
    {
        for(int x = 0; x < 64; ++x)
        {
            if(gfx[(y*64) + x] == 0)
                printf("X");
            else
                printf(" ");
        }
        printf("\n");
    }
    printf("\n");
    drawFlag = false;
    return;
}
#endif

void chip8::clearScreen()
{
    for(uint16_t i=0; i<(64 * 32); i++)
    {
        gfx[i] = 0;
    }
    return;
}
