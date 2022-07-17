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

#if defined(WIN64) || defined(_WIN64) || defined(__MINGW64__) \
 || defined(WIN32) || defined(_WIN32) || defined(__MINGW32__)
    #define GAMEPAD_OS_WINDOWS
#elif defined(__linux__) || defined(linux)
    #define GAMEPAD_OS_LINUX
#elif defined(__APPLE__)
    #define GAMEPAD_OS_APPLE
#else
    #error "Unknown OS for gamepad library."
#endif

#if defined(GAMEPAD_OS_WINDOWS)

 // Will need SetupAPI.lib
 //#pragma comment(lib, "SetupAPI.lib")

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

#define GAMEPAD_HIDDEN_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        static const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

GAMEPAD_HIDDEN_GUID(XUSB_INTERFACE_CLASS_GUID, 0xEC87F1E3, 0xC13B, 0x4100, 0xB5, 0xF7, 0x8B, 0x84, 0xD5, 0x42, 0x60, 0xCB);

#define IOCTL_XINPUT_BASE  0x8000

#define IOCTL_XINPUT_GET_INFORMATION          CTL_CODE(IOCTL_XINPUT_BASE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)                     // 0x80006000
#define IOCTL_XINPUT_GET_CAPABILITIES         CTL_CODE(IOCTL_XINPUT_BASE, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E004
#define IOCTL_XINPUT_GET_LED_STATE            CTL_CODE(IOCTL_XINPUT_BASE, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E008
#define IOCTL_XINPUT_GET_GAMEPAD_STATE        CTL_CODE(IOCTL_XINPUT_BASE, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E00C
#define IOCTL_XINPUT_SET_GAMEPAD_STATE        CTL_CODE(IOCTL_XINPUT_BASE, 0x804, METHOD_BUFFERED, FILE_WRITE_ACCESS)                    // 0x8000A010
#define IOCTL_XINPUT_WAIT_FOR_GUIDE_BUTTON    CTL_CODE(IOCTL_XINPUT_BASE, 0x805, METHOD_BUFFERED, FILE_WRITE_ACCESS)                    // 0x8000A014
#define IOCTL_XINPUT_GET_BATTERY_INFORMATION  CTL_CODE(IOCTL_XINPUT_BASE, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E018
#define IOCTL_XINPUT_POWER_DOWN_DEVICE        CTL_CODE(IOCTL_XINPUT_BASE, 0x807, METHOD_BUFFERED, FILE_WRITE_ACCESS)                    // 0x8000A01C
#define IOCTL_XINPUT_GET_AUDIO_INFORMATION    CTL_CODE(IOCTL_XINPUT_BASE, 0x808, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E020
#define IOCTL_XINPUT_GET_BASE_BUS_INFORMATION CTL_CODE(IOCTL_XINPUT_BASE, 0x8FF, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // 0x8000E3FC

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
struct InGamepadState0100
{
    BYTE DeviceIndex;
};

struct GamepadState0100
{
    BYTE  status;
    BYTE  unk0;
    BYTE  inputId;
    DWORD dwPacketNumber;
    BYTE  unk2;
    WORD  wButtons;
    BYTE  bLeftTrigger;
    BYTE  bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
};

struct InGamepadState0101
{
    WORD wType;
    BYTE DeviceIndex;
};

struct GamepadState0101
{
    BYTE  unk0;
    BYTE  inputId;
    BYTE  status;
    BYTE  unk2;
    BYTE  unk3;
    DWORD dwPacketNumber;
    BYTE  unk4;
    BYTE  unk5;
    WORD  wButtons;
    BYTE  bLeftTrigger;
    BYTE  bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
    BYTE  unk6;
    BYTE  unk7;
    BYTE  unk8;
    BYTE  unk9;
    BYTE  unk10;
    BYTE  bExtraButtons;
};

#pragma pack(pop)

#elif defined(GAMEPAD_OS_LINUX)

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

#elif defined(GAMEPAD_OS_APPLE)

#endif