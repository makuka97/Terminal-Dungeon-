#include "game.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

using namespace std;

void displayTitleScreen() {
    system("cls");
    
    cout << "\n\n\n\n\n\n";
    cout << "  ============================================\n";
    cout << "\n";
    cout << "           THE LEGEND OF ASCII\n";
    cout << "\n";
    cout << "  ============================================\n";
    cout << "\n\n";
    cout << "         Written by Micajah Nordyke\n";
    cout << "\n\n\n";
    cout << "          Press any key to play...\n";
    cout << "\n\n\n\n\n";
    
    _getch();
}

int main() {
    // Set console size
    system("mode con: cols=100 lines=50");
    
    // Hide cursor
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    displayTitleScreen();
    
    GameEngine game;
    game.initialize();
    game.run();
    
    // Show cursor again
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    cout << "\n\n  Thanks for playing!\n\n";
    
    return 0;
}