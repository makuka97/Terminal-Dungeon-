#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <map>

// Constants
const int PLAYER_MAX_HEALTH = 100;
const int BOSS_MAX_HEALTH = 10;
const int ENEMY_HEALTH = 2;

// Enums
enum Direction {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

enum GameState {
    STATE_PLAYING,
    STATE_BOSS_FIGHT,
    STATE_VICTORY
};

// Structures
struct Position {
    int x, y;
    Position() : x(0), y(0) {}
    Position(int px, int py) : x(px), y(py) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct Player {
    Position pos;
    Position spawnPos;
    int health;
    Direction facing;
    bool isAttacking;
    int attackFrames;
    
    Player() : health(PLAYER_MAX_HEALTH), facing(DIR_DOWN), 
               isAttacking(false), attackFrames(0) {}
};

struct Enemy {
    Position pos;
    int health;
    bool isAlive;
    
    Enemy() : health(ENEMY_HEALTH), isAlive(true) {}
    Enemy(int x, int y) : pos(x, y), health(ENEMY_HEALTH), isAlive(true) {}
};

struct Spike {
    Position pos;
    int animationFrame;
    
    Spike() : animationFrame(0) {}
    Spike(int x, int y) : pos(x, y), animationFrame(0) {}
};

struct Boss {
    Position pos;
    int health;
    Direction moveDir;
    int shootTimer;
    bool isAlive;
    
    Boss() : health(BOSS_MAX_HEALTH), moveDir(DIR_RIGHT), 
             shootTimer(0), isAlive(true) {}
    Boss(int x, int y) : pos(x, y), health(BOSS_MAX_HEALTH), 
                         moveDir(DIR_RIGHT), shootTimer(0), isAlive(true) {}
};

struct Flame {
    Position pos;
    Direction dir;
    int lifetime;
    
    Flame(int x, int y, Direction d) : pos(x, y), dir(d), lifetime(20) {}
};

struct Door {
    Position pos;
    bool isLocked;
    
    Door(int x, int y, bool locked) : pos(x, y), isLocked(locked) {}
};

struct PuzzleState {
    std::vector<int> clickedOrder;
    bool isSolved;
    
    PuzzleState() : isSolved(false) {}
};

struct Room {
    std::string name;
    std::vector<std::string> grid;
    int width, height;
    
    Position playerSpawn;
    std::vector<Enemy> enemies;
    std::vector<Spike> spikes;
    std::vector<Door> doors;
    std::vector<Position> puzzleNumbers;
    Boss* boss;
    PuzzleState puzzleState;
    
    std::string linkUp, linkDown, linkLeft, linkRight;
    
    Room() : width(0), height(0), boss(nullptr) {}
    ~Room() {
        if (boss) delete boss;
    }
};

// Game Engine Class
class GameEngine {
private:
    Player player;
    Room* currentRoom;
    std::map<std::string, Room*> loadedRooms;
    std::vector<Flame> flames;
    GameState gameState;
    int frameCounter;
    
public:
    GameEngine();
    ~GameEngine();
    
    void initialize();
    void run();
    void update();
    void render();
    void handleInput();
    
    void loadRoom(const std::string& filename);
    void switchRoom(const std::string& roomName);
    Room* parseRoomFile(const std::string& filename);
    void parseMetadata(Room* room, const std::string& line);
    
    void movePlayer(int dx, int dy);
    void attackPlayer();
    bool canMoveTo(int x, int y);
    
    void checkPlayerAttackHit();
    void dealDamageToPlayer(int damage);
    void checkEnemyCollisions();
    void checkSpikeCollisions();
    void checkFlameCollisions();
    void checkBossCollision();
    void respawnPlayer();
    
    void updateEnemies();
    void updateBoss();
    void moveBoss();
    void bossShootFlames();
    void updateFlames();
    void updateSpikes();
    
    void checkPuzzleStep(int x, int y);
    void solvePuzzle();
    
    Door* getDoorAt(int x, int y);
};

#endif // GAME_H