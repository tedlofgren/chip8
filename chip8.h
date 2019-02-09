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
    SCREEN_WIDTH_SIZE = SCREEN_WIDTH / 8,
    SCREEN_HEIGHT = 32,
    SCREEN_SIZE = SCREEN_WIDTH_SIZE * SCREEN_HEIGHT,
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

    ui8 screen_memory[SCREEN_SIZE];

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
        printf("Failed to load rom `%s`\n", rom_file_path);
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
    // Construct opcode
    ui16 opcode = chip8->memory[chip8->program_counter] << 8 | chip8->memory[chip8->program_counter + 1];

    ui16 add_program_counter = 2;
    switch(opcode & 0xF000)
    {
        case 0x00:
        {
            const ui8 sub_opcode = opcode & 0x00FF;
            switch(sub_opcode)
            {
                case 0xEE:
                    chip8->program_counter = chip8->stack_levels[chip8->stack_pointer--];
                    break;
                default:
                    printf("Unsupported sub_opcode 00%X\n", sub_opcode);
            }
            break;
        }
        case 0x2000:
        {
            const ui16 next_program_counter = opcode & 0x0FFF;
            chip8->stack_levels[++chip8->stack_pointer] = chip8->program_counter;
            chip8->program_counter = next_program_counter;
            break;
        }
        case 0x6000:
        {
            const ui16 register_index = (opcode & 0x0F00) >> 8;
            const ui16 value = opcode & 0x00FF;
            chip8->registers[register_index] = (ui8)value;
            break;
        }
        case 0x7000:
        {
            const ui16 register_index = (opcode & 0x0F00) >> 8;
            const ui16 value = opcode & 0x00FF;
            chip8->registers[register_index] += (ui8)value;
            break;
        }
        case 0xA000:
            chip8->index_register = (opcode & 0x0FFF);
            break;
        case 0xF000:
        {
            const ui16 register_index = (opcode & 0x0F00) >> 8;
            const ui8 sub_opcode = opcode & 0x00FF;
            switch(sub_opcode)
            {
                case 0x07:
                    chip8->registers[register_index] = chip8->delay_timer;
                    break;
                case 0x15:
                    chip8->delay_timer = chip8->registers[register_index];
                    break;
                case 0x18:
                    chip8->sound_timer = chip8->registers[register_index];
                    break;
                case 0x29:
                {
                    const ui8 font_sprite_index = chip8->registers[register_index] * FONT_SIZE;
                    chip8->index_register = font_sprite_index;
                    break;
                }
                case 0x33:
                {
                    const ui8 decimal_value = chip8->registers[register_index];
                    const ui8 hundreds = (decimal_value / 100) % 10;
                    const ui8 tens = (decimal_value / 10) % 10;
                    const ui8 ones = decimal_value % 10;
                    chip8->memory[chip8->index_register + 0] = hundreds;
                    chip8->memory[chip8->index_register + 1] = tens;
                    chip8->memory[chip8->index_register + 2] = ones;
                    break;
                }
                case 0x65:
                {
                    for(ui8 i = 0; i <= register_index; ++i)
                        chip8->registers[i] = chip8->memory[chip8->index_register + i];
                    break;
                }
                default:
                    printf("Unsupported sub_opcode Fx%X\n", sub_opcode);
            }
            break;
        }
        case 0xD000:
        {
            const ui8 screen_position_x = chip8->registers[(opcode & 0x0F00) >> 8];
            const ui8 screen_position_y = chip8->registers[(opcode & 0x00F0) >> 4];
            const ui8 sprite_height = (ui8)(opcode & 0x000F);
            const ui8 *sprite = &chip8->memory[chip8->index_register];

            // Clear collision
            chip8->registers[0xF] = 0;
     
            const ui8 sprite_offset_bits = screen_position_x & 0x7;
            for(ui8 y = 0; y < sprite_height; ++y)
            {
                const ui8 screen_index_y = (screen_position_y + y) * SCREEN_WIDTH_SIZE;

                // TODO: Ensure screen_index_y and screen_index_x is within screen or handle wrap-around

                const ui16 sprite_pixels = sprite[y] << 8 >> sprite_offset_bits;
                const ui8 sprite_pixel_groups[2] = { (sprite_pixels & 0xFF00) >> 8, (sprite_pixels & 0x00FF) >> 4 }; // TODO: Fix warning
                for(ui8 x = 0; x < 2; ++x)
                {
                    const ui8 screen_index_x = (screen_position_x / SCREEN_WIDTH_SIZE) + x;

                    const ui8 screen_pixel_group = chip8->screen_memory[screen_index_x + screen_index_y];
                    const ui8 sprite_pixel_group = sprite_pixel_groups[x];

                    const ui8 new_screen_pixel_group = screen_pixel_group ^ sprite_pixel_group;
                    chip8->screen_memory[screen_index_x + screen_index_y] = new_screen_pixel_group;

                    // Check for erased pixels (collision)
                    if(new_screen_pixel_group < screen_pixel_group)
                        chip8->registers[0xF] = 1;
                }
            }
            break;
        }
        default:
            printf("Unsupported opcode %X\n", opcode);
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
            printf("BEEP\n");
    }
}