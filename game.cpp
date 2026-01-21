#include "game.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <conio.h>
#include <windows.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

GameEngine::GameEngine() 
    : currentRoom(nullptr), gameState(STATE_PLAYING), frameCounter(0) {
    srand(time(0));
}

GameEngine::~GameEngine() {
    for (auto& pair : loadedRooms) {
        delete pair.second;
    }
}

void GameEngine::initialize() {
    loadRoom("start.txt");
    switchRoom("start.txt");
    gameState = STATE_PLAYING;
    player.health = PLAYER_MAX_HEALTH;
}

void GameEngine::loadRoom(const string& filename) {
    if (loadedRooms.find(filename) != loadedRooms.end()) return;
    
    Room* room = parseRoomFile(filename);
    if (room) {
        room->name = filename;
        loadedRooms[filename] = room;
    }
}

Room* GameEngine::parseRoomFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return nullptr;
    }
    
    Room* room = new Room();
    string line;
    int rowIndex = 0;
    
    while (getline(file, line)) {
        // Skip metadata lines (lines starting with # and containing =)
        if (line.length() > 0 && line[0] == '#' && line.find('=') != string::npos) {
            parseMetadata(room, line);
            continue;
        }
        
        // Parse special characters in this row and remove them from the line
        for (int col = 0; col < (int)line.length(); col++) {
            char c = line[col];
            
            if (c == '@') {
                room->playerSpawn = Position(col, rowIndex);
                line[col] = ' '; // Replace @ with space
            }
            else if (c == '{' || c == '}') {
                room->enemies.push_back(Enemy(col, rowIndex));
                line[col] = ' '; // Clear from grid
            }
            else if (c == '^' || c == '_') {
                room->spikes.push_back(Spike(col, rowIndex));
                line[col] = ' '; // Clear from grid
            }
            else if (c == '[' && col + 2 < (int)line.length()) {
                if (line[col+1] == 'x' && line[col+2] == ']') {
                    room->doors.push_back(Door(col, rowIndex, true));
                }
                else if (line[col+1] == 'L' && line[col+2] == ']') {
                    room->doors.push_back(Door(col, rowIndex, false));
                }
            }
            else if (c >= '1' && c <= '9') {
                room->puzzleNumbers.push_back(Position(col, rowIndex));
                // DON'T remove numbers - keep them visible for the puzzle
            }
            else if (c == '<' && col + 3 < (int)line.length() && 
                     line[col+1] == '[' && line[col+2] == ']' && line[col+3] == '>') {
                room->boss = new Boss(col, rowIndex);
                // Clear boss from grid
                line[col] = ' ';
                line[col+1] = ' ';
                line[col+2] = ' ';
                line[col+3] = ' ';
            }
        }
        
        // Add the modified line to grid AFTER cleaning it
        room->grid.push_back(line);
        rowIndex++;
    }
    
    room->height = room->grid.size();
    room->width = 0;
    for (const auto& row : room->grid) {
        if ((int)row.length() > room->width) {
            room->width = row.length();
        }
    }
    
    file.close();
    return room;
}

void GameEngine::parseMetadata(Room* room, const string& line) {
    size_t equalPos = line.find('=');
    if (equalPos == string::npos) return;
    
    string key = line.substr(1, equalPos - 1);
    string value = line.substr(equalPos + 1);
    
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    if (key == "link_up") room->linkUp = value;
    else if (key == "link_down") room->linkDown = value;
    else if (key == "link_left") room->linkLeft = value;
    else if (key == "link_right") room->linkRight = value;
}

void GameEngine::switchRoom(const string& roomName) {
    if (loadedRooms.find(roomName) == loadedRooms.end()) {
        loadRoom(roomName);
    }
    
    currentRoom = loadedRooms[roomName];
    player.pos = currentRoom->playerSpawn;
    player.spawnPos = currentRoom->playerSpawn;
    
    if (currentRoom->boss && currentRoom->boss->isAlive) {
        gameState = STATE_BOSS_FIGHT;
    } else {
        gameState = STATE_PLAYING;
    }
}

void GameEngine::run() {
    while (gameState != STATE_VICTORY) {
        handleInput();
        update();
        render();
        Sleep(50);
    }
    
    system("cls");
    cout << "\n\n  ====================================\n";
    cout << "          VICTORY!\n";
    cout << "  ====================================\n";
    cout << "\n  You have completed the adventure!\n\n";
}

void GameEngine::update() {
    frameCounter++;
    
    if (player.isAttacking) {
        player.attackFrames++;
        if (player.attackFrames > 5) {
            player.isAttacking = false;
            player.attackFrames = 0;
        }
    }
    
    if (frameCounter % 10 == 0) updateSpikes();
    if (frameCounter % 8 == 0) updateEnemies();
    
    if (currentRoom->boss && currentRoom->boss->isAlive) {
        if (frameCounter % 1 == 0) updateBoss();
    }
    
    updateFlames();
    
    checkEnemyCollisions();
    checkSpikeCollisions();
    checkFlameCollisions();
    checkBossCollision();
    
    if (player.health <= 0) {
        respawnPlayer();
    }
}

void GameEngine::handleInput() {
    if (_kbhit()) {
        char key = _getch();
        
        if (key == 'w' || key == 'W') {
            movePlayer(0, -1);
            player.facing = DIR_UP;
        }
        else if (key == 's' || key == 'S') {
            movePlayer(0, 1);
            player.facing = DIR_DOWN;
        }
        else if (key == 'a' || key == 'A') {
            movePlayer(-1, 0);
            player.facing = DIR_LEFT;
        }
        else if (key == 'd' || key == 'D') {
            movePlayer(1, 0);
            player.facing = DIR_RIGHT;
        }
        else if (key == ' ') {
            attackPlayer();
        }
    }
}

void GameEngine::render() {
    system("cls");
    
    // HUD
    cout << "Life: " << player.health << "  Weapon: Sword";
    
    if (currentRoom->boss && currentRoom->boss->isAlive) {
        cout << "  Evil ASCII: ";
        for (int i = 0; i < currentRoom->boss->health; i++) {
            cout << "*";
        }
    }
    
    cout << "\n";
    
    // Render room
    for (int y = 0; y < currentRoom->height; y++) {
        for (int x = 0; x < currentRoom->width; x++) {
            bool rendered = false;
            
            // Player with attack animation
            if (player.pos.x == x && player.pos.y == y) {
                if (player.isAttacking) {
                    // Show attack animation based on direction
                    if (player.facing == DIR_RIGHT) {
                        cout << "@_";
                        x++; // Skip next position since we printed 2 chars
                    } else if (player.facing == DIR_LEFT) {
                        cout << "_@";
                        x++; // Skip next position
                    } else {
                        cout << '@'; // Up/Down just show player
                    }
                } else {
                    cout << '@';
                }
                rendered = true;
            }
            
            // Attack animation for up/down (printed on separate line)
            if (!rendered && player.isAttacking) {
                if (player.facing == DIR_UP && x == player.pos.x && y == player.pos.y - 1) {
                    cout << '|';
                    rendered = true;
                }
                else if (player.facing == DIR_DOWN && x == player.pos.x && y == player.pos.y + 1) {
                    cout << '|';
                    rendered = true;
                }
            }
            
            // Enemies
            if (!rendered) {
                for (const auto& enemy : currentRoom->enemies) {
                    if (enemy.isAlive && enemy.pos.x == x && enemy.pos.y == y) {
                        cout << '{';
                        rendered = true;
                        break;
                    }
                }
            }
            
            // Boss
            if (!rendered && currentRoom->boss && currentRoom->boss->isAlive) {
                Boss* boss = currentRoom->boss;
                if (y == boss->pos.y && x >= boss->pos.x && x < boss->pos.x + 4) {
                    int offset = x - boss->pos.x;
                    if (offset == 0) cout << '<';
                    else if (offset == 1) cout << '[';
                    else if (offset == 2) cout << ']';
                    else if (offset == 3) cout << '>';
                    rendered = true;
                }
            }
            
            // Flames
            if (!rendered) {
                for (const auto& flame : flames) {
                    if (flame.pos.x == x && flame.pos.y == y) {
                        cout << '~';
                        rendered = true;
                        break;
                    }
                }
            }
            
            // Spikes
            if (!rendered) {
                for (const auto& spike : currentRoom->spikes) {
                    if (spike.pos.x == x && spike.pos.y == y) {
                        if (spike.animationFrame == 1) cout << '^';
                        else cout << '_';
                        rendered = true;
                        break;
                    }
                }
            }
            
            // Room tile
            if (!rendered) {
                if (y < (int)currentRoom->grid.size() && x < (int)currentRoom->grid[y].length()) {
                    cout << currentRoom->grid[y][x];
                } else {
                    cout << ' ';
                }
            }
        }
        cout << "\n";
    }
}

void GameEngine::movePlayer(int dx, int dy) {
    int newX = player.pos.x + dx;
    int newY = player.pos.y + dy;
    
    if (canMoveTo(newX, newY)) {
        player.pos.x = newX;
        player.pos.y = newY;
        
        Door* door = getDoorAt(newX, newY);
        if (door && !door->isLocked) {
            string nextRoom = "";
            if (dx < 0) nextRoom = currentRoom->linkLeft;
            else if (dx > 0) nextRoom = currentRoom->linkRight;
            else if (dy < 0) nextRoom = currentRoom->linkUp;
            else if (dy > 0) nextRoom = currentRoom->linkDown;
            
            if (nextRoom != "" && nextRoom != "none") {
                switchRoom(nextRoom);
            }
        }
        
        if (!currentRoom->puzzleNumbers.empty()) {
            checkPuzzleStep(newX, newY);
        }
    }
}

bool GameEngine::canMoveTo(int x, int y) {
    if (x < 0 || y < 0 || y >= currentRoom->height) return false;
    if (y >= (int)currentRoom->grid.size() || x >= (int)currentRoom->grid[y].length()) return false;
    
    char tile = currentRoom->grid[y][x];
    if (tile == '#') return false;
    
    Door* door = getDoorAt(x, y);
    if (door && door->isLocked) return false;
    
    return true;
}

void GameEngine::attackPlayer() {
    player.isAttacking = true;
    player.attackFrames = 0;
    checkPlayerAttackHit();
}

void GameEngine::checkPlayerAttackHit() {
    int attackX = player.pos.x;
    int attackY = player.pos.y;
    
    if (player.facing == DIR_UP) attackY--;
    else if (player.facing == DIR_DOWN) attackY++;
    else if (player.facing == DIR_LEFT) attackX--;
    else if (player.facing == DIR_RIGHT) attackX++;
    
    for (auto& enemy : currentRoom->enemies) {
        if (enemy.isAlive && enemy.pos.x == attackX && enemy.pos.y == attackY) {
            enemy.health--;
            if (enemy.health <= 0) enemy.isAlive = false;
        }
    }
    
    if (currentRoom->boss && currentRoom->boss->isAlive) {
        Boss* boss = currentRoom->boss;
        if (attackY == boss->pos.y && attackX >= boss->pos.x && attackX < boss->pos.x + 4) {
            boss->health--;
            if (boss->health <= 0) {
                boss->isAlive = false;
                gameState = STATE_PLAYING;
            }
        }
    }
}

void GameEngine::dealDamageToPlayer(int damage) {
    player.health -= damage;
    if (player.health < 0) player.health = 0;
}

void GameEngine::checkEnemyCollisions() {
    for (const auto& enemy : currentRoom->enemies) {
        if (enemy.isAlive && enemy.pos == player.pos && !player.isAttacking) {
            dealDamageToPlayer(1);
        }
    }
}

void GameEngine::checkSpikeCollisions() {
    for (const auto& spike : currentRoom->spikes) {
        if (spike.pos == player.pos && spike.animationFrame == 1) {
            dealDamageToPlayer(1);
        }
    }
}

void GameEngine::checkFlameCollisions() {
    for (const auto& flame : flames) {
        if (flame.pos == player.pos) {
            dealDamageToPlayer(1);
        }
    }
}

void GameEngine::checkBossCollision() {
    if (currentRoom->boss && currentRoom->boss->isAlive) {
        Boss* boss = currentRoom->boss;
        if (player.pos.y == boss->pos.y && 
            player.pos.x >= boss->pos.x && player.pos.x < boss->pos.x + 4 &&
            !player.isAttacking) {
            dealDamageToPlayer(1);
        }
    }
}

void GameEngine::respawnPlayer() {
    player.health = PLAYER_MAX_HEALTH;
    player.pos = player.spawnPos;
}

void GameEngine::updateEnemies() {
    for (auto& enemy : currentRoom->enemies) {
        if (!enemy.isAlive) continue;
        
        int dx = player.pos.x - enemy.pos.x;
        int dy = player.pos.y - enemy.pos.y;
        
        int moveX = 0, moveY = 0;
        if (abs(dx) > abs(dy)) {
            moveX = (dx > 0) ? 1 : -1;
        } else {
            moveY = (dy > 0) ? 1 : -1;
        }
        
        int newX = enemy.pos.x + moveX;
        int newY = enemy.pos.y + moveY;
        
        if (newX >= 0 && newY >= 0 && newY < currentRoom->height &&
            newY < (int)currentRoom->grid.size() && newX < (int)currentRoom->grid[newY].length()) {
            if (currentRoom->grid[newY][newX] != '#') {
                enemy.pos.x = newX;
                enemy.pos.y = newY;
            }
        }
    }
}

void GameEngine::updateBoss() {
    if (!currentRoom->boss || !currentRoom->boss->isAlive) return;
    
    moveBoss();
    
    currentRoom->boss->shootTimer++;
    if (currentRoom->boss->shootTimer > 5) {
        bossShootFlames();
        currentRoom->boss->shootTimer = 0;
    }
}

void GameEngine::moveBoss() {
    Boss* boss = currentRoom->boss;
    
    int dx = 0, dy = 0;
    if (boss->moveDir == DIR_UP) dy = -1;
    else if (boss->moveDir == DIR_DOWN) dy = 1;
    else if (boss->moveDir == DIR_LEFT) dx = -1;
    else if (boss->moveDir == DIR_RIGHT) dx = 1;
    
    int newX = boss->pos.x + dx;
    int newY = boss->pos.y + dy;
    
    bool canMove = true;
    if (newY < 0 || newY >= currentRoom->height || newX < 0 || newX + 3 >= currentRoom->width) {
        canMove = false;
    } else {
        for (int i = 0; i < 4; i++) {
            if (newY >= (int)currentRoom->grid.size() || 
                newX + i >= (int)currentRoom->grid[newY].length() ||
                currentRoom->grid[newY][newX + i] == '#') {
                canMove = false;
                break;
            }
        }
    }
    
    if (canMove) {
        boss->pos.x = newX;
        boss->pos.y = newY;
    } else {
        if (boss->moveDir == DIR_UP) boss->moveDir = DIR_DOWN;
        else if (boss->moveDir == DIR_DOWN) boss->moveDir = DIR_UP;
        else if (boss->moveDir == DIR_LEFT) boss->moveDir = DIR_RIGHT;
        else if (boss->moveDir == DIR_RIGHT) boss->moveDir = DIR_LEFT;
    }
}

void GameEngine::bossShootFlames() {
    Boss* boss = currentRoom->boss;
    int shootType = rand() % 3;
    
    if (shootType == 0) {
        flames.push_back(Flame(boss->pos.x - 1, boss->pos.y, DIR_LEFT));
    }
    else if (shootType == 1) {
        flames.push_back(Flame(boss->pos.x + 4, boss->pos.y, DIR_RIGHT));
    }
    else {
        flames.push_back(Flame(boss->pos.x - 1, boss->pos.y, DIR_LEFT));
        flames.push_back(Flame(boss->pos.x + 4, boss->pos.y, DIR_RIGHT));
    }
}

void GameEngine::updateFlames() {
    for (int i = flames.size() - 1; i >= 0; i--) {
        flames[i].lifetime--;
        
        if (flames[i].lifetime <= 0) {
            flames.erase(flames.begin() + i);
            continue;
        }
        
        int dx = (flames[i].dir == DIR_LEFT) ? -1 : 1;
        flames[i].pos.x += dx;
        
        if (flames[i].pos.x < 0 || flames[i].pos.x >= currentRoom->width ||
            flames[i].pos.y < 0 || flames[i].pos.y >= currentRoom->height) {
            flames.erase(flames.begin() + i);
            continue;
        }
        
        if (flames[i].pos.y < (int)currentRoom->grid.size() &&
            flames[i].pos.x < (int)currentRoom->grid[flames[i].pos.y].length() &&
            currentRoom->grid[flames[i].pos.y][flames[i].pos.x] == '#') {
            flames.erase(flames.begin() + i);
        }
    }
}

void GameEngine::updateSpikes() {
    for (auto& spike : currentRoom->spikes) {
        spike.animationFrame = (spike.animationFrame + 1) % 3;
    }
}

void GameEngine::checkPuzzleStep(int x, int y) {
    if (currentRoom->puzzleState.isSolved) return;
    
    char tile = currentRoom->grid[y][x];
    if (tile >= '1' && tile <= '9') {
        int number = tile - '0';
        
        // Check if already clicked
        bool alreadyClicked = false;
        for (int n : currentRoom->puzzleState.clickedOrder) {
            if (n == number) {
                alreadyClicked = true;
                break;
            }
        }
        
        if (!alreadyClicked) {
            currentRoom->puzzleState.clickedOrder.push_back(number);
            
            // Check if all numbers have been stepped on
            if (currentRoom->puzzleState.clickedOrder.size() == currentRoom->puzzleNumbers.size()) {
                solvePuzzle();
            }
        }
    }
}

void GameEngine::solvePuzzle() {
    currentRoom->puzzleState.isSolved = true;
    
    // Unlock all doors in the room by changing them in the grid
    for (auto& door : currentRoom->doors) {
        door.isLocked = false;
        
        // Also update the visual representation in the grid
        if (door.pos.y < (int)currentRoom->grid.size()) {
            string& row = currentRoom->grid[door.pos.y];
            if (door.pos.x + 2 < (int)row.length()) {
                if (row[door.pos.x] == '[' && row[door.pos.x+1] == 'x' && row[door.pos.x+2] == ']') {
                    row[door.pos.x+1] = 'L'; // Change [x] to [L]
                }
            }
        }
    }
}

Door* GameEngine::getDoorAt(int x, int y) {
    for (auto& door : currentRoom->doors) {
        if (door.pos.y == y && x >= door.pos.x && x < door.pos.x + 3) {
            return &door;
        }
    }
    return nullptr;
}