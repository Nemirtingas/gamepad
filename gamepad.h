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

#include <array>
#include <cstdint>
#include <map>
#include <string>

struct gamepad_id_t
{
    uint16_t vendorID;
    uint16_t productID;
};

struct gamepad_id_less_t
{
    bool operator()(gamepad_id_t const& l, gamepad_id_t const& r) const
    {
        return (l.vendorID | (l.productID << sizeof(uint16_t))) < (r.vendorID | (r.productID << sizeof(uint16_t)));
    }
};

struct gamepad_type_t
{
    enum class type_e
    {
        Xbox360,
        XboxOne,
        Switch,
        PS3,
        PS4
    } type;
    std::string name;
};

struct stick_pos_t
{
    float x;
    float y;
};

class Gamepad
{
protected:
    Gamepad();

public:
    constexpr static uint8_t max_connected_gamepads = 16;

    gamepad_id_t id;

    bool  up            : 1;
    bool  down          : 1;
    bool  left          : 1;
    bool  right         : 1;
    bool  start         : 1;
    bool  back          : 1;
    bool  left_shoulder : 1;
    bool  right_shoulder: 1;
    bool  left_thumb    : 1;
    bool  right_thumb   : 1;
    bool  a             : 1;
    bool  b             : 1;
    bool  x             : 1;
    bool  y             : 1;
    bool guide          : 1;
    stick_pos_t left_stick;
    stick_pos_t right_stick;
    float left_trigger;
    float right_trigger;

    static std::array<Gamepad *const, max_connected_gamepads>& get_gamepads(bool redetect = true);
    static const std::map<gamepad_id_t, gamepad_type_t, gamepad_id_less_t> gamepads_ids;

    virtual ~Gamepad();

    virtual int GetXinputId() = 0;
    virtual bool RunFrame() = 0;
    virtual bool SetVibration(uint16_t left_speed, uint16_t right_speed) = 0;
    virtual bool SetLed(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual bool Enabled() = 0;
};
