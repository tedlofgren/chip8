#include <stdio.h>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include "chip8.h"

typedef struct InputKey
{
    ui8 virtual_key_code;
    ui8 virtual_key_state;
} InputKey;

typedef struct InputMap
{
    ui8 virtual_key_code;
    ui8 chip8_key_index;
} InputMap;

const InputMap INPUT_MAP[NUM_KEYS] =
{
    { 0x31, C8_KEY_1 }, { 0x32, C8_KEY_2 }, { 0x33, C8_KEY_3 }, { 0x34, C8_KEY_C }, // 1:1, 2:2, 3:3, 4:C
    { 0x51, C8_KEY_4 }, { 0x57, C8_KEY_5 }, { 0x45, C8_KEY_6 }, { 0x52, C8_KEY_D }, // Q:4, W:5, E:6, R:D
    { 0x41, C8_KEY_7 }, { 0x53, C8_KEY_8 }, { 0x44, C8_KEY_9 }, { 0x46, C8_KEY_E }, // A:7, S:8, D:9, F:E
    { 0x5A, C8_KEY_A }, { 0x58, C8_KEY_0 }, { 0x43, C8_KEY_B }, { 0x56, C8_KEY_F }  // Z:A, X:0, C:B, V:F
};

typedef struct KeyboardProcIO
{
    InputKey *keys;
    ui8 num_keys;
    ui8 *num_available_keys;
} KeyboardProcIO;

KeyboardProcIO *_keyboard_proc_io = NULL;
LRESULT CALLBACK _keyboard_proc_func(int nCode, WPARAM wParam, LPARAM lParam);

void read_input(InputKey *keys, const ui8 num_keys, ui8 *num_available_keys);
void map_input(const InputKey *keys, const ui8 num_keys, Chip8InputKey *chip8_keys, const ui8 num_chip8_keys, ui8 *num_available_chip8_keys);
void draw(ui8 *pixels, const ui16 num_pixels);

int main(int n_args, int **args)
{
    (void)n_args; (void)args;

    SetWindowsHookEx(WH_KEYBOARD_LL, _keyboard_proc_func, GetModuleHandle(NULL), 0);

    Chip8 chip8 = {0};
    chip8_init(&chip8);
    chip8_load_rom(&chip8, "./roms/PONG");

    LARGE_INTEGER current_time = {0}, last_time = {0};
    QueryPerformanceCounter(&current_time);

    const float HZ = 60.f;
    const float SPEED = 1.f;

    InputKey input_keys[NUM_KEYS] = {0};
    Chip8InputKey chip8_input_keys[NUM_KEYS] = {0};
    ui8 pixels[SCREEN_PIXELS] = {0};

    float hz_timer = 0.f;
    ui8 run = 1;
    while(run) 
    {
        last_time = current_time;
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

            chip8_pixel_data(&chip8, pixels, SCREEN_PIXELS);
            draw(pixels, SCREEN_PIXELS);
        }

        Sleep(1);
    }

    return 0;
}

LRESULT CALLBACK _keyboard_proc_func(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0 || _keyboard_proc_io == NULL)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

    InputKey *keys = _keyboard_proc_io->keys;
    const ui8 num_keys = _keyboard_proc_io->num_keys;
    ui8 *num_available_keys = _keyboard_proc_io->num_available_keys;

    if(*num_available_keys == num_keys)
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    const KBDLLHOOKSTRUCT *info = (KBDLLHOOKSTRUCT*)lParam;
    const ui8 virtual_key_code = (ui8)info->vkCode;
    const ui8 virtual_key_state = ((ui8)info->flags & LLKHF_UP) ? 0 : 1;
    //printf("proc: key input: 0x%X state: %i\n", virtual_key_code, *num_available_keys);

    keys[*num_available_keys].virtual_key_code = virtual_key_code;
    keys[*num_available_keys].virtual_key_state = virtual_key_state;
    (*num_available_keys)++;

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void read_input(InputKey *keys, const ui8 num_keys, ui8 *num_available_keys)
{
    if(num_available_keys == NULL)
        return;

    *num_available_keys = 0;

    if(keys == NULL || num_keys == 0)
        return;

    KeyboardProcIO keyboard_proc_io = { keys, num_keys, num_available_keys };
    _keyboard_proc_io = &keyboard_proc_io;
    {
        HWND window = GetConsoleWindow();
        MSG msg = {0};
        while (PeekMessage(&msg, window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    _keyboard_proc_io = NULL;
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

void draw(ui8 *pixels, const ui16 num_pixels)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT window_size = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SetConsoleWindowInfo(handle, TRUE, &window_size);

    COORD screen_buffer_size = { SCREEN_WIDTH, SCREEN_HEIGHT };
    SetConsoleScreenBufferSize(handle, screen_buffer_size);

    CHAR_INFO console_buffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {0};
    for(ui16 i = 0; i < num_pixels; ++i)
    {
        if(pixels[i] != 0)
            console_buffer[i].Attributes = BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY;
    }

    COORD buffer_start = {0,0};
    SMALL_RECT buffer_rect = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 } ; 
    WriteConsoleOutputA(handle, console_buffer, screen_buffer_size, buffer_start, &buffer_rect);
}