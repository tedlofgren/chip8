#include <stdio.h>
#include <string.h>

typedef unsigned char ui8;
typedef unsigned short ui16;
typedef signed long i32;

enum
{
    MEMORY_SIZE = 4096,
    NUM_REGISTERS = 16,
    SCREEN_WIDTH = 64,
    SCREEN_HEIGHT = 32,
    SCREEN_SIZE = (SCREEN_WIDTH * SCREEN_HEIGHT) / 8,
    NUM_STACK_LEVELS = 16,
    NUM_KEYS = 16,
    NUM_FONTS = 16,
    FONT_SIZE = 5,
    ROM_PLACEMENT = 0x200,
    MAX_SPRITE_X_PIXELS = 8,
    MAX_SPRITE_Y_PIXELS = 15
};

typedef struct Chip8
{
    ui8 memory[MEMORY_SIZE];
    ui8 registers[NUM_REGISTERS];

    ui16 opcode;
    ui16 index_register;
    ui16 program_counter;

    ui8 screen_pixels[SCREEN_SIZE];

    ui8 delay_timer;
    ui8 sound_timer;

    ui16 stack_levels[NUM_STACK_LEVELS];
    ui16 stack_pointer;

    ui8 keys[NUM_KEYS];
} Chip8;

void chip8_setup_fonts(Chip8 *chip8);

void chip8_init(Chip8 *chip8)
{
    memset(chip8, 0, sizeof(Chip8));
    chip8_setup_fonts(chip8);
}

void chip8_setup_fonts(Chip8 *chip8)
{
    const ui8 font_memory[NUM_FONTS][FONT_SIZE] =
    {
        { 0xF0, 0x90, 0x90, 0x90, 0xF0 }, // "0"
        { 0x20, 0x60, 0x20, 0x20, 0x70 }, // "1"
        { 0xF0, 0x10, 0xF0, 0x80, 0xF0 }, // "2"
        { 0xF0, 0x10, 0xF0, 0x10, 0xF0 }, // "3"
        { 0x90, 0x90, 0xF0, 0x10, 0x10 }, // "4"
        { 0xF0, 0x80, 0xF0, 0x10, 0xF0 }, // "5"
        { 0xF0, 0x80, 0xF0, 0x90, 0xF0 }, // "6"
        { 0xF0, 0x10, 0x20, 0x40, 0x40 }, // "7"
        { 0xF0, 0x90, 0xF0, 0x90, 0xF0 }, // "8"
        { 0xF0, 0x90, 0xF0, 0x10, 0xF0 }, // "9"
        { 0xF0, 0x90, 0xF0, 0x90, 0x90 }, // "A"
        { 0xE0, 0x90, 0xE0, 0x90, 0xE0 }, // "B"
        { 0xF0, 0x80, 0x80, 0x80, 0xF0 }, // "C"
        { 0xE0, 0x90, 0x90, 0x90, 0xE0 }, // "D"
        { 0xF0, 0x80, 0xF0, 0x80, 0xF0 }, // "E"
        { 0xF0, 0x80, 0xF0, 0x80, 0x80 }, // "F"
    };
    memcpy(chip8->memory, font_memory, NUM_FONTS * FONT_SIZE);
}

void chip8_load_rom(Chip8 *chip8, const char *rom_file_path)
{
    FILE *rom = fopen(rom_file_path, "rb");
    if(rom == NULL)
    {
        printf("Failed to load rom `%s`", rom_file_path);
        return;
    }

    i32 data = EOF;
    ui8 *buffer = &chip8->memory[ROM_PLACEMENT];
    ui8 *buffer_end = &chip8->memory[MEMORY_SIZE];
    while((data = fgetc(rom)) != EOF && buffer != buffer_end)
    {
        *buffer = (ui8)data;
        buffer++;
    }

    chip8->program_counter = ROM_PLACEMENT;
}

void chip8_run_program(Chip8 *chip8);
void chip8_update_timers(Chip8 *chip8);

void chip8_tick(Chip8 *chip8)
{
    chip8_run_program(chip8);
    chip8_update_timers(chip8);
}

void chip8_run_program(Chip8 *chip8)
{
    // Construct opcode by merging m[n] and m[n+1]
    ui16 opcode = chip8->memory[chip8->program_counter] << 8 | chip8->memory[chip8->program_counter + 1];

    ui16 add_program_counter = 2;
    switch(opcode & 0xF000)
    {
        case 0x6000:
            const ui16 register_index = (opcode & 0x0F00) >> 8;
            const ui16 value = opcode & 0x00FF;
            chip8->registers[register_index] = (ui8)value;
            break;
        case 0xA000:
            chip8->index_register = (opcode & 0x0FFF);
            break;
        case 0xD000:
            const ui8 screen_x = chip8->registers[(opcode & 0x0F00) >> 8];
            const ui8 screen_y = chip8->registers[(opcode & 0x00F0) >> 4];
            const ui8 sprite_size_y = (ui8)(opcode & 0x000F);
            const ui8 *sprite = &chip8->memory[chip8->index_register];

            // TODO: This is wrong.
            // Need to offset x coordinate?
            // Y coordinate is just index into row
            // X coordinate is 8 pixel for one index, need to multiple by 8?
            // http://chip8.wikia.com/wiki/Instruction_Draw

            // Q: Draw pixel into it's own value instead of in shared value (bits)?

            // This wrapping is wrong?
            // Some say pixels should just be cut off. Instead of moving object to opposite side.
            const ui8 sprite_size_x = 1;
            const ui8 sprite_pixel_x = (screen_x + sprite_size_x) > SCREEN_WIDTH ? (screen_x + sprite_size_x) % SCREEN_WIDTH : screen_x;
            const ui8 sprite_pixel_y = (screen_y + sprite_size_y) > SCREEN_HEIGHT ? (screen_y + sprite_size_y) % SCREEN_HEIGHT : screen_y;

            ui8 check_collision = 1;
            for(ui8 i = 0; i < sprite_size_y; ++i)
            {
                const ui8 sprite_pixel_data = sprite[i];
                const ui8 screen_pixel_index = (sprite_pixel_x * sprite_pixel_y) / 8; // wrong
                const ui8 screen_pixel_data = chip8->screen_pixels[screen_pixel_index];
                ui8 new_screen_pixel_data = screen_pixel_data ^ sprite_pixel_data;

                for(ui8 j = 0; check_collision && j < MAX_SPRITE_X_PIXELS; ++j)
                {
                    // Check if pixel was set, but isn't no more.
                    if(screen_pixel_data & (1 << j) != 0 && new_screen_pixel_data & (1 << j) == 0)
                    {
                        check_collision = 0;
                        break;
                    }
                }

                const ui8 collision = check_collision == 0 ? 1 : 0;
                chip8->registers[0xF] = collision;
                chip8->screen_pixels[screen_pixel_index] = new_screen_pixel_data;
            }

            break;
        default:
            printf("Unsupported opcode 0%x", opcode);
    }

    chip8->program_counter += add_program_counter;
}

void chip8_update_timers(Chip8 *chip8)
{
    if(chip8->delay_timer > 0)
        --chip8->delay_timer;

    if(chip8->sound_timer > 0)
    {
        if(--chip8->sound_timer == 0)
            printf("BEEP");
    }
}