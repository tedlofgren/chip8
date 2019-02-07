#include <stdio.h>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include "chip8.h"

int main(int n_args, int **args)
{
    (void)n_args; (void)args;

    printf("Chip8 Emulator\n");

    Chip8 chip8 = {0};
    chip8_init(&chip8);
    chip8_load_rom(&chip8, "./roms/PONG");

    LARGE_INTEGER frequency = {0};
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER current_time = {0};
    QueryPerformanceCounter(&current_time);

    const float HZ = 60.f;
    const float SPEED = 1.f;

    float hz_timer = 0.f;
    ui8 run = 1;
    while(run) 
    {
        LARGE_INTEGER last_time = current_time;
        QueryPerformanceCounter(&current_time);
        float dt = (float)((double)(current_time.QuadPart - last_time.QuadPart) / 10000.f);

        hz_timer += (dt * SPEED);
        if(hz_timer >= (1000.0f / HZ)) {
            hz_timer = 0.f;
            chip8_tick(&chip8);
        }

        Sleep(1);
    }

    return 0;
}