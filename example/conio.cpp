#include "conio.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>

void GetConsoleInfos(HANDLE hConsole, CONSOLE_SCREEN_BUFFER_INFOEX* pInfos)
{
    pInfos->cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    GetConsoleScreenBufferInfoEx(hConsole, pInfos);
}

void gotoxy(int x, int y)
{
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;
    
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole , &Infos);

    if (x >= 0 && y >= 0 && x <= Infos.dwSize.X && y <= Infos.dwSize.Y)
    {
        COORD Coords;
        Coords.X = x;
        Coords.Y = y;
        SetConsoleCursorPosition(hConsole, Coords);
    }
}

void clrscr()
{
    DWORD NumberOfCharWritten = 0;
    COORD Coordinates;
    
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole, &Infos);

    Coordinates.X = 0;
    
    for (Coordinates.Y = 0; Coordinates.Y < Infos.dwSize.Y; Coordinates.Y++)
    {
        FillConsoleOutputCharacterA(hConsole, ' ', Infos.dwSize.X, Coordinates, &NumberOfCharWritten);
        FillConsoleOutputAttribute(hConsole, Infos.wAttributes, Infos.dwSize.X, Coordinates, &NumberOfCharWritten);
    }
    gotoxy(1, 1);
}

void clreol()
{
	COORD coords;
	DWORD NumberOfCharWritten = 0;
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole, &Infos);
	coords.X = Infos.dwCursorPosition.X;
    coords.Y = Infos.dwCursorPosition.Y;
	FillConsoleOutputCharacterA(hConsole, ' ', (Infos.dwSize.X - coords.X), coords, &NumberOfCharWritten);
}

bool setcursortype(unsigned int Type)
{
    HANDLE HConsole;
    CONSOLE_CURSOR_INFO CursorInfo;

    HConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    switch (Type)
    {
        case 0:// Hide
            CursorInfo.dwSize = 0x01;
            CursorInfo.bVisible = 0;
            break;

        case 1:// Default
            CursorInfo.dwSize = 0x19;
            CursorInfo.bVisible = 1;
            break;

        case 2:// Big
            CursorInfo.dwSize = 0x64;
            CursorInfo.bVisible = 1;
            break;
    }
    
    return SetConsoleCursorInfo(HConsole, &CursorInfo);
}

WORD getcolor()
{
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole, &Infos);

    return Infos.wAttributes & 0xFF;
}

void setcolor(unsigned short color)
{
    HANDLE hConsole;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void textcolor(unsigned short color)
{
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole, &Infos);

    SetConsoleTextAttribute(hConsole, (color & 0x0F) | (Infos.wAttributes & 0xF0));
}

void textbackground(unsigned short color)
{
    CONSOLE_SCREEN_BUFFER_INFOEX Infos;
    HANDLE hConsole;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleInfos(hConsole, &Infos);

    SetConsoleTextAttribute(hConsole, ((color & 0x0F) << 4) | (Infos.wAttributes & 0x0F));
}

bool textmode(int x, int y)
{
    HANDLE hConsole;
    SMALL_RECT ConsoleWindow;
    COORD coords = { x, y };

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    ConsoleWindow.Top = 0; ConsoleWindow.Left = 0;
    ConsoleWindow.Bottom = coords.Y - 1;
    ConsoleWindow.Right = coords.X - 1;

    SetConsoleScreenBufferSize(hConsole, coords);
    Sleep(40); // This sleep is needed
    SetConsoleWindowInfo(hConsole, 1, &ConsoleWindow);
    Sleep(40); // This sleep is needed
    return SetConsoleScreenBufferSize(hConsole, coords);
}

#elif defined(__linux__) || defined(__APPLE__)
#include <stdio.h>

void clrscr()
{
    printf("\x1B[H\x1B[2J");
}

void clreol()
{
    printf("\x1B[K");
}

void gotoxy(int x, int y)
{
    printf("\x1b[%d;%df", y+1, x+1);
}

void textcolor(unsigned short color)
{
    if(color > 15)
        return;

    char s_color[] = "0;30";
    if(color & 0x08)
      s_color[0] = '1';
    
    s_color[3] += (color & 0x01 ? 4 : 0);
    s_color[3] += (color & 0x02 ? 2 : 0);
    s_color[3] += (color & 0x04 ? 1 : 0);

    printf("\x1b[%sm", s_color);
}

#endif
