typedef unsigned char ui8;
typedef unsigned short ui16;
typedef unsigned long ui32;
typedef signed long i32;

enum
{
    C8_MEMORY_SIZE = 4096,
    C8_NUM_REGISTERS = 16,
    C8_SCREEN_WIDTH = 64,
    C8_SCREEN_WIDTH_SIZE = C8_SCREEN_WIDTH / 8,
    C8_SCREEN_HEIGHT = 32,
    C8_SCREEN_SIZE = C8_SCREEN_WIDTH_SIZE * C8_SCREEN_HEIGHT,
    C8_SCREEN_PIXELS = C8_SCREEN_WIDTH * C8_SCREEN_HEIGHT,
    C8_NUM_STACK_LEVELS = 16,
    C8_NUM_KEYS = 16,
    C8_NUM_FONTS = 16,
    C8_FONT_SIZE = 5,
    C8_ROM_PLACEMENT = 0x200,
};

enum
{
    C8_EVENT_DRAW = 0x01
};

/*
    Chip8 Keyboard Layout:
    1, 2, 3, C
    4, 5, 6, D
    7, 8, 9, E
    A, 0, B, F
*/
enum
{
    C8_KEY_0 = 0x0,
    C8_KEY_1 = 0x1,
    C8_KEY_2 = 0x2,
    C8_KEY_3 = 0x3,
    C8_KEY_4 = 0x4,
    C8_KEY_5 = 0x5,
    C8_KEY_6 = 0x6,
    C8_KEY_7 = 0x7,
    C8_KEY_8 = 0x8,
    C8_KEY_9 = 0x9,

    C8_KEY_A = 0xA,
    C8_KEY_B = 0xB,
    C8_KEY_C = 0xC,
    C8_KEY_D = 0xD,
    C8_KEY_E = 0xE,
    C8_KEY_F = 0xF
};

typedef struct Chip8
{
    ui8 memory[C8_MEMORY_SIZE];
    ui8 registers[C8_NUM_REGISTERS];

    ui16 opcode;
    ui16 index_register;
    ui16 program_counter;

    ui8 screen_memory[C8_SCREEN_SIZE];

    ui8 delay_timer;
    ui8 sound_timer;

    ui16 stack_levels[C8_NUM_STACK_LEVELS];
    ui16 stack_pointer;

    ui8 keys[C8_NUM_KEYS];
} Chip8;

typedef struct Chip8InputKey
{
    ui8 key_index;
    ui8 key_state;
} Chip8InputKey;

void chip8_init(Chip8 *chip8);
void chip8_load_rom(Chip8 *chip8, const char *rom_file_path);
void chip8_run_program(Chip8 *chip8, ui8 *event);
void chip8_update_timers(Chip8 *chip8);
void chip8_feed_input(Chip8 *chip8, const Chip8InputKey *keys, const ui8 num_keys);
void chip8_pixel_data(Chip8 *chip8, ui8 *pixels, const ui16 num_pixels);

ui8 chip8_screen_index(const ui8 x, const ui8 y);
ui16 chip8_pixel_index(const ui16 x, const ui16 y);