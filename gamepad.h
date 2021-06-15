/* Copyright (C) Nemirtingas
 * This file is part of gamepad.
 *
 * gamepad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gamepad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gamepad.  If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cstdint>

namespace gamepad
{

struct gamepad_id_t
{
    union {
        struct {
            uint16_t vendorID;
            uint16_t productID;
            //uint8_t reserved[4];
        };
        uint64_t id;
    };
};

struct gamepad_type_t
{
    enum class type_e
    {
        Unknown,
        Xbox360,
        XboxOne,
        Switch,
        PS3,
        PS4
    };

    type_e type;
    const char* name;
};

struct stick_pos_t
{
    // Left  = -1.0f
    // Right =  1.0f
    float x;
    // Down  = -1.0f
    // Up    =  1.0f
    float y;
};

constexpr uint32_t max_connected_gamepads = 16;

constexpr int32_t success = 0;
constexpr int32_t failed = -1;
constexpr int32_t invalid_parameter = -2;

constexpr float gamepad_left_thumb_deadzone  = 0.1f;
constexpr float gamepad_right_thumb_deadzone = 0.15f;
constexpr float gamepad_trigger_threshold    = 0.12f;

constexpr uint32_t button_none           = 0x0000u;
constexpr uint32_t button_up             = 0x0001u;
constexpr uint32_t button_down           = 0x0002u;
constexpr uint32_t button_left           = 0x0004u;
constexpr uint32_t button_right          = 0x0008u;
constexpr uint32_t button_start          = 0x0010u;
constexpr uint32_t button_back           = 0x0020u;
constexpr uint32_t button_left_thumb     = 0x0040u;
constexpr uint32_t button_right_thumb    = 0x0080u;
constexpr uint32_t button_left_shoulder  = 0x0100u;
constexpr uint32_t button_right_shoulder = 0x0200u;
constexpr uint32_t button_guide          = 0x0400u;
constexpr uint32_t button_a              = 0x1000u;
constexpr uint32_t button_b              = 0x2000u;
constexpr uint32_t button_x              = 0x4000u;
constexpr uint32_t button_y              = 0x8000u;

constexpr inline bool are_all_pressed(uint32_t buttons, uint32_t button_mask)
{
    return ((buttons & button_mask) == button_mask);
}

constexpr inline bool is_any_pressed(uint32_t buttons, uint32_t button_mask)
{
    return ((buttons & button_mask) != gamepad::button_none);
}

constexpr inline float normalize_value(float min, float max, float value)
{
    return (value - min) / (max - min);
}

constexpr inline float denormalize_value(float min, float max, float value)
{
    return value * (max - min) + min;
}

constexpr inline float rerange_value(float src_min, float src_max, float dst_min, float dst_max, float value)
{
    return denormalize_value(dst_min, dst_max,
        normalize_value(src_min, src_max, value));
}

struct gamepad_state_t
{
    uint32_t buttons;
    stick_pos_t left_stick;
    stick_pos_t right_stick;
    float left_trigger;
    float right_trigger;
};

const gamepad_type_t& get_gamepad_type(gamepad_id_t const& id);
int32_t update_gamepad_state(uint32_t index);
int32_t get_gamepad_id(uint32_t index, gamepad_id_t* id);
int32_t get_gamepad_state(uint32_t index, gamepad_state_t* state);
// Normalized strength ([0.0, 1.0])
int32_t set_gamepad_vibration(uint32_t index, float left_strength, float right_strength);
int32_t set_gamepad_led(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

// If you feel like freeing resources before leaving, call this.
void free_gamepad_resources();

}