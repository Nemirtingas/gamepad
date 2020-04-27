/* Copyright (C) 2019 Nemirtingas
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

#include "gamepad.h"
#include <mutex>

constexpr decltype(Gamepad::max_connected_gamepads) Gamepad::max_connected_gamepads;

static std::array<Gamepad*, Gamepad::max_connected_gamepads> gamepads;

static std::recursive_mutex gamepad_mutex;

Gamepad::Gamepad() :
    id{ 0, 0 },
    up(false), down(false),
    left(false), right(false),
    start(false), back(false),
    left_shoulder(false), right_shoulder(false),
    left_thumb(false), right_thumb(false),
    a(false), b(false),
    x(false), y(false),
    guide(false),
    left_stick{ 0.0f, 0.0f }, right_stick{ 0.0f, 0.0f },
    left_trigger(0.0f), right_trigger(0.0f)
{}

Gamepad::~Gamepad()
{}

static constexpr inline float normalize_value(float min, float max, float value)
{
    return (value - min) / (max - min);
}

static constexpr inline float denormalize_value(float min, float max, float value)
{
    return value * (max - min) + min;
}

static constexpr inline float rerange_value(float src_min, float src_max, float dst_min, float dst_max, float value)
{
    return denormalize_value(dst_min, dst_max,
        normalize_value(src_min, src_max, value));
}

const std::map<gamepad_id_t, gamepad_type_t, gamepad_id_less_t> Gamepad::gamepads_ids = {
    {{0x0079, 0x18d4}, {gamepad_type_t::type_e::Xbox360, "GPD Win 2 X-Box Controller"}},
    {{0x044f, 0xb326}, {gamepad_type_t::type_e::Xbox360, "Thrustmaster Gamepad GP XID"}},
    {{0x045e, 0x028e}, {gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad"}},
    {{0x045e, 0x028f}, {gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad v2"}},
    {{0x045e, 0x0291}, {gamepad_type_t::type_e::Xbox360, "Xbox 360 Wireless Receiver (XBOX)"}},
    {{0x045e, 0x02a0}, {gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 Big Button IR"}},
    {{0x045e, 0x02a1}, {gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad"}},
    {{0x045e, 0x02dd}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One pad"}},
    {{0x044f, 0xb326}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One pad (Firmware 2015)"}},
    {{0x045e, 0x02e0}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad (Bluetooth)"}},
    {{0x045e, 0x02e3}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One Elite pad"}},
    {{0x045e, 0x02ea}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad"}},
    {{0x045e, 0x02fd}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad (Bluetooth)"}},
    {{0x045e, 0x02ff}, {gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One Elite pad"}},
    {{0x045e, 0x0719}, {gamepad_type_t::type_e::Xbox360, "Xbox 360 Wireless Receiver"}},
    {{0x046d, 0xc21d}, {gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F310"}},
    {{0x046d, 0xc21e}, {gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F510"}},
    {{0x046d, 0xc21f}, {gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F710"}},
    {{0x046d, 0xc242}, {gamepad_type_t::type_e::Xbox360, "Logitech Chillstream Controller"}},
    {{0x0f0d, 0x00c1}, {gamepad_type_t::type_e::Switch, "HORI Pad Switch"}},
    {{0x0f0d, 0x0092}, {gamepad_type_t::type_e::Switch, "HORI Pokken Tournament DX Pro Pad"}},
    {{0x0f0d, 0x00f6}, {gamepad_type_t::type_e::Switch, "HORI Wireless Switch Pad"}},
    {{0x0f0d, 0x00dc}, {gamepad_type_t::type_e::Switch, "HORI Battle Pad"}},
    {{0x20d6, 0xa711}, {gamepad_type_t::type_e::Switch, "PowerA Wired Controller Plus/PowerA Wired Gamcube Controller"}},
    {{0x0e6f, 0x0185}, {gamepad_type_t::type_e::Switch, "PDP Wired Fight Pad Pro for Nintendo Switch"}},
    {{0x0e6f, 0x0180}, {gamepad_type_t::type_e::Switch, "PDP Faceoff Wired Pro Controller for Nintendo Switch"}},
    {{0x0e6f, 0x0181}, {gamepad_type_t::type_e::Switch, "PDP Faceoff Deluxe Wired Pro Controller for Nintendo Switch"}},
    {{0x054c, 0x0268}, {gamepad_type_t::type_e::PS3, "Sony PS3 Controller"}},
    {{0x0925, 0x0005}, {gamepad_type_t::type_e::PS3, "Sony PS3 Controller"}},
    {{0x8888, 0x0308}, {gamepad_type_t::type_e::PS3, "Sony PS3 Controller"}},
    {{0x1a34, 0x0836}, {gamepad_type_t::type_e::PS3, "Afterglow PS3"}},
    {{0x0f0d, 0x006e}, {gamepad_type_t::type_e::PS3, "HORI horipad4 PS3"}},
    {{0x0f0d, 0x0066}, {gamepad_type_t::type_e::PS3, "HORI horipad4 PS4"}},
    {{0x0f0d, 0x005f}, {gamepad_type_t::type_e::PS3, "HORI Fighting commander PS3"}},
    {{0x0f0d, 0x005e}, {gamepad_type_t::type_e::PS3, "HORI Fighting commander PS4"}},
    {{0x0738, 0x8250}, {gamepad_type_t::type_e::PS3, "Madcats Fightpad Pro PS4"}},
    {{0x0079, 0x181a}, {gamepad_type_t::type_e::PS3, "Venom Arcade Stick"}},
    {{0x0079, 0x0006}, {gamepad_type_t::type_e::PS3, "PC Twin Shock Controller"}},
    {{0x2563, 0x0523}, {gamepad_type_t::type_e::PS3, "Digiflip GP006"}},
    {{0x11ff, 0x3331}, {gamepad_type_t::type_e::PS3, "SRXJ-PH2400"}},
    {{0x20bc, 0x5500}, {gamepad_type_t::type_e::PS3, "ShanWan PS3"}},
    {{0x044f, 0xb315}, {gamepad_type_t::type_e::PS3, "Firestorm Dual Analog 3"}},
    {{0x0f0d, 0x004d}, {gamepad_type_t::type_e::PS3, "Horipad 3"}},
    {{0x0f0d, 0x0009}, {gamepad_type_t::type_e::PS3, "HORI BDA GP1"}},
    {{0x0e8f, 0x0008}, {gamepad_type_t::type_e::PS3, "Green Asia"}},
    {{0x0f0d, 0x006a}, {gamepad_type_t::type_e::PS3, "Real Arcade Pro 4"}},
    {{0x0e6f, 0x011e}, {gamepad_type_t::type_e::PS3, "Rock Candy PS4"}},
    {{0x0e6f, 0x0214}, {gamepad_type_t::type_e::PS3, "Afterglow PS3"}},
    {{0x056e, 0x2013}, {gamepad_type_t::type_e::PS3, "JC-U4113SBK"}},
    {{0x0738, 0x8838}, {gamepad_type_t::type_e::PS3, "Madcatz Fightstick Pro"}},
    {{0x1a34, 0x0836}, {gamepad_type_t::type_e::PS3, "Afterglow PS3"}},
    {{0x0f30, 0x1100}, {gamepad_type_t::type_e::PS3, "Quanba Q1 fight stick"}},
    {{0x0f0d, 0x0087}, {gamepad_type_t::type_e::PS3, "HORI fighting mini stick"}},
    {{0x8380, 0x0003}, {gamepad_type_t::type_e::PS3, "BTP 2163"}},
    {{0x1345, 0x1000}, {gamepad_type_t::type_e::PS3, "PS2 ACME GA-D5"}},
    {{0x0e8f, 0x3075}, {gamepad_type_t::type_e::PS3, "SpeedLink Strike FX"}},
    {{0x0e6f, 0x0128}, {gamepad_type_t::type_e::PS3, "Rock Candy PS3"}},
    {{0x2c22, 0x2000}, {gamepad_type_t::type_e::PS3, "Quanba Drone"}},
    {{0x06a3, 0xf622}, {gamepad_type_t::type_e::PS3, "Cyborg V3"}},
    {{0x044f, 0xd007}, {gamepad_type_t::type_e::PS3, "Thrustmaster wireless 3-1"}},
    {{0x25f0, 0x83c3}, {gamepad_type_t::type_e::PS3, "Gioteck vx2"}},
    {{0x05b8, 0x1006}, {gamepad_type_t::type_e::PS3, "JC-U3412SBK"}},
    {{0x20d6, 0x576d}, {gamepad_type_t::type_e::PS3, "Power A PS3"}},
    {{0x0e6f, 0x1314}, {gamepad_type_t::type_e::PS3, "PDP Afterglow Wireless PS3 controller"}},
    {{0x0738, 0x3180}, {gamepad_type_t::type_e::PS3, "Mad Catz Alpha PS3 mode"}},
    {{0x0738, 0x8180}, {gamepad_type_t::type_e::PS3, "Mad Catz Alpha PS4 mode"}},
    {{0x0e6f, 0x0203}, {gamepad_type_t::type_e::PS3, "Victrix Pro FS"}},
    {{0x054c, 0x05c4}, {gamepad_type_t::type_e::PS4, "Sony PS4 Controller"}},
    {{0x054c, 0x09cc}, {gamepad_type_t::type_e::PS4, "Sony PS4 Slim Controller"}},
    {{0x054c, 0x0ba0}, {gamepad_type_t::type_e::PS4, "Sony PS4 Controller (Wireless dongle)"}},
    {{0x0f0d, 0x008a}, {gamepad_type_t::type_e::PS4, "HORI Real Arcade Pro 4"}},
    {{0x0f0d, 0x0055}, {gamepad_type_t::type_e::PS4, "HORIPAD 4 FPS"}},
    {{0x0f0d, 0x0066}, {gamepad_type_t::type_e::PS4, "HORIPAD 4 FPS Plus"}},
    {{0x0738, 0x8384}, {gamepad_type_t::type_e::PS4, "HORIPAD 4 FPS Plus"}},
    {{0x0738, 0x8250}, {gamepad_type_t::type_e::PS4, "Mad Catz FightPad Pro PS4"}},
    {{0x0738, 0x8384}, {gamepad_type_t::type_e::PS4, "Mad Catz Fightstick TE S+"}},
    {{0x0C12, 0x0E10}, {gamepad_type_t::type_e::PS4, "Armor Armor 3 Pad PS4"}},
    {{0x0C12, 0x1CF6}, {gamepad_type_t::type_e::PS4, "EMIO PS4 Elite Controller"}},
    {{0x1532, 0x1000}, {gamepad_type_t::type_e::PS4, "Razer Raiju PS4 Controller"}},
    {{0x1532, 0X0401}, {gamepad_type_t::type_e::PS4, "Razer Panthera PS4 Controller"}},
    {{0x054c, 0x05c5}, {gamepad_type_t::type_e::PS4, "STRIKEPAD PS4 Grip Add-on"}},
    {{0x146b, 0x0d01}, {gamepad_type_t::type_e::PS4, "Nacon Revolution Pro Controller"}},
    {{0x146b, 0x0d02}, {gamepad_type_t::type_e::PS4, "Nacon Revolution Pro Controller V2"}},
    {{0x0f0d, 0x00a0}, {gamepad_type_t::type_e::PS4, "HORI TAC4"}},
    {{0x0f0d, 0x009c}, {gamepad_type_t::type_e::PS4, "HORI TAC PRO"}},
    {{0x0c12, 0x0ef6}, {gamepad_type_t::type_e::PS4, "Hitbox Arcade Stick"}},
    {{0x0079, 0x181b}, {gamepad_type_t::type_e::PS4, "Venom Arcade Stick"}},
    {{0x0738, 0x3250}, {gamepad_type_t::type_e::PS4, "Mad Catz FightPad PRO"}},
    {{0x0f0d, 0x00ee}, {gamepad_type_t::type_e::PS4, "HORI mini wired gamepad"}},
    {{0x0738, 0x8481}, {gamepad_type_t::type_e::PS4, "Mad Catz FightStick TE 2+ PS4"}},
    {{0x0738, 0x8480}, {gamepad_type_t::type_e::PS4, "Mad Catz FightStick TE 2"}},
    {{0x7545, 0x0104}, {gamepad_type_t::type_e::PS4, "Armor 3, Level Up Cobra"}},
    {{0x1532, 0x1007}, {gamepad_type_t::type_e::PS4, "Razer Raiju 2 Tournament Edition (USB)"}},
    {{0x1532, 0x100A}, {gamepad_type_t::type_e::PS4, "Razer Raiju 2 Tournament Edition (BT)"}},
    {{0x1532, 0x1004}, {gamepad_type_t::type_e::PS4, "Razer Raiju 2 Ultimate Edition (USB)"}},
    {{0x1532, 0x1009}, {gamepad_type_t::type_e::PS4, "Razer Raiju 2 Ultimate Edition (BT)"}},
    {{0x1532, 0x1008}, {gamepad_type_t::type_e::PS4, "Razer Panthera Evo Fightstick"}},
    {{0x9886, 0x0025}, {gamepad_type_t::type_e::PS4, "Astro C40"}},
    {{0x0c12, 0x0e15}, {gamepad_type_t::type_e::PS4, "Game:Pad 4"}},
    {{0x4001, 0x0104}, {gamepad_type_t::type_e::PS4, "PS4 Fun Controller"}},
    {{0x056e, 0x2004}, {gamepad_type_t::type_e::Xbox360, "Elecom JC-U3613M"}},
    {{0x06a3, 0xf51a}, {gamepad_type_t::type_e::Xbox360, "Saitek P3600"}},
    {{0x0738, 0x4716}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Wired Xbox 360 Controller"}},
    {{0x0738, 0x4718}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV FightStick SE"}},
    {{0x0738, 0x4726}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox 360 Controller"}},
    {{0x0738, 0x4728}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV FightPad"}},
    {{0x0738, 0x4736}, {gamepad_type_t::type_e::Xbox360, "Mad Catz MicroCon Gamepad"}},
    {{0x0738, 0x4738}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Wired Xbox 360 Controller (SFIV)"}},
    {{0x0738, 0x4740}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Beat Pad"}},
    {{0x0738, 0x4a01}, {gamepad_type_t::type_e::XboxOne, "Mad Catz FightStick TE 2"}},
    {{0x0738, 0xb726}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox controller - MW2"}},
    {{0x0738, 0xbeef}, {gamepad_type_t::type_e::Xbox360, "Mad Catz JOYTECH NEO SE Advanced GamePad"}},
    {{0x0738, 0xcb02}, {gamepad_type_t::type_e::Xbox360, "Saitek Cyborg Rumble Pad - PC/Xbox 360"}},
    {{0x0738, 0xcb03}, {gamepad_type_t::type_e::Xbox360, "Saitek P3200 Rumble Pad - PC/Xbox 360"}},
    {{0x0738, 0xf738}, {gamepad_type_t::type_e::Xbox360, "Super SFIV FightStick TE S"}},
    {{0x0e6f, 0x0105}, {gamepad_type_t::type_e::Xbox360, "HSM3 Xbox360 dancepad"}},
    {{0x0e6f, 0x0113}, {gamepad_type_t::type_e::Xbox360, "Afterglow AX.1 Gamepad for Xbox 360"}},
    {{0x0e6f, 0x011f}, {gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad Wired Controller"}},
    {{0x0e6f, 0x0133}, {gamepad_type_t::type_e::Xbox360, "Xbox 360 Wired Controller"}},
    {{0x0e6f, 0x0139}, {gamepad_type_t::type_e::XboxOne, "Afterglow Prismatic Wired Controller"}},
    {{0x0e6f, 0x013a}, {gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"}},
    {{0x0e6f, 0x0146}, {gamepad_type_t::type_e::XboxOne, "Rock Candy Wired Controller for Xbox One"}},
    {{0x0e6f, 0x0147}, {gamepad_type_t::type_e::XboxOne, "PDP Marvel Xbox One Controller"}},
    {{0x0e6f, 0x015c}, {gamepad_type_t::type_e::XboxOne, "PDP Xbox One Arcade Stick"}},
    {{0x0e6f, 0x0161}, {gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"}},
    {{0x0e6f, 0x0162}, {gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"}},
    {{0x0e6f, 0x0163}, {gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"}},
    {{0x0e6f, 0x0164}, {gamepad_type_t::type_e::XboxOne, "PDP Battlefield One"}},
    {{0x0e6f, 0x0165}, {gamepad_type_t::type_e::XboxOne, "PDP Titanfall 2"}},
    {{0x0e6f, 0x0201}, {gamepad_type_t::type_e::Xbox360, "Pelican PL-3601 'TSZ' Wired Xbox 360 Controller"}},
    {{0x0e6f, 0x0213}, {gamepad_type_t::type_e::Xbox360, "Afterglow Gamepad for Xbox 360"}},
    {{0x0e6f, 0x021f}, {gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad for Xbox 360"}},
    {{0x0e6f, 0x0246}, {gamepad_type_t::type_e::XboxOne, "Rock Candy Gamepad for Xbox One 2015"}},
    {{0x0e6f, 0x0301}, {gamepad_type_t::type_e::Xbox360, "Logic3 Controller"}},
    {{0x0e6f, 0x0346}, {gamepad_type_t::type_e::XboxOne, "Rock Candy Gamepad for Xbox One 2016"}},
    {{0x0e6f, 0x0401}, {gamepad_type_t::type_e::Xbox360, "Logic3 Controller"}},
    {{0x0e6f, 0x0413}, {gamepad_type_t::type_e::Xbox360, "Afterglow AX.1 Gamepad for Xbox 360"}},
    {{0x0e6f, 0x0501}, {gamepad_type_t::type_e::Xbox360, "PDP Xbox 360 Controller"}},
    {{0x0e6f, 0xf900}, {gamepad_type_t::type_e::Xbox360, "PDP Afterglow AX.1"}},
    {{0x0f0d, 0x000a}, {gamepad_type_t::type_e::Xbox360, "Hori Co. DOA4 FightStick"}},
    {{0x0f0d, 0x000c}, {gamepad_type_t::type_e::Xbox360, "Hori PadEX Turbo"}},
    {{0x0f0d, 0x000d}, {gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick EX2"}},
    {{0x0f0d, 0x0016}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.EX"}},
    {{0x0f0d, 0x001b}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro VX"}},
    {{0x0f0d, 0x0063}, {gamepad_type_t::type_e::XboxOne, "Hori Real Arcade Pro Hayabusa (USA) Xbox One"}},
    {{0x0f0d, 0x0067}, {gamepad_type_t::type_e::XboxOne, "HORIPAD ONE"}},
    {{0x0f0d, 0x0078}, {gamepad_type_t::type_e::XboxOne, "Hori Real Arcade Pro V Kai Xbox One"}},
    {{0x11c9, 0x55f0}, {gamepad_type_t::type_e::Xbox360, "Nacon GC-100XF"}},
    {{0x12ab, 0x0004}, {gamepad_type_t::type_e::Xbox360, "Honey Bee Xbox360 dancepad"}},
    {{0x12ab, 0x0301}, {gamepad_type_t::type_e::Xbox360, "PDP AFTERGLOW AX.1"}},
    {{0x12ab, 0x0303}, {gamepad_type_t::type_e::Xbox360, "Mortal Kombat Klassic FightStick"}},
    {{0x1430, 0x4748}, {gamepad_type_t::type_e::Xbox360, "RedOctane Guitar Hero X-plorer"}},
    {{0x1430, 0xf801}, {gamepad_type_t::type_e::Xbox360, "RedOctane Controller"}},
    {{0x146b, 0x0601}, {gamepad_type_t::type_e::Xbox360, "BigBen Interactive XBOX 360 Controller"}},
    {{0x1532, 0x0037}, {gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"}},
    {{0x1532, 0x0a00}, {gamepad_type_t::type_e::XboxOne, "Razer Atrox Arcade Stick"}},
    {{0x1532, 0x0a03}, {gamepad_type_t::type_e::XboxOne, "Razer Wildcat"}},
    {{0x15e4, 0x3f00}, {gamepad_type_t::type_e::Xbox360, "Power A Mini Pro Elite"}},
    {{0x15e4, 0x3f0a}, {gamepad_type_t::type_e::Xbox360, "Xbox Airflo wired controller"}},
    {{0x15e4, 0x3f10}, {gamepad_type_t::type_e::Xbox360, "Batarang Xbox 360 controller"}},
    {{0x162e, 0xbeef}, {gamepad_type_t::type_e::Xbox360, "Joytech Neo-Se Take2"}},
    {{0x1689, 0xfd00}, {gamepad_type_t::type_e::Xbox360, "Razer Onza Tournament Edition"}},
    {{0x1689, 0xfd01}, {gamepad_type_t::type_e::Xbox360, "Razer Onza Classic Edition"}},
    {{0x1689, 0xfe00}, {gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"}},
    {{0x1bad, 0x0002}, {gamepad_type_t::type_e::Xbox360, "Harmonix Rock Band Guitar"}},
    {{0x1bad, 0x0003}, {gamepad_type_t::type_e::Xbox360, "Harmonix Rock Band Drumkit"}},
    {{0x1bad, 0xf016}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox 360 Controller"}},
    {{0x1bad, 0xf018}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV SE Fighting Stick"}},
    {{0x1bad, 0xf019}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Brawlstick for Xbox 360"}},
    {{0x1bad, 0xf021}, {gamepad_type_t::type_e::Xbox360, "Mad Cats Ghost Recon FS GamePad"}},
    {{0x1bad, 0xf023}, {gamepad_type_t::type_e::Xbox360, "MLG Pro Circuit Controller (Xbox)"}},
    {{0x1bad, 0xf025}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Call Of Duty"}},
    {{0x1bad, 0xf027}, {gamepad_type_t::type_e::Xbox360, "Mad Catz FPS Pro"}},
    {{0x1bad, 0xf028}, {gamepad_type_t::type_e::Xbox360, "Street Fighter IV FightPad"}},
    {{0x1bad, 0xf02e}, {gamepad_type_t::type_e::Xbox360, "Mad Catz Fightpad"}},
    {{0x1bad, 0xf036}, {gamepad_type_t::type_e::Xbox360, "Mad Catz MicroCon GamePad Pro"}},
    {{0x1bad, 0xf038}, {gamepad_type_t::type_e::Xbox360, "Street Fighter IV FightStick TE"}},
    {{0x1bad, 0xf039}, {gamepad_type_t::type_e::Xbox360, "Mad Catz MvC2 TE"}},
    {{0x1bad, 0xf03a}, {gamepad_type_t::type_e::Xbox360, "Mad Catz SFxT Fightstick Pro"}},
    {{0x1bad, 0xf03d}, {gamepad_type_t::type_e::Xbox360, "Street Fighter IV Arcade Stick TE - Chun Li"}},
    {{0x1bad, 0xf03e}, {gamepad_type_t::type_e::Xbox360, "Mad Catz MLG FightStick TE"}},
    {{0x1bad, 0xf03f}, {gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick SoulCaliber"}},
    {{0x1bad, 0xf042}, {gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick TES+"}},
    {{0x1bad, 0xf080}, {gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick TE2"}},
    {{0x1bad, 0xf501}, {gamepad_type_t::type_e::Xbox360, "HoriPad EX2 Turbo"}},
    {{0x1bad, 0xf502}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.VX SA"}},
    {{0x1bad, 0xf503}, {gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick VX"}},
    {{0x1bad, 0xf504}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro. EX"}},
    {{0x1bad, 0xf505}, {gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick EX2B"}},
    {{0x1bad, 0xf506}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.EX Premium VLX"}},
    {{0x1bad, 0xf900}, {gamepad_type_t::type_e::Xbox360, "Harmonix Xbox 360 Controller"}},
    {{0x1bad, 0xf901}, {gamepad_type_t::type_e::Xbox360, "Gamestop Xbox 360 Controller"}},
    {{0x1bad, 0xf903}, {gamepad_type_t::type_e::Xbox360, "Tron Xbox 360 controller"}},
    {{0x1bad, 0xf904}, {gamepad_type_t::type_e::Xbox360, "PDP Versus Fighting Pad"}},
    {{0x1bad, 0xf906}, {gamepad_type_t::type_e::Xbox360, "MortalKombat FightStick"}},
    {{0x1bad, 0xfa01}, {gamepad_type_t::type_e::Xbox360, "MadCatz GamePad"}},
    {{0x1bad, 0xfd00}, {gamepad_type_t::type_e::Xbox360, "Razer Onza TE"}},
    {{0x1bad, 0xfd01}, {gamepad_type_t::type_e::Xbox360, "Razer Onza"}},
    {{0x24c6, 0x5000}, {gamepad_type_t::type_e::Xbox360, "Razer Atrox Arcade Stick"}},
    {{0x24c6, 0x5300}, {gamepad_type_t::type_e::Xbox360, "PowerA MINI PROEX Controller"}},
    {{0x24c6, 0x5303}, {gamepad_type_t::type_e::Xbox360, "Xbox Airflo wired controller"}},
    {{0x24c6, 0x530a}, {gamepad_type_t::type_e::Xbox360, "Xbox 360 Pro EX Controller"}},
    {{0x24c6, 0x531a}, {gamepad_type_t::type_e::Xbox360, "PowerA Pro Ex"}},
    {{0x24c6, 0x5397}, {gamepad_type_t::type_e::Xbox360, "FUS1ON Tournament Controller"}},
    {{0x24c6, 0x541a}, {gamepad_type_t::type_e::XboxOne, "PowerA Xbox One Mini Wired Controller"}},
    {{0x24c6, 0x542a}, {gamepad_type_t::type_e::XboxOne, "Xbox ONE spectra"}},
    {{0x24c6, 0x543a}, {gamepad_type_t::type_e::XboxOne, "PowerA Xbox One wired controller"}},
    {{0x24c6, 0x5500}, {gamepad_type_t::type_e::Xbox360, "Hori XBOX 360 EX 2 with Turbo"}},
    {{0x24c6, 0x5501}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro VX-SA"}},
    {{0x24c6, 0x5502}, {gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick VX Alt"}},
    {{0x24c6, 0x5503}, {gamepad_type_t::type_e::Xbox360, "Hori Fighting Edge"}},
    {{0x24c6, 0x5506}, {gamepad_type_t::type_e::Xbox360, "Hori SOULCALIBUR V Stick"}},
    {{0x24c6, 0x550d}, {gamepad_type_t::type_e::Xbox360, "Hori GEM Xbox controller"}},
    {{0x24c6, 0x550e}, {gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro V Kai 360"}},
    {{0x24c6, 0x551a}, {gamepad_type_t::type_e::XboxOne, "PowerA FUSION Pro Controller"}},
    {{0x24c6, 0x561a}, {gamepad_type_t::type_e::XboxOne, "PowerA FUSION Controller"}},
    {{0x24c6, 0x5b02}, {gamepad_type_t::type_e::Xbox360, "Thrustmaster"}},
    {{0x24c6, 0x5b03}, {gamepad_type_t::type_e::Xbox360, "Thrustmaster Ferrari 458 Racing Wheel"}},
    {{0x24c6, 0x5d04}, {gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"}},
    {{0x24c6, 0xfafe}, {gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad for Xbox 360"}}
};

#if defined(WIN64) || defined(_WIN64) || defined(__MINGW64__) \
 || defined(WIN32) || defined(_WIN32) || defined(__MINGW32__)

// Will need SetupAPI.lib
//#pragma comment(lib, "SetupAPI.lib")

#define INITGUID
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

#include <SetupAPI.h>
#include <winioctl.h>

#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_GUIDE            0x0400
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

DEFINE_GUID(XUSB_INTERFACE_CLASS_GUID, 0xEC87F1E3, 0xC13B, 0x4100, 0xB5, 0xF7, 0x8B, 0x84, 0xD5, 0x42, 0x60, 0xCB);

#define IOCTL_XINPUT_BASE  0x8000

#define XINPUT_READ_ACCESS  ( FILE_READ_ACCESS )
#define XINPUT_WRITE_ACCESS ( FILE_WRITE_ACCESS )
#define XINPUT_RW_ACCESS    ( (XINPUT_READ_ACCESS) | (XINPUT_WRITE_ACCESS) )

#define IOCTL_XINPUT_GET_INFORMATION         CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x00, XINPUT_READ_ACCESS)  // 0x80006000
#define IOCTL_XINPUT_GET_CAPABILITIES        CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x04, XINPUT_RW_ACCESS)    // 0x8000E004
#define IOCTL_XINPUT_GET_LED_STATE           CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x08, XINPUT_RW_ACCESS)    // 0x8000E008
#define IOCTL_XINPUT_GET_GAMEPAD_STATE       CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x0C, XINPUT_RW_ACCESS)    // 0x8000E00C
#define IOCTL_XINPUT_SET_GAMEPAD_STATE       CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x10, XINPUT_WRITE_ACCESS) // 0x8000A010
#define IOCTL_XINPUT_WAIT_FOR_GUIDE_BUTTON   CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x14, XINPUT_WRITE_ACCESS) // 0x8000A014
#define IOCTL_XINPUT_GET_BATTERY_INFORMATION CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x18, XINPUT_RW_ACCESS)    // 0x8000E018
#define IOCTL_XINPUT_POWER_DOWN_DEVICE       CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x1C, XINPUT_WRITE_ACCESS) // 0x8000A010
#define IOCTL_XINPUT_GET_AUDIO_INFORMATION   CTL_CODE(IOCTL_XINPUT_BASE, 0x800, 0x20, XINPUT_RW_ACCESS)    // 0x8000E020

#define XINPUT_LED_OFF            ((BYTE)0)
#define XINPUT_LED_BLINK          ((BYTE)1)
#define XINPUT_LED_1_SWITCH_BLINK ((BYTE)2)
#define XINPUT_LED_2_SWITCH_BLINK ((BYTE)3)
#define XINPUT_LED_3_SWITCH_BLINK ((BYTE)4)
#define XINPUT_LED_4_SWITCH_BLINK ((BYTE)5)
#define XINPUT_LED_1              ((BYTE)6)
#define XINPUT_LED_2              ((BYTE)7)
#define XINPUT_LED_3              ((BYTE)8)
#define XINPUT_LED_4              ((BYTE)9)
#define XINPUT_LED_CYCLE          ((BYTE)10)
#define XINPUT_LED_FAST_BLINK     ((BYTE)11)
#define XINPUT_LED_SLOW_BLINK     ((BYTE)12)
#define XINPUT_LED_FLIPFLOP       ((BYTE)13)
#define XINPUT_LED_ALLBLINK       ((BYTE)14)
static BYTE XINPUT_PORT_TO_LED_MAP[] =
{
    XINPUT_LED_1,
    XINPUT_LED_2,
    XINPUT_LED_3,
    XINPUT_LED_4,
};
#define MAX_XINPUT_PORT_TO_LED_MAP (sizeof(XINPUT_PORT_TO_LED_MAP)/sizeof(*XINPUT_PORT_TO_LED_MAP))

static BYTE XINPUT_LED_TO_PORT_MAP[] =
{
    0xFF, 0xFF, 0, 1, 2, 3, 0, 1, 2, 3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0
};

#pragma pack(push, 1)
struct buff_in_t
{
    int16_t field_0;
    uint8_t field_2;
};

struct buff_out_xbox_t
{
    uint8_t field_0;
    uint8_t field_1;
    uint8_t status;
    uint8_t field_3;
    uint8_t field_4;
    uint32_t dwPacketNumber;
    uint8_t field_9;
    uint8_t field_A;
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
    uint32_t field_17;
    uint16_t field_1B;
};

struct buff_out_xbox_one_t
{
    uint8_t status;
    uint8_t field_1;
    uint8_t field_2;
    uint32_t dwPacketNumber;
    uint8_t field_7;
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
    uint16_t field_14;
};

#pragma pack(pop)

class XInput_Device : public Gamepad
{
    bool       _dead;
    HANDLE     _hDevice;
    uint16_t    _type;

    std::wstring _device_path;

    void reopen_device_if_dead()
    {
        if (_dead && !_device_path.empty())// Just reopen with the same parameters
        {
            if (!open_device(_device_path))
                _dead = true;
        }
    }

    HRESULT DeviceIo(DWORD ioControlCode, LPVOID inBuff, DWORD inBuffSize, LPVOID outBuff, DWORD outBuffSize, LPOVERLAPPED pOverlapped)
    {
        HRESULT result;
        int BytesReturned;
        DWORD lpBytesReturned;

        lpBytesReturned = 0;
        BytesReturned = DeviceIoControl(_hDevice, ioControlCode, inBuff, inBuffSize, outBuff, outBuffSize, &lpBytesReturned, pOverlapped);
        if (BytesReturned)
            return ERROR_SUCCESS;

        if (GetLastError() == ERROR_IO_PENDING && pOverlapped)
            result = E_PENDING;
        else
            result = E_FAIL;
        return result;
    }

    HRESULT DeviceInIo(DWORD nioctl, void* buff, size_t buff_size)
    {
        return DeviceIo(nioctl, nullptr, 0, buff, buff_size, nullptr);
    }

    HRESULT DeviceOutIo(DWORD nioctl, void* buff, size_t buff_size)
    {
        return DeviceIo(nioctl, buff, buff_size, nullptr, 0, nullptr);
    }

    HRESULT DeviceInOutIo(DWORD nioctl, void* in_buff, size_t in_buff_size, void* out_buff, size_t out_buff_size)
    {
        return DeviceIo(nioctl, in_buff, in_buff_size, out_buff, out_buff_size, nullptr);
    }

    template<typename T>
    bool parse_buffer(T &buff)
    {
        left_stick.x = buff.sThumbLX / (buff.sThumbLX > 0 ? 32767.0f : 32768.0f);
        left_stick.y = buff.sThumbLY / (buff.sThumbLY > 0 ? 32767.0f : 32768.0f);

        right_stick.x = buff.sThumbRX / (buff.sThumbRX > 0 ? 32767.0f : 32768.0f);
        right_stick.y = buff.sThumbRY / (buff.sThumbRY > 0 ? 32767.0f : 32768.0f);

        up    = buff.wButtons & XINPUT_GAMEPAD_DPAD_UP;
        down  = buff.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
        left  = buff.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
        right = buff.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

        start = buff.wButtons & XINPUT_GAMEPAD_START;
        back  = buff.wButtons & XINPUT_GAMEPAD_BACK;

        left_thumb  = buff.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
        right_thumb = buff.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;

        left_shoulder  = buff.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
        right_shoulder = buff.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

        guide = buff.wButtons & XINPUT_GAMEPAD_GUIDE;

        a = buff.wButtons & XINPUT_GAMEPAD_A;
        b = buff.wButtons & XINPUT_GAMEPAD_B;
        x = buff.wButtons & XINPUT_GAMEPAD_X;
        y = buff.wButtons & XINPUT_GAMEPAD_Y;

        left_trigger  = buff.bLeftTrigger / 255.0f;
        right_trigger = buff.bRightTrigger / 255.0f;

        return buff.status == 1;
    }

    bool get_gamepad_infos()
    {
        WORD buffer[6] = { 0 };
        bool success = (DeviceInIo(IOCTL_XINPUT_GET_INFORMATION, buffer, sizeof(buffer)) >= 0 ? true : false);
        if (success)
        {
            if (!(buffer[2] & 0x80))
            {
                for (int i = 0; i < (buffer[1] & 0xFF); ++i)
                {
                    //dwUserIndex = i;
                    _type = buffer[0];
                    id.vendorID = buffer[4];
                    id.productID = buffer[5];

                    {// This part might not be needed
                        char buff[5] = {
                            0,              // XInput User Index
                            XINPUT_LED_OFF, // LED Status
                            0,
                            0,
                            1 };

                        DeviceOutIo(IOCTL_XINPUT_SET_GAMEPAD_STATE, buff, sizeof(buff));
                    }
                }
            }
        }
        return success;
    }

    bool get_gamepad_data()
    {
        union
        {
            buff_out_xbox_t xboxOutBuff;
            buff_out_xbox_one_t xboxOneOutBuff;
        } out_buff;

        union
        {
            buff_in_t xboxInBuff;
            uint8_t xbonOneInBuff;
        } in_buff;


        DWORD inBuffSize;
        DWORD outBuffSize;
        unsigned int res;

        memset(&out_buff, 0, sizeof(out_buff));
        memset(&in_buff, 0, sizeof(in_buff));

        if (_type == 256)
        {// I don't know if its xboxOne stuff
            in_buff.xbonOneInBuff = 0;
            inBuffSize = sizeof(in_buff.xbonOneInBuff);
            outBuffSize = sizeof(out_buff.xboxOneOutBuff);
        }
        else
        {
            in_buff.xboxInBuff.field_0 = 257;
            in_buff.xboxInBuff.field_2 = 0;
            inBuffSize = sizeof(in_buff.xboxInBuff);
            outBuffSize = sizeof(out_buff.xboxOutBuff);
        }

        res = DeviceInOutIo(IOCTL_XINPUT_GET_GAMEPAD_STATE, &in_buff, inBuffSize, &out_buff, outBuffSize);
        if (res < 0)
            return false;

        bool online;
        if (_type == 256)
            online = parse_buffer(out_buff.xboxOneOutBuff);
        else
            online = parse_buffer(out_buff.xboxOutBuff);

        return online;
    }

    bool send_gamepad_vibration(uint16_t left_speed, uint16_t right_speed)
    {
        uint8_t buff[5] = {
            0,
            0,
            left_speed / 256,
            right_speed / 256,
            2
        };

        return DeviceOutIo(IOCTL_XINPUT_SET_GAMEPAD_STATE, buff, sizeof(buff)) == ERROR_SUCCESS;
    }

public:
    XInput_Device() :
        _dead(false),
        _hDevice(INVALID_HANDLE_VALUE),
        _type(0)
    {}

    virtual ~XInput_Device()
    {
        close_device();
    }

    std::wstring const& device_path() const
    {
        return _device_path;
    }

    void close_device()
    {
        if (_hDevice != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_hDevice);

            _hDevice = INVALID_HANDLE_VALUE;
            _dead = false;
        }
    }

    bool open_device(std::wstring device_path)
    {
        close_device();

        _device_path = std::move(device_path);

        _hDevice = CreateFileW(_device_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (_hDevice == INVALID_HANDLE_VALUE)
            return false;

        get_gamepad_infos();

        return true;
    }

    virtual int GetXinputId()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return -1;
    }

    virtual bool RunFrame()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        if (_hDevice == INVALID_HANDLE_VALUE)
            return false;

        if (get_gamepad_data())
            return true;

        _dead = true;
        return false;
    }

    virtual bool SetVibration(uint16_t left_speed, uint16_t right_speed)
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        if (_hDevice == INVALID_HANDLE_VALUE)
            return false;

        if (send_gamepad_vibration(left_speed, right_speed))
            return true;

        _dead = true;
        return false;
    }

    virtual bool SetLed(uint8_t r, uint8_t g, uint8_t b)
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return false;
    }

    virtual bool Enabled()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return !_dead && _hDevice != INVALID_HANDLE_VALUE;
    }
};

void find_xinput_gamepads()
{
    HDEVINFO device_info_set;
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W* data;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    DWORD idx;

    device_info_set = SetupDiGetClassDevsW(&XUSB_INTERFACE_CLASS_GUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)HeapAlloc(GetProcessHeap(), 0, sizeof(*data) + detail_size);
    data->cbSize = sizeof(*data);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    idx = 0;
    while (SetupDiEnumDeviceInterfaces(device_info_set, NULL, &XUSB_INTERFACE_CLASS_GUID, idx++, &interface_data))
    {
        if (!SetupDiGetDeviceInterfaceDetailW(device_info_set,
            &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
            continue;

        bool found = false;
        int free_device = -1;
        for (int i = 0; i < gamepads.max_size(); ++i)
        {
            if (gamepads[i]->Enabled())
            {
                if (dynamic_cast<XInput_Device*>(gamepads[i])->device_path() == data->DevicePath)
                    found = true;
            }
            else
            {
                dynamic_cast<XInput_Device*>(gamepads[i])->close_device();
                if (free_device == -1)
                    free_device = i;
            }
        }

        if (!found && free_device != -1)
            dynamic_cast<XInput_Device*>(gamepads[free_device])->open_device(data->DevicePath);
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(device_info_set);
}

std::array<Gamepad* const, Gamepad::max_connected_gamepads>& Gamepad::get_gamepads(bool redetect)
{
    std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        for (auto& gp : gamepads)
        {
            gp = new XInput_Device;
        }
    }

    if (redetect)
        find_xinput_gamepads();

    return reinterpret_cast<std::array<Gamepad* const, Gamepad::max_connected_gamepads>&>(gamepads);
}

#elif defined(__linux__) || defined(linux)// if(windows)

#include <fstream>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <string.h>

/* Number of bits for 1 unsigned char */
#define nBitsPerUchar          (sizeof(unsigned char) * 8)

/* Number of unsigned chars to contain a given number of bits */
#define nUcharsForNBits(nBits) ((((nBits)-1)/nBitsPerUchar)+1)

/* Index=Offset of given bit in 1 unsigned char */
#define bitOffsetInUchar(bit)  ((bit)%nBitsPerUchar)

/* Index=Offset of the unsigned char associated to the bit
   at the given index=offset */
#define ucharIndexForBit(bit)  ((bit)/nBitsPerUchar)

/* Value of an unsigned char with bit set at given index=offset */
#define ucharValueForBit(bit)  (((unsigned char)(1))<<bitOffsetInUchar(bit))

#define testBit(bit, array)    ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)

#define NUM_EFFECTS (FF_EFFECT_MAX-FF_EFFECT_MIN+1)
#define EFFECT_INDEX(EFFECT_ID) (EFFECT_ID-FF_EFFECT_MIN)

class Linux_Gamepad : public Gamepad
{
    int _event_fd;
    int _led_fd;
    std::string _device_path;
    bool _dead;

    struct ff_effect _effects[NUM_EFFECTS];

    void get_gamepad_infos()
    {
        struct input_id inpid;
        ioctl(_event_fd, EVIOCGID, &inpid);

        id.productID = inpid.product;
        id.vendorID = inpid.vendor;

        //std::cout << std::hex << id.vendorID << ' ' << id.productID << std::dec << std::endl;
    }

    void reopen_device_if_dead()
    {
        if (_dead && !_device_path.empty())// Just reopen with the same parameters
        {
            if (!open_device(_device_path))
                _dead = true;
        }
    }


public:
    static bool is_gamepad(std::string const& path)
    {
        unsigned char evbit[1 + EV_CNT / 8 / sizeof(unsigned char)] = { 0 };
        unsigned char keybit[1 + KEY_CNT / 8 / sizeof(unsigned char)] = { 0 };
        unsigned char absbit[1 + ABS_CNT / 8 / sizeof(unsigned char)] = { 0 };

        int fd = open(path.c_str(), O_RDONLY);
        bool res = false;
        if (fd == -1)
            return res;

        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) > 0 &&
            ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) > 0 &&
            ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) > 0)
        {
            if (testBit(EV_KEY, evbit) &&
                testBit(EV_ABS, evbit) &&
                testBit(ABS_X, absbit) &&
                testBit(ABS_Y, absbit) &&
                testBit(ABS_RX, absbit) &&
                testBit(ABS_RY, absbit) &&
                testBit(ABS_Z, absbit) &&
                testBit(ABS_RZ, absbit) &&
                testBit(ABS_HAT0X, absbit))
            {
                res = true;
            }
        }

        close(fd);

        return res;
    }

    Linux_Gamepad() :
        _event_fd(-1),
        _led_fd(-1),
        _device_path(),
        _dead(false)
    {
        for(int i = 0; i < NUM_EFFECTS; ++i)
            _effects[i].id = -1;
    }

    virtual ~Linux_Gamepad()
    {
        close_device();
    }

    std::string const& device_path() const
    {
        return _device_path;
    }

    void close_device()
    {
        _dead = false;

        for (int i = 0; i < NUM_EFFECTS; ++i)
            unregister_effect(_effects[i]);

        if (_event_fd != -1)
        {
            close(_event_fd);
            _event_fd = -1;
        }
        if (_led_fd != -1)
        {
            close(_led_fd);
            _led_fd = -1;
        }
    }

    bool open_device(std::string device_path)
    {
        close_device();

        _device_path = std::move(device_path);

        if (!is_gamepad(_device_path))
            return false;

        _event_fd = open(_device_path.c_str(), O_RDWR | O_NONBLOCK);
        for (int i = 0; i < 500 && _event_fd == -1 && errno == EACCES; ++i)
        {// When you plugin a new device, it takes some time to set the acls for write access
         // So sleep for 1ms and retry 500 times for a total of 500ms retry, it success on ~40ms on my computer
            usleep(1000);
            _event_fd = open(_device_path.c_str(), O_RDWR | O_NONBLOCK);
        }

        if (_event_fd == -1)
            _event_fd = open(_device_path.c_str(), O_RDONLY);

        if (_event_fd == -1)
            return false;

        // TODO: led
        //dir_path = "/sys/class/leds/xpad";
        //dir_path += _device_path.substr(_device_path.rfind("/js") + 3);
        //dir_path += "/brightness";

        //_led_fd = open(dir_path.c_str(), O_RDWR);

        //std::cout << "_event_fd: " << _event_fd << ", _led_fd: " << _led_fd << std::endl;

        get_gamepad_infos();

        return true;
    }

    void get_available_effects()
    {
        if (_event_fd != -1)
        {
            unsigned char ffFeatures[1 + FF_MAX / 8 / sizeof(unsigned char)] = { 0 };
            if (ioctl(_event_fd, EVIOCGBIT(EV_FF, sizeof(ffFeatures) * sizeof(unsigned char)), ffFeatures) != -1)
            {
            }
        }
    }

    void register_effect(struct ff_effect &ff)
    {
        if (_event_fd == -1 || ioctl(_event_fd, EVIOCSFF, &ff) == -1)
        {
            //std::cout << "Failed to register effect " << ff.type << std::endl;
            ff.id = -1;
        }
    }

    void unregister_effect(struct ff_effect &ff)
    {
        if(ff.id == -1 || _event_fd == -1)
            return;

        stop_effect(ff);

        // delete the effect
        if (ioctl(_event_fd, EVIOCRMFF, ff.id) == -1)
        {
            //std::cout << "Failed to unregister effect " << ff.type << std::endl;
        }
        ff.id = -1;
    }

    bool play_effect(struct ff_effect &ff)
    {
        if(ff.id == -1 || _event_fd == -1)
            return false;

        struct input_event play;

        play.type = EV_FF;
        play.code = ff.id;
        play.value = 1;

        if (write(_event_fd, (const void*)&play, sizeof(play)) == -1)
        {
            //std::cout << "Failed to play effect " << effect_id << std::endl;
            return false;
        }

        return true;
    }

    bool stop_effect(struct ff_effect &ff)
    {
        if(ff.id == -1 || _event_fd == -1)
            return false;

        struct input_event play;

        play.type = EV_FF;
        play.code = ff.id;
        play.value = 0;

        if (write(_event_fd, (const void*)&play, sizeof(play)) == -1)
        {
            //std::cout << "Failed to stop effect " << effect_id << std::endl;
            return false;
        }

        return true;
    }

    virtual int GetXinputId()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return -1;
    }

    virtual bool RunFrame()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        struct input_event events[32];
        int num_events;
        bool r = false;
        while ((num_events = read(_event_fd, events, (sizeof events))) > 0)
        {
            r = true;
            num_events /= sizeof(*events);
            for (int i = 0; i < num_events; ++i)
            {
                switch (events[i].type)
                {
                    case EV_KEY:
                        switch (events[i].code)
                        {
                            case BTN_A: a = events[i].value; break;
                            case BTN_B: b = events[i].value; break;
                            case BTN_X: x = events[i].value; break;
                            case BTN_Y: y = events[i].value; break;
                            case BTN_TL: left_shoulder = events[i].value; break;
                            case BTN_TR: right_shoulder = events[i].value; break;
                            case BTN_SELECT: back = events[i].value; break;
                            case BTN_START: start = events[i].value; break;
                            case BTN_THUMBL: left_thumb = events[i].value; break;
                            case BTN_THUMBR: right_thumb = events[i].value; break;
                            case BTN_MODE: guide = events[i].value; break;
                        }
                    break;

                    case EV_ABS:
                        switch (events[i].code)
                        {
                            case ABS_X: left_stick.x = events[i].value / (events[i].value > 0 ? 32767.0f : 32768.0f); break;
                            case ABS_Y: left_stick.y = events[i].value / (events[i].value > 0 ? -32767.0f : -32768.0f); break;
                            case ABS_RX: right_stick.x = events[i].value / (events[i].value > 0 ? 32767.0f : 32768.0f); break;
                            case ABS_RY: right_stick.y = events[i].value / (events[i].value > 0 ? -32767.0f : -32768.0f); break;
                            case ABS_Z: left_trigger = events[i].value / 255.0f; break;
                            case ABS_RZ: right_trigger = events[i].value / 255.0f; break;
                            case ABS_HAT0X:
                                if (events[i].value == 0)
                                    left = right = false;
                                else
                                    left = !(right = events[i].value == 1);
                            break;
                            case ABS_HAT0Y:
                                if (events[i].value == 0)
                                    up = down = false;
                                else
                                    up = !(down = events[i].value == 1);
                            break;
                        }
                    break;
                }
            }
        }

        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            //std::cout << "dead" << std::endl;
            _dead = true;
        }

        return r;
    }

    virtual bool SetVibration(uint16_t left_speed, uint16_t right_speed)
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        if (_event_fd == -1)
          return false;
        
        struct ff_effect& rumble = _effects[EFFECT_INDEX(FF_RUMBLE)];

        if (rumble.u.rumble.strong_magnitude == left_speed &&
            rumble.u.rumble.weak_magnitude == right_speed)
            return true;

        if (rumble.id != -1)
        {
            stop_effect(rumble);
            unregister_effect(rumble);
            rumble.u.rumble.strong_magnitude = 0;
            rumble.u.rumble.weak_magnitude = 0;
        }

        if(left_speed == 0 && right_speed == 0)
            return true;

        rumble.type = FF_RUMBLE;
        rumble.id = -1;
        rumble.u.rumble.strong_magnitude = left_speed;
        rumble.u.rumble.weak_magnitude = right_speed;
        rumble.replay.length = 0xFFFF;
        rumble.replay.delay = 0;

        register_effect(rumble);
        return play_effect(rumble);
    }

    virtual bool SetLed(uint8_t r, uint8_t g, uint8_t b)
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return false;
    }

    virtual bool Enabled()
    {
        std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
        reopen_device_if_dead();

        return !_dead && _event_fd != -1;
    }
};

void find_linux_gamepads()
{
    DIR* input_dir;
    struct dirent *input_dir_entry;

    input_dir = opendir("/dev/input");
    if (input_dir == nullptr)
        return;

    while ((input_dir_entry = readdir(input_dir)) != nullptr)
    {
        if (strncmp(input_dir_entry->d_name, "event", 2) != 0)
            continue;
    
        std::string js_path("/dev/input/");
        js_path += input_dir_entry->d_name;
        if (!Linux_Gamepad::is_gamepad(js_path))
            continue;

        bool found = false;
        int free_device = -1;
        for (int i = 0; i < gamepads.max_size(); ++i)
        {
            if (gamepads[i]->Enabled())
            {
                if(dynamic_cast<Linux_Gamepad*>(gamepads[i])->device_path() == js_path)
                    found = true;
            }
            else
            {
                dynamic_cast<Linux_Gamepad*>(gamepads[i])->close_device();
                if(free_device == -1)
                    free_device = i;
            }
        }

        if (!found && free_device != -1)
            dynamic_cast<Linux_Gamepad*>(gamepads[free_device])->open_device(js_path);
    }

    closedir(input_dir);
}

std::array<Gamepad *const, Gamepad::max_connected_gamepads>& Gamepad::get_gamepads(bool redetect)
{
    std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        for (auto& gp : gamepads)
        {   
            gp = new Linux_Gamepad;
        }
    }

    if (redetect)
        find_linux_gamepads();

    return reinterpret_cast<std::array<Gamepad* const, Gamepad::max_connected_gamepads>&>(gamepads);
}

#elif defined(__APPLE__)//(linux)

class MacOSX_Device : public Gamepad
{
public:
    MacOSX_Device()
    {}

    virtual ~MacOSX_Device()
    {
    }

    virtual int GetXinputId()
    {
        return -1;
    }

    virtual bool RunFrame()
    {
        return false;
    }

    virtual bool SetVibration(uint16_t left_speed, uint16_t right_speed)
    {
        return false;
    }

    virtual bool SetLed(uint8_t r, uint8_t g, uint8_t b)
    {
        return false;
    }

    virtual bool Enabled()
    {
        return false;
    }
};

void find_macosx_gamepads()
{
    // TODO: Detect MacOS gamepads
}

std::array<Gamepad *const, Gamepad::max_connected_gamepads>& Gamepad::get_gamepads(bool redetect)
{
    std::lock_guard<std::recursive_mutex> lock(gamepad_mutex);
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        for (auto& gp : gamepads)
        {
            gp = new MacOSX_Device;
        }
    }

    if (redetect)
        find_macosx_gamepads();

    return reinterpret_cast<std::array<Gamepad* const, Gamepad::max_connected_gamepads>&>(gamepads);
}

#endif
