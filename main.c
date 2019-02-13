#include <stdio.h>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include "chip8.h"

typedef struct InputKey
{
    i32 virtual_key_code;
    ui8 virtual_key_state;
} InputKey;

typedef struct InputMap
{
    i32 virtual_key_code;
    ui8 chip8_key_index;
} InputMap;

const InputMap INPUT_MAP[NUM_KEYS] =
{
    { 0x31, C8_KEY_1 }, { 0x32, C8_KEY_2 }, { 0x33, C8_KEY_3 }, { 0x34, C8_KEY_C },
    { 0x51, C8_KEY_4 }, { 0x57, C8_KEY_5 }, { 0x45, C8_KEY_6 }, { 0x52, C8_KEY_D },
    { 0x41, C8_KEY_7 }, { 0x53, C8_KEY_8 }, { 0x44, C8_KEY_9 }, { 0x46, C8_KEY_E },
    { 0x5A, C8_KEY_A }, { 0x58, C8_KEY_0 }, { 0x43, C8_KEY_B }, { 0x56, C8_KEY_F }
};

void read_input(InputKey *keys, const ui8 num_keys, ui8 *num_available_keys);
void map_input(const InputKey *keys, const ui8 num_keys, Chip8InputKey *chip8_keys, const ui8 num_chip8_keys, ui8 *num_available_chip8_keys);

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

    InputKey input_keys[NUM_KEYS] = {0};
    Chip8InputKey chip8_input_keys[NUM_KEYS] = {0};

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

            ui8 num_available_input_keys = 0;
            read_input(input_keys, NUM_KEYS, &num_available_input_keys);
            ui8 num_available_chip8_input_keys = 0;
            map_input(input_keys, num_available_input_keys, chip8_input_keys, NUM_KEYS, &num_available_chip8_input_keys);
            
            chip8_feed_input(&chip8, chip8_input_keys, num_available_chip8_input_keys);
            chip8_tick(&chip8);
        }

        Sleep(1);
    }

    return 0;
}

void read_input(InputKey *keys, const ui8 num_keys, ui8 *num_available_keys)
{
    if(num_available_keys == NULL)
        return;

    *num_available_keys = 0;

    if(keys == NULL || num_keys == 0)
        return;

    enum { NUM_INPUT_RECORDS = 32 };
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD records[NUM_INPUT_RECORDS] = {0};
	DWORD num_records = NUM_INPUT_RECORDS;
	DWORD num_available_records = 0;

    PeekConsoleInput(handle, records, num_records, &num_available_records);
    for(ui8 i = 0; i < num_available_records && *num_available_keys < num_keys; ++i) {
        if(records[i].Event.KeyEvent.wVirtualKeyCode == 0)
            continue;

        //printf("key input: 0x%X state: %i (%i)\n", records[i].Event.KeyEvent.wVirtualKeyCode, records[i].Event.KeyEvent.bKeyDown, num_available_records);
        keys[*num_available_keys].virtual_key_code = records[i].Event.KeyEvent.wVirtualKeyCode;
        keys[*num_available_keys].virtual_key_state = (ui8)records[i].Event.KeyEvent.bKeyDown;
        (*num_available_keys)++;
    }
    FlushConsoleInputBuffer(handle);
}

void map_input(const InputKey *keys, const ui8 num_keys, Chip8InputKey *chip8_keys, const ui8 num_chip8_keys, ui8 *num_available_chip8_keys)
{
    if(num_available_chip8_keys == NULL)
        return;
    
    *num_available_chip8_keys = 0;

    if(keys == NULL || num_keys == 0 || chip8_keys == NULL || num_chip8_keys == 0)
        return;

    for(ui8 key = 0; key < num_keys && key < num_chip8_keys; ++key)
    {
        for(ui8 map = 0; map < NUM_KEYS; ++map)
        {
            if(keys[key].virtual_key_code == INPUT_MAP[map].virtual_key_code)
            {
                chip8_keys[key].key_index = INPUT_MAP[map].chip8_key_index;
                chip8_keys[key].key_state = keys[key].virtual_key_state;
                (*num_available_chip8_keys)++;
            }
        }
    }
}