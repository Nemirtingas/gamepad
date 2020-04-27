#include "gamepad.h"

#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[])
{
  while(1)
  {
    int i = 0;
    auto gamepads = Gamepad::get_gamepads(true);
    for(auto& gp : gamepads)
    {
      if(gp->RunFrame())
      {
        cout << "Gamepad: " << i << endl;
        cout << "A: "     << gp->a << ' '
             << "B: "     << gp->b << ' '
             << "X: "     << gp->x << ' '
             << "Y: "     << gp->y << ' '
             << "UP: "    << gp->up << ' '
             << "DOWN: "  << gp->down << ' '
             << "LEFT: "  << gp->left << ' '
             << "RIGHT: " << gp->right << ' '
             << "START: " << gp->start<< ' '
             << "BACK: "  << gp->back << ' '
             << "L1: "    << gp->left_shoulder << ' '
             << "R1: "    << gp->right_shoulder << ' '
             << "L2: "    << gp->left_trigger << ' '
             << "R2: "    << gp->right_trigger << ' '
             << "LT: "    << gp->left_thumb << ' '
             << "RT: "    << gp->right_thumb << ' '
             << "LX: "    << gp->left_stick.x << ' '
             << "LY: "    << gp->left_stick.y << ' '
             << "RX: "    << gp->right_stick.x << ' '
             << "RY: "    << gp->right_stick.y << ' '
             << endl;
      }
      ++i;
    }
  }

  return 0;
}
