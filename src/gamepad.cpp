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

#include <gamepad/gamepad.h>
#include "gamepad_internal.h"
#include <mutex>

namespace gamepad 
{
struct gamepad_context_t;

static int32_t internal_get_gamepad(uint32_t index, gamepad_context_t** pp_context);
static int32_t internal_update_gamepad_state(gamepad_context_t* p_context);
static int32_t internal_get_gamepad_state(gamepad_context_t* p_context, gamepad_state_t* p_gamepad_state);
static int32_t internal_get_gamepad_id(gamepad_context_t* p_context, gamepad_id_t* p_gamepad_id);
static int32_t internal_set_gamepad_vibration(gamepad_context_t* p_context, float left_strength, float right_strength);
static int32_t internal_set_gamepad_led(gamepad_context_t* p_context, uint8_t r, uint8_t g, uint8_t b);
static void    internal_free_all_contexts();

static std::mutex s_gamepad_mutex;
static gamepad_context_t* s_gamepads[max_connected_gamepads] = {
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
};

template<typename ...Args>
static inline int32_t call_internal_action(uint32_t index, int32_t(*pfn_internal)(gamepad_context_t*, Args ...), Args ...args)
{
    std::lock_guard<std::mutex> lk(s_gamepad_mutex);

    int32_t res;

    gamepad_context_t* p_context;
    if ((res = internal_get_gamepad(index, &p_context)) != gamepad::success)
        return res;

    return pfn_internal(p_context, std::forward<Args>(args)...);
}

const gamepad_type_t& get_gamepad_type(gamepad_id_t const& id)
{
    static const struct internal_gamepad_map_t
    {
        gamepad_id_t id;
        gamepad_type_t type_infos;
    } known_gamepads[] = {
        {{0x0079, 0x18d4}, gamepad_type_t::type_e::Xbox360, "GPD Win 2 X-Box Controller"},
        {{0x044f, 0xb326}, gamepad_type_t::type_e::Xbox360, "Thrustmaster Gamepad GP XID"},
        {{0x045e, 0x028e}, gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad"},
        {{0x045e, 0x028f}, gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad v2"},
        {{0x045e, 0x0291}, gamepad_type_t::type_e::Xbox360, "Xbox 360 Wireless Receiver (XBOX)"},
        {{0x045e, 0x02a0}, gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 Big Button IR"},
        {{0x045e, 0x02a1}, gamepad_type_t::type_e::Xbox360, "Microsoft X-Box 360 pad"},
        {{0x045e, 0x02dd}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One pad"},
        {{0x044f, 0xb326}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One pad (Firmware 2015)"},
        {{0x045e, 0x02e0}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad (Bluetooth)"},
        {{0x045e, 0x02e3}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One Elite pad"},
        {{0x045e, 0x02ea}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad"},
        {{0x045e, 0x02fd}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One S pad (Bluetooth)"},
        {{0x045e, 0x02ff}, gamepad_type_t::type_e::XboxOne, "Microsoft X-Box One Elite pad"},
        {{0x045e, 0x0b13}, gamepad_type_t::type_e::XboxOne, "Xbox Wireless Controller"},
        {{0x045e, 0x0719}, gamepad_type_t::type_e::Xbox360, "Xbox 360 Wireless Receiver"},
        {{0x046d, 0xc21d}, gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F310"},
        {{0x046d, 0xc21e}, gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F510"},
        {{0x046d, 0xc21f}, gamepad_type_t::type_e::Xbox360, "Logitech Gamepad F710"},
        {{0x046d, 0xc242}, gamepad_type_t::type_e::Xbox360, "Logitech Chillstream Controller"},
        {{0x0f0d, 0x00c1}, gamepad_type_t::type_e::Switch , "HORI Pad Switch"},
        {{0x0f0d, 0x0092}, gamepad_type_t::type_e::Switch , "HORI Pokken Tournament DX Pro Pad"},
        {{0x0f0d, 0x00f6}, gamepad_type_t::type_e::Switch , "HORI Wireless Switch Pad"},
        {{0x0f0d, 0x00dc}, gamepad_type_t::type_e::Switch , "HORI Battle Pad"},
        {{0x20d6, 0xa711}, gamepad_type_t::type_e::Switch , "PowerA Wired Controller Plus/PowerA Wired Gamcube Controller"},
        {{0x0e6f, 0x0185}, gamepad_type_t::type_e::Switch , "PDP Wired Fight Pad Pro for Nintendo Switch"},
        {{0x0e6f, 0x0180}, gamepad_type_t::type_e::Switch , "PDP Faceoff Wired Pro Controller for Nintendo Switch"},
        {{0x0e6f, 0x0181}, gamepad_type_t::type_e::Switch , "PDP Faceoff Deluxe Wired Pro Controller for Nintendo Switch"},
        {{0x054c, 0x0268}, gamepad_type_t::type_e::PS3    , "Sony PS3 Controller"},
        {{0x0925, 0x0005}, gamepad_type_t::type_e::PS3    , "Sony PS3 Controller"},
        {{0x8888, 0x0308}, gamepad_type_t::type_e::PS3    , "Sony PS3 Controller"},
        {{0x1a34, 0x0836}, gamepad_type_t::type_e::PS3    , "Afterglow PS3"},
        {{0x0f0d, 0x006e}, gamepad_type_t::type_e::PS3    , "HORI horipad4 PS3"},
        {{0x0f0d, 0x0066}, gamepad_type_t::type_e::PS3    , "HORI horipad4 PS4"},
        {{0x0f0d, 0x005f}, gamepad_type_t::type_e::PS3    , "HORI Fighting commander PS3"},
        {{0x0f0d, 0x005e}, gamepad_type_t::type_e::PS3    , "HORI Fighting commander PS4"},
        {{0x0738, 0x8250}, gamepad_type_t::type_e::PS3    , "Madcats Fightpad Pro PS4"},
        {{0x0079, 0x181a}, gamepad_type_t::type_e::PS3    , "Venom Arcade Stick"},
        {{0x0079, 0x0006}, gamepad_type_t::type_e::PS3    , "PC Twin Shock Controller"},
        {{0x2563, 0x0523}, gamepad_type_t::type_e::PS3    , "Digiflip GP006"},
        {{0x11ff, 0x3331}, gamepad_type_t::type_e::PS3    , "SRXJ-PH2400"},
        {{0x20bc, 0x5500}, gamepad_type_t::type_e::PS3    , "ShanWan PS3"},
        {{0x044f, 0xb315}, gamepad_type_t::type_e::PS3    , "Firestorm Dual Analog 3"},
        {{0x0f0d, 0x004d}, gamepad_type_t::type_e::PS3    , "Horipad 3"},
        {{0x0f0d, 0x0009}, gamepad_type_t::type_e::PS3    , "HORI BDA GP1"},
        {{0x0e8f, 0x0008}, gamepad_type_t::type_e::PS3    , "Green Asia"},
        {{0x0f0d, 0x006a}, gamepad_type_t::type_e::PS3    , "Real Arcade Pro 4"},
        {{0x0e6f, 0x011e}, gamepad_type_t::type_e::PS3    , "Rock Candy PS4"},
        {{0x0e6f, 0x0214}, gamepad_type_t::type_e::PS3    , "Afterglow PS3"},
        {{0x056e, 0x2013}, gamepad_type_t::type_e::PS3    , "JC-U4113SBK"},
        {{0x0738, 0x8838}, gamepad_type_t::type_e::PS3    , "Madcatz Fightstick Pro"},
        {{0x1a34, 0x0836}, gamepad_type_t::type_e::PS3    , "Afterglow PS3"},
        {{0x0f30, 0x1100}, gamepad_type_t::type_e::PS3    , "Quanba Q1 fight stick"},
        {{0x0f0d, 0x0087}, gamepad_type_t::type_e::PS3    , "HORI fighting mini stick"},
        {{0x8380, 0x0003}, gamepad_type_t::type_e::PS3    , "BTP 2163"},
        {{0x1345, 0x1000}, gamepad_type_t::type_e::PS3    , "PS2 ACME GA-D5"},
        {{0x0e8f, 0x3075}, gamepad_type_t::type_e::PS3    , "SpeedLink Strike FX"},
        {{0x0e6f, 0x0128}, gamepad_type_t::type_e::PS3    , "Rock Candy PS3"},
        {{0x2c22, 0x2000}, gamepad_type_t::type_e::PS3    , "Quanba Drone"},
        {{0x06a3, 0xf622}, gamepad_type_t::type_e::PS3    , "Cyborg V3"},
        {{0x044f, 0xd007}, gamepad_type_t::type_e::PS3    , "Thrustmaster wireless 3-1"},
        {{0x25f0, 0x83c3}, gamepad_type_t::type_e::PS3    , "Gioteck vx2"},
        {{0x05b8, 0x1006}, gamepad_type_t::type_e::PS3    , "JC-U3412SBK"},
        {{0x20d6, 0x576d}, gamepad_type_t::type_e::PS3    , "Power A PS3"},
        {{0x0e6f, 0x1314}, gamepad_type_t::type_e::PS3    , "PDP Afterglow Wireless PS3 controller"},
        {{0x0738, 0x3180}, gamepad_type_t::type_e::PS3    , "Mad Catz Alpha PS3 mode"},
        {{0x0738, 0x8180}, gamepad_type_t::type_e::PS3    , "Mad Catz Alpha PS4 mode"},
        {{0x0e6f, 0x0203}, gamepad_type_t::type_e::PS3    , "Victrix Pro FS"},
        {{0x054c, 0x05c4}, gamepad_type_t::type_e::PS4    , "Sony PS4 Controller"},
        {{0x054c, 0x09cc}, gamepad_type_t::type_e::PS4    , "Sony PS4 Slim Controller"},
        {{0x054c, 0x0ba0}, gamepad_type_t::type_e::PS4    , "Sony PS4 Controller (Wireless dongle)"},
        {{0x0f0d, 0x008a}, gamepad_type_t::type_e::PS4    , "HORI Real Arcade Pro 4"},
        {{0x0f0d, 0x0055}, gamepad_type_t::type_e::PS4    , "HORIPAD 4 FPS"},
        {{0x0f0d, 0x0066}, gamepad_type_t::type_e::PS4    , "HORIPAD 4 FPS Plus"},
        {{0x0738, 0x8384}, gamepad_type_t::type_e::PS4    , "HORIPAD 4 FPS Plus"},
        {{0x0738, 0x8250}, gamepad_type_t::type_e::PS4    , "Mad Catz FightPad Pro PS4"},
        {{0x0738, 0x8384}, gamepad_type_t::type_e::PS4    , "Mad Catz Fightstick TE S+"},
        {{0x0C12, 0x0E10}, gamepad_type_t::type_e::PS4    , "Armor Armor 3 Pad PS4"},
        {{0x0C12, 0x1CF6}, gamepad_type_t::type_e::PS4    , "EMIO PS4 Elite Controller"},
        {{0x1532, 0x1000}, gamepad_type_t::type_e::PS4    , "Razer Raiju PS4 Controller"},
        {{0x1532, 0X0401}, gamepad_type_t::type_e::PS4    , "Razer Panthera PS4 Controller"},
        {{0x054c, 0x05c5}, gamepad_type_t::type_e::PS4    , "STRIKEPAD PS4 Grip Add-on"},
        {{0x146b, 0x0d01}, gamepad_type_t::type_e::PS4    , "Nacon Revolution Pro Controller"},
        {{0x146b, 0x0d02}, gamepad_type_t::type_e::PS4    , "Nacon Revolution Pro Controller V2"},
        {{0x0f0d, 0x00a0}, gamepad_type_t::type_e::PS4    , "HORI TAC4"},
        {{0x0f0d, 0x009c}, gamepad_type_t::type_e::PS4    , "HORI TAC PRO"},
        {{0x0c12, 0x0ef6}, gamepad_type_t::type_e::PS4    , "Hitbox Arcade Stick"},
        {{0x0079, 0x181b}, gamepad_type_t::type_e::PS4    , "Venom Arcade Stick"},
        {{0x0738, 0x3250}, gamepad_type_t::type_e::PS4    , "Mad Catz FightPad PRO"},
        {{0x0f0d, 0x00ee}, gamepad_type_t::type_e::PS4    , "HORI mini wired gamepad"},
        {{0x0738, 0x8481}, gamepad_type_t::type_e::PS4    , "Mad Catz FightStick TE 2+ PS4"},
        {{0x0738, 0x8480}, gamepad_type_t::type_e::PS4    , "Mad Catz FightStick TE 2"},
        {{0x7545, 0x0104}, gamepad_type_t::type_e::PS4    , "Armor 3, Level Up Cobra"},
        {{0x1532, 0x1007}, gamepad_type_t::type_e::PS4    , "Razer Raiju 2 Tournament Edition (USB)"},
        {{0x1532, 0x100A}, gamepad_type_t::type_e::PS4    , "Razer Raiju 2 Tournament Edition (BT)"},
        {{0x1532, 0x1004}, gamepad_type_t::type_e::PS4    , "Razer Raiju 2 Ultimate Edition (USB)"},
        {{0x1532, 0x1009}, gamepad_type_t::type_e::PS4    , "Razer Raiju 2 Ultimate Edition (BT)"},
        {{0x1532, 0x1008}, gamepad_type_t::type_e::PS4    , "Razer Panthera Evo Fightstick"},
        {{0x9886, 0x0025}, gamepad_type_t::type_e::PS4    , "Astro C40"},
        {{0x0c12, 0x0e15}, gamepad_type_t::type_e::PS4    , "Game:Pad 4"},
        {{0x4001, 0x0104}, gamepad_type_t::type_e::PS4    , "PS4 Fun Controller"},
        {{0x056e, 0x2004}, gamepad_type_t::type_e::Xbox360, "Elecom JC-U3613M"},
        {{0x06a3, 0xf51a}, gamepad_type_t::type_e::Xbox360, "Saitek P3600"},
        {{0x0738, 0x4716}, gamepad_type_t::type_e::Xbox360, "Mad Catz Wired Xbox 360 Controller"},
        {{0x0738, 0x4718}, gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV FightStick SE"},
        {{0x0738, 0x4726}, gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox 360 Controller"},
        {{0x0738, 0x4728}, gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV FightPad"},
        {{0x0738, 0x4736}, gamepad_type_t::type_e::Xbox360, "Mad Catz MicroCon Gamepad"},
        {{0x0738, 0x4738}, gamepad_type_t::type_e::Xbox360, "Mad Catz Wired Xbox 360 Controller (SFIV)"},
        {{0x0738, 0x4740}, gamepad_type_t::type_e::Xbox360, "Mad Catz Beat Pad"},
        {{0x0738, 0x4a01}, gamepad_type_t::type_e::XboxOne, "Mad Catz FightStick TE 2"},
        {{0x0738, 0xb726}, gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox controller - MW2"},
        {{0x0738, 0xbeef}, gamepad_type_t::type_e::Xbox360, "Mad Catz JOYTECH NEO SE Advanced GamePad"},
        {{0x0738, 0xcb02}, gamepad_type_t::type_e::Xbox360, "Saitek Cyborg Rumble Pad - PC/Xbox 360"},
        {{0x0738, 0xcb03}, gamepad_type_t::type_e::Xbox360, "Saitek P3200 Rumble Pad - PC/Xbox 360"},
        {{0x0738, 0xf738}, gamepad_type_t::type_e::Xbox360, "Super SFIV FightStick TE S"},
        {{0x0e6f, 0x0105}, gamepad_type_t::type_e::Xbox360, "HSM3 Xbox360 dancepad"},
        {{0x0e6f, 0x0113}, gamepad_type_t::type_e::Xbox360, "Afterglow AX.1 Gamepad for Xbox 360"},
        {{0x0e6f, 0x011f}, gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad Wired Controller"},
        {{0x0e6f, 0x0133}, gamepad_type_t::type_e::Xbox360, "Xbox 360 Wired Controller"},
        {{0x0e6f, 0x0139}, gamepad_type_t::type_e::XboxOne, "Afterglow Prismatic Wired Controller"},
        {{0x0e6f, 0x013a}, gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"},
        {{0x0e6f, 0x0146}, gamepad_type_t::type_e::XboxOne, "Rock Candy Wired Controller for Xbox One"},
        {{0x0e6f, 0x0147}, gamepad_type_t::type_e::XboxOne, "PDP Marvel Xbox One Controller"},
        {{0x0e6f, 0x015c}, gamepad_type_t::type_e::XboxOne, "PDP Xbox One Arcade Stick"},
        {{0x0e6f, 0x0161}, gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"},
        {{0x0e6f, 0x0162}, gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"},
        {{0x0e6f, 0x0163}, gamepad_type_t::type_e::XboxOne, "PDP Xbox One Controller"},
        {{0x0e6f, 0x0164}, gamepad_type_t::type_e::XboxOne, "PDP Battlefield One"},
        {{0x0e6f, 0x0165}, gamepad_type_t::type_e::XboxOne, "PDP Titanfall 2"},
        {{0x0e6f, 0x0201}, gamepad_type_t::type_e::Xbox360, "Pelican PL-3601 'TSZ' Wired Xbox 360 Controller"},
        {{0x0e6f, 0x0213}, gamepad_type_t::type_e::Xbox360, "Afterglow Gamepad for Xbox 360"},
        {{0x0e6f, 0x021f}, gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad for Xbox 360"},
        {{0x0e6f, 0x0246}, gamepad_type_t::type_e::XboxOne, "Rock Candy Gamepad for Xbox One 2015"},
        {{0x0e6f, 0x0301}, gamepad_type_t::type_e::Xbox360, "Logic3 Controller"},
        {{0x0e6f, 0x0346}, gamepad_type_t::type_e::XboxOne, "Rock Candy Gamepad for Xbox One 2016"},
        {{0x0e6f, 0x0401}, gamepad_type_t::type_e::Xbox360, "Logic3 Controller"},
        {{0x0e6f, 0x0413}, gamepad_type_t::type_e::Xbox360, "Afterglow AX.1 Gamepad for Xbox 360"},
        {{0x0e6f, 0x0501}, gamepad_type_t::type_e::Xbox360, "PDP Xbox 360 Controller"},
        {{0x0e6f, 0xf900}, gamepad_type_t::type_e::Xbox360, "PDP Afterglow AX.1"},
        {{0x0f0d, 0x000a}, gamepad_type_t::type_e::Xbox360, "Hori Co. DOA4 FightStick"},
        {{0x0f0d, 0x000c}, gamepad_type_t::type_e::Xbox360, "Hori PadEX Turbo"},
        {{0x0f0d, 0x000d}, gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick EX2"},
        {{0x0f0d, 0x0016}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.EX"},
        {{0x0f0d, 0x001b}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro VX"},
        {{0x0f0d, 0x0063}, gamepad_type_t::type_e::XboxOne, "Hori Real Arcade Pro Hayabusa (USA) Xbox One"},
        {{0x0f0d, 0x0067}, gamepad_type_t::type_e::XboxOne, "HORIPAD ONE"},
        {{0x0f0d, 0x0078}, gamepad_type_t::type_e::XboxOne, "Hori Real Arcade Pro V Kai Xbox One"},
        {{0x11c9, 0x55f0}, gamepad_type_t::type_e::Xbox360, "Nacon GC-100XF"},
        {{0x12ab, 0x0004}, gamepad_type_t::type_e::Xbox360, "Honey Bee Xbox360 dancepad"},
        {{0x12ab, 0x0301}, gamepad_type_t::type_e::Xbox360, "PDP AFTERGLOW AX.1"},
        {{0x12ab, 0x0303}, gamepad_type_t::type_e::Xbox360, "Mortal Kombat Klassic FightStick"},
        {{0x1430, 0x4748}, gamepad_type_t::type_e::Xbox360, "RedOctane Guitar Hero X-plorer"},
        {{0x1430, 0xf801}, gamepad_type_t::type_e::Xbox360, "RedOctane Controller"},
        {{0x146b, 0x0601}, gamepad_type_t::type_e::Xbox360, "BigBen Interactive XBOX 360 Controller"},
        {{0x1532, 0x0037}, gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"},
        {{0x1532, 0x0a00}, gamepad_type_t::type_e::XboxOne, "Razer Atrox Arcade Stick"},
        {{0x1532, 0x0a03}, gamepad_type_t::type_e::XboxOne, "Razer Wildcat"},
        {{0x15e4, 0x3f00}, gamepad_type_t::type_e::Xbox360, "Power A Mini Pro Elite"},
        {{0x15e4, 0x3f0a}, gamepad_type_t::type_e::Xbox360, "Xbox Airflo wired controller"},
        {{0x15e4, 0x3f10}, gamepad_type_t::type_e::Xbox360, "Batarang Xbox 360 controller"},
        {{0x162e, 0xbeef}, gamepad_type_t::type_e::Xbox360, "Joytech Neo-Se Take2"},
        {{0x1689, 0xfd00}, gamepad_type_t::type_e::Xbox360, "Razer Onza Tournament Edition"},
        {{0x1689, 0xfd01}, gamepad_type_t::type_e::Xbox360, "Razer Onza Classic Edition"},
        {{0x1689, 0xfe00}, gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"},
        {{0x1bad, 0x0002}, gamepad_type_t::type_e::Xbox360, "Harmonix Rock Band Guitar"},
        {{0x1bad, 0x0003}, gamepad_type_t::type_e::Xbox360, "Harmonix Rock Band Drumkit"},
        {{0x1bad, 0xf016}, gamepad_type_t::type_e::Xbox360, "Mad Catz Xbox 360 Controller"},
        {{0x1bad, 0xf018}, gamepad_type_t::type_e::Xbox360, "Mad Catz Street Fighter IV SE Fighting Stick"},
        {{0x1bad, 0xf019}, gamepad_type_t::type_e::Xbox360, "Mad Catz Brawlstick for Xbox 360"},
        {{0x1bad, 0xf021}, gamepad_type_t::type_e::Xbox360, "Mad Cats Ghost Recon FS GamePad"},
        {{0x1bad, 0xf023}, gamepad_type_t::type_e::Xbox360, "MLG Pro Circuit Controller (Xbox)"},
        {{0x1bad, 0xf025}, gamepad_type_t::type_e::Xbox360, "Mad Catz Call Of Duty"},
        {{0x1bad, 0xf027}, gamepad_type_t::type_e::Xbox360, "Mad Catz FPS Pro"},
        {{0x1bad, 0xf028}, gamepad_type_t::type_e::Xbox360, "Street Fighter IV FightPad"},
        {{0x1bad, 0xf02e}, gamepad_type_t::type_e::Xbox360, "Mad Catz Fightpad"},
        {{0x1bad, 0xf036}, gamepad_type_t::type_e::Xbox360, "Mad Catz MicroCon GamePad Pro"},
        {{0x1bad, 0xf038}, gamepad_type_t::type_e::Xbox360, "Street Fighter IV FightStick TE"},
        {{0x1bad, 0xf039}, gamepad_type_t::type_e::Xbox360, "Mad Catz MvC2 TE"},
        {{0x1bad, 0xf03a}, gamepad_type_t::type_e::Xbox360, "Mad Catz SFxT Fightstick Pro"},
        {{0x1bad, 0xf03d}, gamepad_type_t::type_e::Xbox360, "Street Fighter IV Arcade Stick TE - Chun Li"},
        {{0x1bad, 0xf03e}, gamepad_type_t::type_e::Xbox360, "Mad Catz MLG FightStick TE"},
        {{0x1bad, 0xf03f}, gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick SoulCaliber"},
        {{0x1bad, 0xf042}, gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick TES+"},
        {{0x1bad, 0xf080}, gamepad_type_t::type_e::Xbox360, "Mad Catz FightStick TE2"},
        {{0x1bad, 0xf501}, gamepad_type_t::type_e::Xbox360, "HoriPad EX2 Turbo"},
        {{0x1bad, 0xf502}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.VX SA"},
        {{0x1bad, 0xf503}, gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick VX"},
        {{0x1bad, 0xf504}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro. EX"},
        {{0x1bad, 0xf505}, gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick EX2B"},
        {{0x1bad, 0xf506}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro.EX Premium VLX"},
        {{0x1bad, 0xf900}, gamepad_type_t::type_e::Xbox360, "Harmonix Xbox 360 Controller"},
        {{0x1bad, 0xf901}, gamepad_type_t::type_e::Xbox360, "Gamestop Xbox 360 Controller"},
        {{0x1bad, 0xf903}, gamepad_type_t::type_e::Xbox360, "Tron Xbox 360 controller"},
        {{0x1bad, 0xf904}, gamepad_type_t::type_e::Xbox360, "PDP Versus Fighting Pad"},
        {{0x1bad, 0xf906}, gamepad_type_t::type_e::Xbox360, "MortalKombat FightStick"},
        {{0x1bad, 0xfa01}, gamepad_type_t::type_e::Xbox360, "MadCatz GamePad"},
        {{0x1bad, 0xfd00}, gamepad_type_t::type_e::Xbox360, "Razer Onza TE"},
        {{0x1bad, 0xfd01}, gamepad_type_t::type_e::Xbox360, "Razer Onza"},
        {{0x24c6, 0x5000}, gamepad_type_t::type_e::Xbox360, "Razer Atrox Arcade Stick"},
        {{0x24c6, 0x5300}, gamepad_type_t::type_e::Xbox360, "PowerA MINI PROEX Controller"},
        {{0x24c6, 0x5303}, gamepad_type_t::type_e::Xbox360, "Xbox Airflo wired controller"},
        {{0x24c6, 0x530a}, gamepad_type_t::type_e::Xbox360, "Xbox 360 Pro EX Controller"},
        {{0x24c6, 0x531a}, gamepad_type_t::type_e::Xbox360, "PowerA Pro Ex"},
        {{0x24c6, 0x5397}, gamepad_type_t::type_e::Xbox360, "FUS1ON Tournament Controller"},
        {{0x24c6, 0x541a}, gamepad_type_t::type_e::XboxOne, "PowerA Xbox One Mini Wired Controller"},
        {{0x24c6, 0x542a}, gamepad_type_t::type_e::XboxOne, "Xbox ONE spectra"},
        {{0x24c6, 0x543a}, gamepad_type_t::type_e::XboxOne, "PowerA Xbox One wired controller"},
        {{0x24c6, 0x5500}, gamepad_type_t::type_e::Xbox360, "Hori XBOX 360 EX 2 with Turbo"},
        {{0x24c6, 0x5501}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro VX-SA"},
        {{0x24c6, 0x5502}, gamepad_type_t::type_e::Xbox360, "Hori Fighting Stick VX Alt"},
        {{0x24c6, 0x5503}, gamepad_type_t::type_e::Xbox360, "Hori Fighting Edge"},
        {{0x24c6, 0x5506}, gamepad_type_t::type_e::Xbox360, "Hori SOULCALIBUR V Stick"},
        {{0x24c6, 0x550d}, gamepad_type_t::type_e::Xbox360, "Hori GEM Xbox controller"},
        {{0x24c6, 0x550e}, gamepad_type_t::type_e::Xbox360, "Hori Real Arcade Pro V Kai 360"},
        {{0x24c6, 0x551a}, gamepad_type_t::type_e::XboxOne, "PowerA FUSION Pro Controller"},
        {{0x24c6, 0x561a}, gamepad_type_t::type_e::XboxOne, "PowerA FUSION Controller"},
        {{0x24c6, 0x5b02}, gamepad_type_t::type_e::Xbox360, "Thrustmaster"},
        {{0x24c6, 0x5b03}, gamepad_type_t::type_e::Xbox360, "Thrustmaster Ferrari 458 Racing Wheel"},
        {{0x24c6, 0x5d04}, gamepad_type_t::type_e::Xbox360, "Razer Sabertooth"},
        {{0x24c6, 0xfafe}, gamepad_type_t::type_e::Xbox360, "Rock Candy Gamepad for Xbox 360"},
        {{0xffff, 0xffff}, gamepad_type_t::type_e::Unknown, "Unknown gamepad"},
    };
    
    constexpr size_t known_gamepad_count = sizeof(known_gamepads) / sizeof(*known_gamepads);

    for (size_t i = 0; i < known_gamepad_count; ++i)
    {
        if (known_gamepads[i].id.id == id.id)
            return known_gamepads[i].type_infos;
    }

    return known_gamepads[known_gamepad_count-1].type_infos;
}

int32_t update_gamepad_state(uint32_t index)
{
    if (index >= gamepad::max_connected_gamepads)
        return gamepad::invalid_parameter;

    return call_internal_action(index, &internal_update_gamepad_state);
}

int32_t get_gamepad_state(uint32_t index, gamepad_state_t* p_gamepad_state)
{
    if (index >= gamepad::max_connected_gamepads || p_gamepad_state == nullptr)
        return gamepad::invalid_parameter;

    return call_internal_action(index, &internal_get_gamepad_state, p_gamepad_state);
}

int32_t get_gamepad_id(uint32_t index, gamepad_id_t* p_gamepad_id)
{
    if (index >= gamepad::max_connected_gamepads || p_gamepad_id == nullptr)
        return gamepad::invalid_parameter;

    return call_internal_action(index, &internal_get_gamepad_id, p_gamepad_id);
}

int32_t set_gamepad_vibration(uint32_t index, float left_strength, float right_strength)
{
    if (index >= gamepad::max_connected_gamepads || left_strength < 0.0f || right_strength < 0.0f)
        return gamepad::invalid_parameter;

    if (left_strength > 1.0f)
        left_strength = 1.0f;

    if (right_strength > 1.0f)
        right_strength = 1.0f;

    return call_internal_action(index, &internal_set_gamepad_vibration, left_strength, right_strength);
}

int32_t set_gamepad_led(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= gamepad::max_connected_gamepads)
        return gamepad::invalid_parameter;

    return call_internal_action(index, &internal_set_gamepad_led, r, g, b);
}

void free_gamepad_resources()
{
    std::lock_guard<std::mutex> lock(s_gamepad_mutex);

    internal_free_all_contexts();
}

#if defined(GAMEPAD_OS_WINDOWS)

struct gamepad_context_t
{
    HANDLE       hDevice;
    PWSTR        devicePath;

    uint8_t      dead;
    uint16_t     type;
    gamepad_id_t id;

    gamepad_state_t gamepadState;
};

static HRESULT DeviceIo(gamepad_context_t* context, DWORD ioControlCode, LPVOID inBuff, DWORD inBuffSize, LPVOID outBuff, DWORD outBuffSize, LPOVERLAPPED pOverlapped)
{
    HRESULT result;
    int BytesReturned;
    DWORD lpBytesReturned;

    lpBytesReturned = 0;
    BytesReturned = DeviceIoControl(context->hDevice, ioControlCode, inBuff, inBuffSize, outBuff, outBuffSize, &lpBytesReturned, pOverlapped);
    if (BytesReturned)
        return ERROR_SUCCESS;

    if (GetLastError() == ERROR_IO_PENDING && pOverlapped)
        result = E_PENDING;
    else
        result = E_FAIL;
    return result;
}

static HRESULT DeviceInIo(gamepad_context_t* context, DWORD nioctl, void* buff, size_t buff_size)
{
    return DeviceIo(context, nioctl, nullptr, 0, buff, buff_size, nullptr);
}

static HRESULT DeviceOutIo(gamepad_context_t* context, DWORD nioctl, void* buff, size_t buff_size)
{
    return DeviceIo(context, nioctl, buff, buff_size, nullptr, 0, nullptr);
}

static HRESULT DeviceInOutIo(gamepad_context_t* context, DWORD nioctl, void* in_buff, size_t in_buff_size, void* out_buff, size_t out_buff_size)
{
    return DeviceIo(context, nioctl, in_buff, in_buff_size, out_buff, out_buff_size, nullptr);
}

static int32_t parse_buffer(gamepad_state_t* p_gamepad_state, GamepadState0100 &buff)
{
    if (buff.status != 1) // Gamepad is offline
        return gamepad::failed;

    p_gamepad_state->left_stick.x = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbLX);
    p_gamepad_state->left_stick.y = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbLY);

    p_gamepad_state->right_stick.x = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbRX);
    p_gamepad_state->right_stick.y = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbRY);

    p_gamepad_state->buttons = buff.wButtons;

    p_gamepad_state->left_trigger = rerange_value(0.0f, 255.0f, 0.0f, 1.0f, buff.bLeftTrigger);
    p_gamepad_state->right_trigger = rerange_value(0.0f, 255.0f, 0.0f, 1.0f, buff.bRightTrigger);

    return gamepad::success;
}

static int32_t parse_buffer(gamepad_state_t* p_gamepad_state, GamepadState0101 &buff)
{
    if (buff.status != 1) // Gamepad is offline
        return gamepad::failed;

    p_gamepad_state->left_stick.x = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbLX);
    p_gamepad_state->left_stick.y = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbLY);

    p_gamepad_state->right_stick.x = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbRX);
    p_gamepad_state->right_stick.y = rerange_value(-32768.0f, 32767.0f, -1.0f, 1.0f, buff.sThumbRY);

    p_gamepad_state->buttons = buff.wButtons;

    if (buff.bExtraButtons & 0x01)
        p_gamepad_state->buttons |= button_share;

    p_gamepad_state->left_trigger = rerange_value(0.0f, 255.0f, 0.0f, 1.0f, buff.bLeftTrigger);
    p_gamepad_state->right_trigger = rerange_value(0.0f, 255.0f, 0.0f, 1.0f, buff.bRightTrigger);

    return gamepad::success;
}

static int32_t get_gamepad_infos(gamepad_context_t* p_context)
{
    WORD buffer[6] = { 0 };
    bool success = (DeviceInIo(p_context, IOCTL_XINPUT_GET_INFORMATION, buffer, sizeof(buffer)) >= 0 ? true : false);
    if (success)
    {
        if (!(buffer[2] & 0x80))
        {
            for (int i = 0; i < (buffer[1] & 0xFF); ++i)
            {
                //dwUserIndex = i;
                p_context->type = buffer[0];
                p_context->id.id = 0;
                p_context->id.vendorID = buffer[4];
                p_context->id.productID = buffer[5];

                {// This part might not be needed
                    char buff[5] = {
                        0,              // XInput User Index
                        XINPUT_LED_OFF, // LED Status
                        0,
                        0,
                        1 };

                    DeviceOutIo(p_context, IOCTL_XINPUT_SET_GAMEPAD_STATE, buff, sizeof(buff));
                }
            }
        }
    }
    return success;
}

static void close_device(gamepad_context_t* p_context)
{
    if (p_context->hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(p_context->hDevice);
        p_context->hDevice = INVALID_HANDLE_VALUE;
    }
}

static int32_t internal_create_context(gamepad_context_t** pp_context, PWSTR device_path)
{
    *pp_context = (gamepad_context_t*)malloc(sizeof(gamepad_context_t));

    if (*pp_context == nullptr)
        return gamepad::failed;

    (*pp_context)->hDevice = INVALID_HANDLE_VALUE;
    (*pp_context)->dead = false;

    memset(&(*pp_context)->gamepadState, 0, sizeof(gamepad_state_t));

    (*pp_context)->devicePath = _wcsdup(device_path);
    if ((*pp_context)->devicePath == nullptr)
        return gamepad::failed;

    (*pp_context)->hDevice = CreateFileW((*pp_context)->devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if ((*pp_context)->hDevice == INVALID_HANDLE_VALUE)
        return gamepad::failed;

    if (!get_gamepad_infos(*pp_context))
        return gamepad::failed;

    return gamepad::success;
}

static void internal_free_context(gamepad_context_t** pp_context)
{
    if (*pp_context == nullptr)
        return;

    close_device(*pp_context);

    if ((*pp_context)->devicePath != nullptr)
    {
        free((*pp_context)->devicePath);
        (*pp_context)->devicePath = nullptr;
    }

    free(*pp_context);
    *pp_context = nullptr;
}

static int32_t internal_get_gamepad(uint32_t index, gamepad_context_t** pp_context)
{
    HDEVINFO device_info_set;
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W* data;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    DWORD idx = 0;

    if (s_gamepads[index] != nullptr && !s_gamepads[index]->dead)
    {
        *pp_context = s_gamepads[index];
        return gamepad::success;
    }
    *pp_context = nullptr;

    // Didn't find the gamepad, going to check for new ones
    device_info_set = SetupDiGetClassDevsW(&XUSB_INTERFACE_CLASS_GUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)HeapAlloc(GetProcessHeap(), 0, sizeof(*data) + detail_size);
    if (data == nullptr)
        return gamepad::failed;

    data->cbSize = sizeof(*data);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    while (SetupDiEnumDeviceInterfaces(device_info_set, NULL, &XUSB_INTERFACE_CLASS_GUID, idx++, &interface_data))
    {
        if (!SetupDiGetDeviceInterfaceDetailW(device_info_set,
            &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
            continue;

        bool found = false;
        int free_device = -1;
        for (uint32_t i = 0; i < max_connected_gamepads; ++i)
        {
            if (s_gamepads[i] != nullptr)
            {
                if (!s_gamepads[i]->dead)
                {
                    if (wcscmp(s_gamepads[i]->devicePath, data->DevicePath) == 0)
                    {
                        found = true;
                        break;
                    }
                    continue;
                }
                internal_free_context(&s_gamepads[i]);
            }
            if (free_device == -1)
            {
                free_device = i;
            }
        }

        if (!found && free_device != -1)
        {
            if (internal_create_context(&s_gamepads[free_device], data->DevicePath) != gamepad::success)
            {
                internal_free_context(&s_gamepads[free_device]);
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(device_info_set);

    if(s_gamepads[index] != nullptr && !s_gamepads[index]->dead)
    {
        *pp_context = s_gamepads[index];
        return gamepad::success;
    }

    return gamepad::failed;
}

static int32_t internal_update_gamepad_state(gamepad_context_t* p_context)
{
    union
    {
        GamepadState0100 state0100;
        GamepadState0101 state0101;
    } out_buff;

    union
    {
        InGamepadState0100 state0100;
        InGamepadState0101 state0101;
    } in_buff;

    DWORD inBuffSize;
    DWORD outBuffSize;
    unsigned int res;

    memset(&out_buff, 0, sizeof(out_buff));
    memset(&in_buff, 0, sizeof(in_buff));

    if (p_context->type == 256)
    {// I don't know if its xboxOne stuff
        in_buff.state0100.DeviceIndex = 0;
        inBuffSize = sizeof(in_buff.state0100);
        outBuffSize = sizeof(out_buff.state0100);
    }
    else
    {
        in_buff.state0101.wType = 257;
        in_buff.state0101.DeviceIndex= 0;
        inBuffSize = sizeof(in_buff.state0101);
        outBuffSize = sizeof(out_buff.state0101);
    }

    res = DeviceInOutIo(p_context, IOCTL_XINPUT_GET_GAMEPAD_STATE, &in_buff, inBuffSize, &out_buff, outBuffSize);
    if (res < 0)
    {
        p_context->dead = 1;
        return gamepad::failed;
    }

    if (p_context->type == 256)
    {
        res = parse_buffer(&p_context->gamepadState, out_buff.state0100);
    }
    else
    {
        res = parse_buffer(&p_context->gamepadState, out_buff.state0101);
    }

    if (res != gamepad::success)
    {
        p_context->dead = 1;
    }

    return res;
}

static int32_t internal_get_gamepad_state(gamepad_context_t* p_context, gamepad_state_t* p_gamepad_state)
{
    memcpy(p_gamepad_state, &p_context->gamepadState, sizeof(gamepad_state_t));
    return gamepad::success;
}

static int32_t internal_get_gamepad_id(gamepad_context_t* p_context, gamepad_id_t* p_gamepad_id)
{
    p_gamepad_id->id = p_context->id.id;
    return gamepad::success;
}

static int32_t internal_set_gamepad_vibration(gamepad_context_t* p_context, float left_strength, float right_strength)
{
    uint8_t buff[5] = {
        0,
        0,
        static_cast<uint8_t>(left_strength * 255),
        static_cast<uint8_t>(right_strength * 255),
        2
    };

    int32_t res;
    if ((res = (DeviceOutIo(p_context, IOCTL_XINPUT_SET_GAMEPAD_STATE, buff, sizeof(buff)) == ERROR_SUCCESS ? gamepad::success : gamepad::failed)) != gamepad::success)
    {
        p_context->dead = 1;
    }

    return res;
}

static int32_t internal_set_gamepad_led(gamepad_context_t* p_context, uint8_t r, uint8_t g, uint8_t b)
{
    char buff[5] = {
                    0,              // XInput User Index
                    XINPUT_LED_OFF, // LED Status
                    0,
                    0,
                    1 };

    DeviceOutIo(p_context, IOCTL_XINPUT_SET_GAMEPAD_STATE, buff, sizeof(buff));

    return gamepad::failed;
}

void internal_free_all_contexts()
{
    for (uint32_t i = 0; i < gamepad::max_connected_gamepads; ++i)
    {
        internal_free_context(&s_gamepads[i]);
    }
}

#elif defined(GAMEPAD_OS_LINUX)

struct gamepad_context_t
{
    int eventFd;
    int ledFd;
    char* devicePath;

    int8_t dead;
    gamepad_id_t id;

    float axisMin[ABS_MAX];
    float axisMax[ABS_MAX];

    struct ff_effect rumbleEffect;
    //struct ff_effect effects[NUM_EFFECTS];

    gamepad_state_t gamepadState;
};

//static void get_available_effects()
//{
//    if (_event_fd != -1)
//    {
//        unsigned char ffFeatures[1 + FF_MAX / 8 / sizeof(unsigned char)] = { 0 };
//        if (ioctl(_event_fd, EVIOCGBIT(EV_FF, sizeof(ffFeatures) * sizeof(unsigned char)), ffFeatures) != -1)
//        {
//        }
//    }
//}

static int32_t play_effect(gamepad_context_t* p_context, struct ff_effect* p_effect)
{
    if (p_effect->id == -1 || p_context->eventFd == -1)
        return gamepad::failed;

    struct input_event play;

    play.type = EV_FF;
    play.code = p_effect->id;
    play.value = 1;

    if (write(p_context->eventFd, (const void*)&play, sizeof(play)) == -1)
    {
        //std::cout << "Failed to play effect " << effect_id << std::endl;
        return gamepad::failed;
    }

    return gamepad::success;
}

static void stop_effect(gamepad_context_t* p_context, struct ff_effect* p_effect)
{
    if (p_effect->id == -1 || p_context->eventFd == -1)
        return;

    struct input_event play;

    play.type = EV_FF;
    play.code = p_effect->id;
    play.value = 0;

    write(p_context->eventFd, (const void*)&play, sizeof(play));
    //if (write(p_context->eventFd, (const void*)&play, sizeof(play)) < 0)
    //{
    //
    //}
}

static int32_t register_effect(gamepad_context_t* p_context, struct ff_effect* p_effect)
{
    if (p_context->eventFd == -1 || ioctl(p_context->eventFd, EVIOCSFF, p_effect) < 0)
    {
        p_effect->id = -1;
        return gamepad::failed;
    }

    return gamepad::success;
}

static void unregister_effect(gamepad_context_t* p_context, struct ff_effect* p_effect)
{
    if (p_effect->id == -1 || p_context->eventFd == -1)
        return;

    stop_effect(p_context, p_effect);

    //if (ioctl(p_context->eventFd, EVIOCRMFF, p_effect->id) < -1)
    //{        
    //}
    ioctl(p_context->eventFd, EVIOCRMFF, p_effect->id);
    p_effect->id = -1;
}

static bool is_gamepad(const char* device_path)
{
    unsigned char evbit[1 + EV_CNT / 8 / sizeof(unsigned char)] = { 0 };
    unsigned char keybit[1 + KEY_CNT / 8 / sizeof(unsigned char)] = { 0 };
    unsigned char absbit[1 + ABS_CNT / 8 / sizeof(unsigned char)] = { 0 };

    int fd = open(device_path, O_RDONLY);
    if (fd == -1)
        return false;

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
            return true;
        }
    }

    close(fd);

    return false;
}

static int32_t get_gamepad_infos(gamepad_context_t* p_context)
{
    struct input_id inpid;
    if (ioctl(p_context->eventFd, EVIOCGID, &inpid) < 0)
        return gamepad::failed;

    p_context->id.productID = inpid.product;
    p_context->id.vendorID = inpid.vendor;

    struct input_absinfo absinfo;

    for (auto const& axis : { ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ })
    {
        if (ioctl(p_context->eventFd, EVIOCGABS(axis), &absinfo) >= 0)
        {
            if (axis == ABS_Y || axis == ABS_RY)
            {
                // On Y, down is positiv, up is negativ, so swap the values
                auto x = absinfo.minimum;
                absinfo.minimum = absinfo.maximum;
                absinfo.maximum = x;
            }

            p_context->axisMin[axis] = absinfo.minimum;
            p_context->axisMax[axis] = absinfo.maximum;
        }
        else // Failed to retrieve infos, device did not inform the OS of the ranges ?
        {// Set some common values
            switch (axis)
            {
                case ABS_X: case ABS_RX:
                    p_context->axisMin[axis] = -32768.0f;
                    p_context->axisMax[axis] = 32767.0f;
                    break;

                case ABS_Y: case ABS_RY:
                    p_context->axisMin[axis] = 32767.0f;
                    p_context->axisMax[axis] = -32768.0f;
                    break;

                case ABS_Z: case ABS_RZ:
                    p_context->axisMin[axis] = 0.0f;
                    p_context->axisMax[axis] = 255.0f;
                    break;
            }
        }
    }

    return gamepad::success;
}

static void close_device(gamepad_context_t* p_context)
{
    //for (int i = 0; i < NUM_EFFECTS; ++i)
    //    unregister_effect(&p_context->effects[i]);
    unregister_effect(p_context, &p_context->rumbleEffect);

    if (p_context->eventFd != -1)
    {
        close(p_context->eventFd);
        p_context->eventFd = -1;
    }
    if (p_context->ledFd != -1)
    {
        close(p_context->ledFd);
        p_context->ledFd = -1;
    }
}

static int32_t internal_create_context(gamepad_context_t** pp_context, const char* device_path)
{
    *pp_context = (gamepad_context_t*)malloc(sizeof(gamepad_context_t));

    if (*pp_context == nullptr)
        return gamepad::failed;

    //for (int i = 0; i < NUM_EFFECTS; ++i)
    //    (*pp_context)->effects[i].id = -1;
    (*pp_context)->rumbleEffect.id = -1;

    (*pp_context)->eventFd = -1;
    (*pp_context)->ledFd = -1;
    (*pp_context)->dead = false;

    memset(&(*pp_context)->gamepadState, 0, sizeof(gamepad_state_t));

    (*pp_context)->devicePath = strdup(device_path);
    if ((*pp_context)->devicePath == nullptr)
        return gamepad::failed;

    int gamepad_fd = open(device_path, O_RDWR | O_NONBLOCK);
    for (int i = 0; i < 75 && gamepad_fd == -1 && errno == EACCES; ++i)
    {// When you plugin a new device, it takes some time to set the acls for write access
     // So sleep for 1ms and retry 75 times for a total of 75ms retry, it success on ~40ms on my computer
        usleep(1000);
        gamepad_fd = open(device_path, O_RDWR | O_NONBLOCK);
    }

    if (gamepad_fd == -1)// Try without write permission (no rumble)
        gamepad_fd = open(device_path, O_RDONLY);

    if (gamepad_fd == -1)
        return gamepad::failed;

    (*pp_context)->eventFd = gamepad_fd;

    // TODO: led
    //led_path = "/sys/class/leds/xpad<ID>/brightness";

    return get_gamepad_infos(*pp_context);
}

static void internal_free_context(gamepad_context_t** pp_context)
{
    if (*pp_context == nullptr)
        return;

    close_device(*pp_context);
    if ((*pp_context)->devicePath != nullptr)
    {
        free((*pp_context)->devicePath);
        (*pp_context)->devicePath = nullptr;
    }
    free(*pp_context);
    *pp_context = nullptr;
}

static int32_t internal_get_gamepad(uint32_t index, gamepad_context_t** pp_context)
{
    DIR* input_dir;
    struct dirent* input_dir_entry;
    uint32_t device_path_len = 20;
    char* device_path;
    uint32_t entry_len;

    if (s_gamepads[index] != nullptr && !s_gamepads[index]->dead)
    {
        *pp_context = s_gamepads[index];
        return gamepad::success;
    }
    *pp_context = nullptr;

    // Didn't find the gamepad, going to check for new ones
    input_dir = opendir("/dev/input");
    if (input_dir == nullptr)
        return gamepad::failed;

    device_path = (char*)malloc(sizeof(char) * device_path_len);
    strcpy(device_path, "/dev/input/");

    while ((input_dir_entry = readdir(input_dir)) != nullptr)
    {
        if (strncmp(input_dir_entry->d_name, "event", 5) != 0)
            continue;

        //                     /dev/input/
        entry_len = sizeof(char) * (11 + strlen(input_dir_entry->d_name) + 1);
        if (entry_len > device_path_len)
        {
            free(device_path);
            device_path_len = entry_len;
            device_path = (char*)malloc(sizeof(char) * device_path_len);
            strcpy(device_path, "/dev/input/");
        }

        strcpy(device_path + 11, input_dir_entry->d_name);

        if (!is_gamepad(device_path))
            continue;

        bool found = false;
        int free_device = -1;
        for (uint32_t i = 0; i < max_connected_gamepads; ++i)
        {
            if (s_gamepads[i] != nullptr)
            {
                if (!s_gamepads[i]->dead)
                {
                    if (strcmp(s_gamepads[i]->devicePath, device_path) == 0)
                    {
                        found = true;
                        break;
                    }
                    continue;
                }
                internal_free_context(&s_gamepads[i]);
            }
            if (free_device == -1)
            {
                free_device = i;
            }
        }

        if (!found && free_device != -1)
        {
            if (internal_create_context(&s_gamepads[free_device], device_path) != gamepad::success)
            {
                internal_free_context(&s_gamepads[free_device]);
            }
        }
    }

    free(device_path);
    closedir(input_dir);

    if (s_gamepads[index] != nullptr && !s_gamepads[index]->dead)
    {
        *pp_context = s_gamepads[index];
        return gamepad::success;
    }

    return gamepad::failed;
}

constexpr inline uint32_t set_button_value(uint32_t& buttons, uint32_t value, bool activated)
{
    if (activated)
        buttons |= value;
    else
        buttons &= ~value;
}

static int32_t internal_update_gamepad_state(gamepad_context_t* p_context)
{
    struct input_event events[32];
    int num_events;
    bool r = false;
    while ((num_events = read(p_context->eventFd, events, (sizeof events))) > 0)
    {
        r = true;
        num_events /= sizeof(*events);
        for (int i = 0; i < num_events; ++i)
        {
            auto const& event_code = events[i].code;
            auto const& event_value = events[i].value;
            switch (events[i].type)
            {
                case EV_KEY:
                    switch (event_code)
                    {
                        case BTN_A     : set_button_value(p_context->gamepadState.buttons, gamepad::button_a             , event_value); break;
                        case BTN_B     : set_button_value(p_context->gamepadState.buttons, gamepad::button_b             , event_value); break;
                        case BTN_X     : set_button_value(p_context->gamepadState.buttons, gamepad::button_x             , event_value); break;
                        case BTN_Y     : set_button_value(p_context->gamepadState.buttons, gamepad::button_y             , event_value); break;
                        case BTN_TL    : set_button_value(p_context->gamepadState.buttons, gamepad::button_left_shoulder , event_value); break;
                        case BTN_TR    : set_button_value(p_context->gamepadState.buttons, gamepad::button_right_shoulder, event_value); break;
                        case BTN_SELECT: set_button_value(p_context->gamepadState.buttons, gamepad::button_back          , event_value); break;
                        case BTN_START : set_button_value(p_context->gamepadState.buttons, gamepad::button_start         , event_value); break;
                        case BTN_THUMBL: set_button_value(p_context->gamepadState.buttons, gamepad::button_left_thumb    , event_value); break;
                        case BTN_THUMBR: set_button_value(p_context->gamepadState.buttons, gamepad::button_right_thumb   , event_value); break;
                        case BTN_MODE  : set_button_value(p_context->gamepadState.buttons, gamepad::button_guide         , event_value); break;
                    }
                    break;

                case EV_ABS:
                    switch (event_code)
                    {
                        case ABS_X:
                            p_context->gamepadState.left_stick.x = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], -1.0f, 1.0f, event_value);
                            break;

                        case ABS_Y:
                            p_context->gamepadState.left_stick.y = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], -1.0f, 1.0f, event_value);
                            break;

                        case ABS_RX:
                            p_context->gamepadState.right_stick.x = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], -1.0f, 1.0f, event_value);
                            break;

                        case ABS_RY:
                            p_context->gamepadState.right_stick.y = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], -1.0f, 1.0f, event_value);
                            break;

                        case ABS_Z:
                            p_context->gamepadState.left_trigger = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], 0.0f, 1.0f, event_value);
                            break;

                        case ABS_RZ:
                            p_context->gamepadState.right_trigger = rerange_value(p_context->axisMin[event_code], p_context->axisMax[event_code], 0.0f, 1.0f, event_value);
                            break;

                        case ABS_HAT0X:
                            if (event_value == 0)
                            {
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_left, false);
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_right, false);
                            }
                            else
                            {
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_left, event_value < 0);
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_right, event_value > 0);
                            }
                            break;

                        case ABS_HAT0Y:
                            if (event_value == 0)
                            {
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_up, false);
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_down, false);
                            }
                            else
                            {
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_up, event_value < 0);
                                set_button_value(p_context->gamepadState.buttons, gamepad::button_down, event_value > 0);
                            }
                        break;
                    }
                    break;
            }
        }
    }

    if (errno != EWOULDBLOCK && errno != EAGAIN)
    {
        p_context->dead = 1;
        return gamepad::failed;
    }

    return gamepad::success;
}

static int32_t internal_get_gamepad_state(gamepad_context_t* p_context, gamepad_state_t* p_gamepad_state)
{
    memcpy(p_gamepad_state, &p_context->gamepadState, sizeof(gamepad_state_t));
    return gamepad::success;
}

static int32_t internal_get_gamepad_id(gamepad_context_t* p_context, gamepad_id_t* p_gamepad_id)
{
    p_gamepad_id->id = p_context->id.id;
    return gamepad::success;
}

static int32_t internal_set_gamepad_vibration(gamepad_context_t* p_context, float left_strength, float right_strength)
{
    struct ff_effect* p_effect = &p_context->rumbleEffect;

    int32_t res;

    if (p_effect->id != -1)
    {
        unregister_effect(p_context, p_effect);
    }

    p_effect->type = FF_RUMBLE;
    p_effect->id = -1;
    p_effect->u.rumble.strong_magnitude = static_cast<uint16_t>(left_strength * 65535);
    p_effect->u.rumble.weak_magnitude = static_cast<uint16_t>(right_strength * 65535);
    p_effect->replay.length = 0xffffu;
    p_effect->replay.delay = 0;

    if ((res = register_effect(p_context, p_effect)) != gamepad::success)
        return res;

    return play_effect(p_context, p_effect);
}

static int32_t internal_set_gamepad_led(gamepad_context_t* p_context, uint8_t r, uint8_t g, uint8_t b)
{
    return gamepad::failed;
}

void internal_free_all_contexts()
{
    for (uint32_t i = 0; i < gamepad::max_connected_gamepads; ++i)
    {
        internal_free_context(&s_gamepads[i]);
    }
}

#elif defined(GAMEPAD_OS_APPLE)

struct gamepad_context_t
{
};

static int32_t internal_get_gamepad(uint32_t index, gamepad_context_t** pp_context)
{
    return gamepad::failed;
}

static int32_t internal_update_gamepad_state(gamepad_context_t* p_context)
{
    return gamepad::failed;
}

static int32_t internal_get_gamepad_state(gamepad_context_t* p_context, gamepad_state_t* p_gamepad_state)
{
    return gamepad::failed;
}

static int32_t internal_get_gamepad_id(gamepad_context_t* p_context, gamepad_id_t* p_gamepad_id)
{
    return gamepad::failed;
}

static int32_t internal_set_gamepad_vibration(gamepad_context_t* p_context, float left_strength, float right_strength)
{
    return gamepad::failed;
}

static int32_t internal_set_gamepad_led(gamepad_context_t* p_context, uint8_t r, uint8_t g, uint8_t b)
{
    return gamepad::failed;
}

static void internal_free_all_contexts()
{
}

#endif

}// namespace gamepad