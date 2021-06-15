#include "gamepad.h"

#include <iostream>
#include <thread>

void example_read_gamepad()
{
    gamepad::gamepad_state_t state = {};
    while (1)
    {
        //for (int i = 0; i < gamepad::max_connected_gamepads; ++i)
        for (int i = 0; i < 2; ++i)
        {
            if (gamepad::update_gamepad_state(i) == gamepad::success && gamepad::get_gamepad_state(i, &state) == gamepad::success)
            {
                std::cout << "Gamepad: " << i << std::endl;
                std::cout << "A    : " << gamepad::are_all_pressed(state.buttons, gamepad::button_a             ) << std::endl
                          << "B    : " << gamepad::are_all_pressed(state.buttons, gamepad::button_b             ) << std::endl
                          << "X    : " << gamepad::are_all_pressed(state.buttons, gamepad::button_x             ) << std::endl
                          << "Y    : " << gamepad::are_all_pressed(state.buttons, gamepad::button_y             ) << std::endl
                          << "UP   : " << gamepad::are_all_pressed(state.buttons, gamepad::button_up            ) << std::endl
                          << "DOWN : " << gamepad::are_all_pressed(state.buttons, gamepad::button_down          ) << std::endl
                          << "LEFT : " << gamepad::are_all_pressed(state.buttons, gamepad::button_left          ) << std::endl
                          << "RIGHT: " << gamepad::are_all_pressed(state.buttons, gamepad::button_right         ) << std::endl
                          << "START: " << gamepad::are_all_pressed(state.buttons, gamepad::button_start         ) << std::endl
                          << "BACK : " << gamepad::are_all_pressed(state.buttons, gamepad::button_back          ) << std::endl
                          << "XBOX : " << gamepad::are_all_pressed(state.buttons, gamepad::button_guide         ) << std::endl
                          << "L1   : " << gamepad::are_all_pressed(state.buttons, gamepad::button_left_shoulder ) << std::endl
                          << "R1   : " << gamepad::are_all_pressed(state.buttons, gamepad::button_right_shoulder) << std::endl
                          << "L2   : " << (state.left_trigger  > gamepad::gamepad_trigger_threshold)              << std::endl
                          << "R2   : " << (state.right_trigger > gamepad::gamepad_trigger_threshold)              << std::endl
                          << "L3   : " << gamepad::are_all_pressed(state.buttons, gamepad::button_left_thumb    ) << std::endl
                          << "R3   : " << gamepad::are_all_pressed(state.buttons, gamepad::button_right_thumb   ) << std::endl
                          << "LX   : " << (state.left_stick.x                                                   ) << std::endl
                          << "LY   : " << (state.left_stick.y                                                   ) << std::endl
                          << "RX   : " << (state.right_stick.x                                                  ) << std::endl
                          << "RY   : " << (state.right_stick.y                                                  ) << std::endl
                          << std::endl;

                if (gamepad::are_all_pressed(state.buttons, gamepad::button_start | gamepad::button_back))
                    return;

                std::this_thread::sleep_for(std::chrono::milliseconds{250});
            }
        }
    }
}

void example_set_vibration()
{
    float a(0.0f), b(0.0f);
    uint32_t index;
    while (a != -1.0f && b != -1.0f)
    {
        std::cout << "Input: <gamepad index> <left strength [0.0f, 1.0f]> <right strength [0.0f, 1.0f]>, use -1 to leave the loop." << std::endl;
        std::cin >> index >> a >> b;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        gamepad::set_gamepad_vibration(index, a, b);
    }
}

int main(int argc, char *argv[])
{
    example_set_vibration();
    example_read_gamepad();

    return 0;
}
