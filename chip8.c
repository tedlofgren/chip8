#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void chip8_setup_fonts(Chip8 *chip8);

void chip8_init(Chip8 *chip8)
{
    memset(chip8, 0, sizeof(Chip8));
    chip8_setup_fonts(chip8);
    srand((ui32)time(0));
}

void chip8_setup_fonts(Chip8 *chip8)
{
    const ui8 font_memory[C8_NUM_FONTS][C8_FONT_SIZE] =
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
    memcpy(chip8->memory, font_memory, C8_NUM_FONTS * C8_FONT_SIZE);
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
    ui8 *buffer = &chip8->memory[C8_ROM_PLACEMENT];
    ui8 *buffer_end = &chip8->memory[C8_MEMORY_SIZE];
    while((data = fgetc(rom)) != EOF && buffer != buffer_end)
    {
        *buffer = (ui8)data;
        buffer++;
    }

    chip8->program_counter = C8_ROM_PLACEMENT;
}

void chip8_run_program(Chip8 *chip8, ui8 *event)
{
    // Construct opcode
    const ui16 opcode = chip8->memory[chip8->program_counter] << 8 | chip8->memory[chip8->program_counter + 1];

    ui8 local_event = 0;
    ui16 add_program_counter = 2;
    switch(opcode & 0xF000)
    {
        case 0x00:
        {
            const ui8 sub_opcode = opcode & 0x00FF;
            switch(sub_opcode)
            {
                case 0xE0:
                    memset(chip8->screen_memory, 0, C8_SCREEN_SIZE);
                    break;
                case 0xEE:
                    chip8->program_counter = chip8->stack_levels[chip8->stack_pointer--];
                    // `program_counter` still needs to be incremented by `add_program_counter` at end of execution
                    break;
                default:
                    printf("Unsupported opcode %X\n", opcode);
            }
            break;
        }
        case 0x1000:
        {
            const ui16 next_program_counter = opcode & 0x0FFF;
            chip8->program_counter = next_program_counter;
            add_program_counter = 0;
            break;
        }
        case 0x2000:
        {
            const ui16 next_program_counter = opcode & 0x0FFF;
            chip8->stack_levels[++chip8->stack_pointer] = chip8->program_counter;
            chip8->program_counter = next_program_counter;
            add_program_counter = 0;
            break;
        }
        case 0x3000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 value = opcode & 0x00FF;
            if(chip8->registers[register_index] == value)
                add_program_counter += 2;
            break;
        }
        case 0x4000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 value = opcode & 0x00FF;
            if(chip8->registers[register_index] != value)
                add_program_counter += 2;
            break;
        }
        case 0x6000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 value = opcode & 0x00FF;
            chip8->registers[register_index] = value;
            break;
        }
        case 0x7000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 value = opcode & 0x00FF;
            chip8->registers[register_index] += value;
            break;
        }
        case 0x8000:
        {
            const ui8 register_index_x = (opcode & 0x0F00) >> 8;
            const ui8 register_index_y = (opcode & 0x00F0) >> 4;
            const ui8 sub_opcode = opcode & 0x000F;
            switch(sub_opcode)
            {
                case 0x0:
                    chip8->registers[register_index_x] = chip8->registers[register_index_y];
                    break;
                case 0x1:
                    chip8->registers[register_index_x] |= chip8->registers[register_index_y];
                    break;
                case 0x2:
                    chip8->registers[register_index_x] &= chip8->registers[register_index_y];
                    break;
                case 0x3:
                    chip8->registers[register_index_x] ^= chip8->registers[register_index_y];
                    break;
                case 0x4:
                {
                    const ui16 value = chip8->registers[register_index_x] + chip8->registers[register_index_y];
                    chip8->registers[0xF] = value > 0xFF ? 1 : 0;
                    chip8->registers[register_index_x] = value & 0x00FF;
                    break;
                }
                case 0x5:
                {
                    const ui8 value_x = chip8->registers[register_index_x];
                    const ui8 value_y = chip8->registers[register_index_y];
                    chip8->registers[0xF] = value_x > value_y ? 1 : 0;
                    chip8->registers[register_index_x] = value_x - value_y;
                    break;
                }
                case 0x6:
                {
                    const ui8 value = chip8->registers[register_index_x];
                    chip8->registers[0xF] = value & 0x1;
                    chip8->registers[register_index_x] = value >> 1;
                    break;
                }
                case 0x7:
                {
                    const ui8 value_x = chip8->registers[register_index_x];
                    const ui8 value_y = chip8->registers[register_index_y];
                    chip8->registers[0xF] = value_y > value_x ? 1 : 0;
                    chip8->registers[register_index_x] = value_y - value_x;
                    break;
                }
                case 0xE:
                {
                    const ui8 value = chip8->registers[register_index_x];
                    chip8->registers[0xF] = value >> 7;
                    chip8->registers[register_index_x] = value << 1;
                    break;
                }
                default:
                    printf("Unsupported opcode %X\n", opcode);
            }  
            break;
        }
        case 0xA000:
            chip8->index_register = (opcode & 0x0FFF);
            break;
        case 0xF000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 sub_opcode = opcode & 0x00FF;
            switch(sub_opcode)
            {
                case 0x07:
                    chip8->registers[register_index] = chip8->delay_timer;
                    break;
                case 0x0A:
                {
                    add_program_counter = 0;
                    for(ui8 i = 0; i < C8_NUM_KEYS; ++i)
                    {
                        if(chip8->keys[i] == 1)
                        {
                            chip8->registers[register_index] = i;
                            add_program_counter = 2;
                            break;
                        }
                    }
                    break;
                }
                case 0x15:
                    chip8->delay_timer = chip8->registers[register_index];
                    break;
                case 0x18:
                    chip8->sound_timer = chip8->registers[register_index];
                    break;
                case 0x1E:
                {
                    const ui8 register_value = chip8->registers[register_index];
                    chip8->index_register += register_value;
                    break;
                }
                case 0x29:
                {
                    const ui8 font_sprite_index = chip8->registers[register_index] * C8_FONT_SIZE;
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
                case 0x55:
                {
                    for(ui8 i = 0; i <= register_index; ++i)
                        chip8->memory[chip8->index_register + i] = chip8->registers[i];
                    break;
                }
                case 0x65:
                {
                    for(ui8 i = 0; i <= register_index; ++i)
                        chip8->registers[i] = chip8->memory[chip8->index_register + i];
                    break;
                }
                default:
                    printf("Unsupported opcode %X\n", opcode);
            }
            break;
        }
        case 0xC000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 value = opcode & 0x00FF;
            const ui8 random_value = rand() % 0xFF;
            chip8->registers[register_index] = (ui8)(value & random_value);
            break;
        }
        case 0xD000:
        {
            const ui8 screen_position_x = chip8->registers[(opcode & 0x0F00) >> 8];
            const ui8 screen_position_y = chip8->registers[(opcode & 0x00F0) >> 4];
            const ui8 sprite_height = (ui8)(opcode & 0x000F);
            const ui8 *sprite = &chip8->memory[chip8->index_register];

            local_event = C8_EVENT_DRAW;

            // Clear collision
            chip8->registers[0xF] = 0;
     
            const ui8 sprite_offset_bits = screen_position_x & 0x7;
            for(ui8 y = 0; y < sprite_height; ++y)
            {
                const ui8 screen_index_y = (screen_position_y + y) * C8_SCREEN_WIDTH_SIZE;

                const ui16 sprite_pixels = sprite[y] << 8 >> sprite_offset_bits;
                const ui8 sprite_pixel_groups[2] = { (sprite_pixels & 0xFF00) >> 8, sprite_pixels & 0x00FF }; // TODO: Fix warning
                for(ui8 x = 0; x < 2; ++x)
                {
                    const ui8 screen_index_x = (screen_position_x / C8_SCREEN_WIDTH_SIZE + x) % C8_SCREEN_WIDTH_SIZE;

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
        case 0xE000:
        {
            const ui8 register_index = (opcode & 0x0F00) >> 8;
            const ui8 sub_opcode = opcode & 0x00FF;
            const ui8 key_index = chip8->registers[register_index];
            const ui8 key_state = chip8->keys[key_index];
            if((sub_opcode == 0x9E && key_state == 1) || (sub_opcode == 0xA1 && key_state == 0))
                add_program_counter += 2;
            break;
        }
        default:
            printf("Unsupported opcode %X\n", opcode);
    }

    if(event)
        *event = local_event;

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

void chip8_feed_input(Chip8 *chip8, const Chip8InputKey *keys, const ui8 num_keys)
{
    if(keys == NULL || num_keys == 0)
        return;
    
    for(ui8 i = 0; i < num_keys && i < C8_NUM_KEYS; ++i)
        chip8->keys[keys[i].key_index] = keys[i].key_state;
}

void chip8_pixel_data(Chip8 *chip8, ui8 *pixels, const ui16 num_pixels)
{
    if(pixels == NULL || num_pixels == 0)
        return;

    memset(pixels, 0, num_pixels);

    for(ui8 screen_position_y = 0; screen_position_y < C8_SCREEN_HEIGHT; ++screen_position_y)
    {
        const ui8 screen_index_y = screen_position_y * C8_SCREEN_WIDTH_SIZE;
        for(ui8 screen_position_x = 0; screen_position_x < C8_SCREEN_WIDTH; screen_position_x += 8)
        {
            const ui8 screen_index_x = screen_position_x / C8_SCREEN_WIDTH_SIZE;
            const ui8 C8_SCREEN_PIXELS = chip8->screen_memory[screen_index_x + screen_index_y];
            if(C8_SCREEN_PIXELS == 0)
                continue;

            for(ui8 pixel = 0; pixel < 8; ++pixel)
            {
                const ui16 pixel_index = (ui16)(screen_position_x + pixel) + (ui16)screen_position_y * C8_SCREEN_WIDTH;
                const ui8 pixel_bit = 0x80 >> pixel;
                pixels[pixel_index] = (C8_SCREEN_PIXELS & pixel_bit) != 0;
            }
        }
    }
}

ui8 chip8_screen_index(const ui8 x, const ui8 y)
{
    return (x / C8_SCREEN_WIDTH_SIZE) + y * C8_SCREEN_WIDTH_SIZE;
}

ui16 chip8_pixel_index(const ui16 x, const ui16 y)
{
    return x + y * C8_SCREEN_WIDTH;
}