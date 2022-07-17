#include <gamepad/gamepad.h>

#include <memory>
#include <thread>
#include <string.h>

#include "conio.h"

#ifdef _WIN32
#define SPRINTF sprintf_s

#else
#define SPRINTF sprintf

#endif

enum DeviceConsoleBufferIndex
{
    DEVICE_STATUS = 0                 ,
    DEVICE_ID_HEADER                  ,
    DEVICE_VENDOR_ID                  ,
    DEVICE_PRODUCT_ID                 ,
    DEVICE_STATE_HEADER               ,
    DEVICE_STATE_BUTTON_DPAD_UP       ,
    DEVICE_STATE_BUTTON_DPAD_DOWN     ,
    DEVICE_STATE_BUTTON_DPAD_LEFT     ,
    DEVICE_STATE_BUTTON_DPAD_RIGHT    ,
    DEVICE_STATE_BUTTON_START         ,
    DEVICE_STATE_BUTTON_BACK          ,
    DEVICE_STATE_BUTTON_LEFT_THUMB    ,
    DEVICE_STATE_BUTTON_RIGHT_THUMB   ,
    DEVICE_STATE_BUTTON_LEFT_SHOULDER ,
    DEVICE_STATE_BUTTON_RIGHT_SHOULDER,
    DEVICE_STATE_BUTTON_GUIDE         ,
    DEVICE_STATE_BUTTON_A             ,
    DEVICE_STATE_BUTTON_B             ,
    DEVICE_STATE_BUTTON_X             ,
    DEVICE_STATE_BUTTON_Y             ,
    DEVICE_STATE_SHARE                ,
    DEVICE_STATE_LEFTTRIGGER          ,
    DEVICE_STATE_RIGHTTRIGGER         ,
    DEVICE_STATE_LEFTTHUMBX           ,
    DEVICE_STATE_LEFTTHUMBY           ,
    DEVICE_STATE_RIGHTTHUMBX          ,
    DEVICE_STATE_RIGHTTHUMBY          ,
    DEVICE_MAX_CONSOLE_LINES
};

#define MAX_DEVICE_LINE_WIDTH 35
#define STR_MAX_DEVICE_LINE_WIDTH "35"

struct GamepadDevice_t
{
    bool connected;
    int device_index;
    char console_buffer[DEVICE_MAX_CONSOLE_LINES][MAX_DEVICE_LINE_WIDTH+1];
    gamepad::gamepad_id_t id;
    gamepad::gamepad_state_t old_state;
    gamepad::gamepad_state_t state;
};

void BuildDeviceConsoleOutput(GamepadDevice_t& device)
{
    // Status
    SPRINTF(device.console_buffer[DEVICE_STATUS]                     , "     Device %u %s !", device.device_index, device.connected ? "connected" : "disconnected");
    // Id
    SPRINTF(device.console_buffer[DEVICE_ID_HEADER]                  , "Device ID:");
    SPRINTF(device.console_buffer[DEVICE_VENDOR_ID]                  , "  - Vendor ID : %04x", device.id.vendorID);
    SPRINTF(device.console_buffer[DEVICE_PRODUCT_ID]                 , "  - Product ID: %04x", device.id.productID);
    // State
    SPRINTF(device.console_buffer[DEVICE_STATE_HEADER]               , "Device State:");
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_DPAD_UP]       , "  - UP       : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_up));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_DPAD_DOWN]     , "  - DOWN     : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_down));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_DPAD_LEFT]     , "  - LEFT     : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_left));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_DPAD_RIGHT]    , "  - RIGHT    : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_right));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_START]         , "  - START    : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_start));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_BACK]          , "  - BACK     : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_back));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_LEFT_THUMB]    , "  - LTHUMB   : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_left_thumb));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_RIGHT_THUMB]   , "  - RTHUMB   : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_right_thumb));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_LEFT_SHOULDER] , "  - LSHOULDER: %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_left_shoulder));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_RIGHT_SHOULDER], "  - RSHOULDER: %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_right_shoulder));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_GUIDE]         , "  - GUIDE    : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_guide));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_A]             , "  - A        : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_a));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_B]             , "  - B        : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_b));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_X]             , "  - X        : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_x));
    SPRINTF(device.console_buffer[DEVICE_STATE_BUTTON_Y]             , "  - Y        : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_y));
    SPRINTF(device.console_buffer[DEVICE_STATE_SHARE]                , "  - SHARE    : %u"   , gamepad::are_all_pressed(device.state.buttons, gamepad::button_share));
    SPRINTF(device.console_buffer[DEVICE_STATE_LEFTTRIGGER]          , "  - LTrigger : %.2f" , device.state.left_trigger);
    SPRINTF(device.console_buffer[DEVICE_STATE_RIGHTTRIGGER]         , "  - RTrigger : %.2f" , device.state.right_trigger);
    SPRINTF(device.console_buffer[DEVICE_STATE_LEFTTHUMBX]           , "  - Left X   : %.2f" , device.state.left_stick.x);
    SPRINTF(device.console_buffer[DEVICE_STATE_LEFTTHUMBY]           , "  - Left Y   : %.2f" , device.state.left_stick.y);
    SPRINTF(device.console_buffer[DEVICE_STATE_RIGHTTHUMBX]          , "  - Right X  : %.2f" , device.state.right_stick.x);
    SPRINTF(device.console_buffer[DEVICE_STATE_RIGHTTHUMBY]          , "  - Right Y  : %.2f" , device.state.right_stick.y);
}

void PrintDeviceConsoleOutput(GamepadDevice_t& device)
{
    for (int i = 0; i < DEVICE_MAX_CONSOLE_LINES; ++i)
    {
        gotoxy(MAX_DEVICE_LINE_WIDTH * device.device_index, i);
        if(i == DEVICE_STATUS)
        {
            textcolor(device.connected ? COLOR_GREEN : COLOR_LIGHTRED);
        }
        else if(i > DEVICE_STATE_HEADER && i < DEVICE_MAX_CONSOLE_LINES)
        {
            textcolor(COLOR_GRAY);
        }
        else
        {
            textcolor(COLOR_GRAY);
        }
        printf("%-" STR_MAX_DEVICE_LINE_WIDTH "s", device.console_buffer[i]);
    }
    fflush(stdout);
}

void OnDeviceInfoChange(GamepadDevice_t& device)
{
    BuildDeviceConsoleOutput(device);
    PrintDeviceConsoleOutput(device);
    memcpy(&device.old_state, &device.state, sizeof(device.state));
}

void OnDeviceConnect(GamepadDevice_t& device)
{
    if(gamepad::get_gamepad_id(device.device_index, &device.id) != gamepad::success)
        return;

    device.connected = true;
    memset(&device.old_state, 0, sizeof(device.old_state));
    memset(&device.state, 0, sizeof(device.state));

    OnDeviceInfoChange(device);
}

void OnDeviceDisconnect(GamepadDevice_t& device)
{
    device.connected = false;
    memset(&device.id, 0xff, sizeof(device.id));
    memset(&device.state, 0, sizeof(device.state));

    OnDeviceInfoChange(device);
}

int main(int argc, char *argv[])
{
    int MaxControllerCount = 2;
    std::unique_ptr<GamepadDevice_t[]> devices(new GamepadDevice_t[MaxControllerCount]);

    for (int i = 0; i < MaxControllerCount; ++i)
    {
        devices[i].device_index = i;
        OnDeviceDisconnect(devices[i]);
    }

    while (1)
    {
        for (int i = 0; i < MaxControllerCount; ++i)
        {
            GamepadDevice_t& device = devices[i];
            if (gamepad::update_gamepad_state(i) == gamepad::success && gamepad::get_gamepad_state(i, &device.state) == gamepad::success)
            {
                if (device.connected == false)
                {
                    OnDeviceConnect(device);
                }
                else if (memcmp(&device.old_state, &device.state, sizeof(device.state)) != 0)
                {
                    OnDeviceInfoChange(device);
                }
            }
            else if (device.connected)
            {
                OnDeviceDisconnect(device);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}
