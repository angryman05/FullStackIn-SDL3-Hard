#include <iostream>
#include <vector>
#include <SDL3/SDL.h>

#include "circle.h"
#include "rectangle.h"
#include <random>

struct Enemy {
    Rectangle body;
    int slice;
    float fireTimer;
};

//define window dimensions
constexpr int WINDOW_WIDTH{700};
constexpr int WINDOW_HEIGHT{800};

// Vertical scrolling grid related variables
constexpr float VERTICAL_SCROLLING_SPEED{ 100 };
constexpr int VERTICAL_SCROLLING_GRID_SQUARE_SIZE{ 20 };
std::vector<Rectangle> scrollingGrid;

// Animation related variables
float DT{ 0.01f };
Uint64 startTime{ 0 };
Uint64 endTime{ 0 };

//define SDL Window related variables
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};
SDL_Event currentEvent;
SDL_Color backgroundColor{255, 255, 255, 255};

bool quit{false};
bool gameOver{false};
bool playerWon{false};

float mouseX{-1.0f}, mouseY{-1.0f};

float displayScale{1.0f};


glm::vec2 mouseDelta{0.0f, 0.0f};
bool leftMouseDown{false};
float playerFireTimer{0.0f};
//stiu ca nu respecta cerinta dar am adaugat variabila pt sensivitate ca se misca prea incet
constexpr float MOUSE_SENSITIVITY{20.0f}; // eu zic ca e mai ok asa, eu joc mai high sensitivity ca is invatat de la osu!
constexpr int ENEMIES_TO_WIN{5}; // am pus 5 enemies ca ii mai usor de testat
int killedEnemies{0};
constexpr int ENEMY_SLICES{3};
constexpr float ENEMY_SPAWN_INTERVAL{3.0f};
constexpr float ENEMY_FIRST_SHOT_DELAY{1.0f};
constexpr float ENEMY_FIRE_INTERVAL{3.0f};
constexpr float ENEMY_PROJECTILE_SPEED{100.0f};
constexpr float ENEMY_MIN_SPEED{50.0f};
constexpr float ENEMY_MAX_SPEED{150.0f};
constexpr float PLAYER_ACCELERATION{400.0f};
constexpr float PLAYER_DECELERATION{800.0f};
constexpr float PLAYER_MAX_SPEED{500.0f};
constexpr float PLAYER_FIRE_INTERVAL{0.3f};
constexpr float PROJECTILE_RADIUS{10.0f};
constexpr float PLAYER_PROJECTILE_SPEED{500.0f};
SDL_Color green{0, 255, 0, 255};
SDL_Color orange{255, 165, 0, 255};
SDL_Color red{255, 0, 0, 255};
SDL_Color magenta{255, 0, 255, 255};
SDL_Color blue{0, 0, 255, 255};

float enemySpawnTimer{0.0f};
std::mt19937 rng{std::random_device{}()};

Rectangle player;
std::vector<Enemy> enemies;
std::vector<Circle> proiectile_inamici, proiectile_player;

float randomFloat(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(rng);
}

glm::vec2 rotateVec(glm::vec2 v, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

glm::vec2 getLogicalWindowSize() {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return glm::vec2(w / displayScale, h / displayScale);
}

void animatePlayer() {
    if (glm::length(mouseDelta) > 0.0f) {
        player.acc = mouseDelta * PLAYER_ACCELERATION * MOUSE_SENSITIVITY;
        player.vel += player.acc * DT;
        float speed = glm::length(player.vel);
        if (speed > PLAYER_MAX_SPEED) {
            player.vel = player.vel / speed * PLAYER_MAX_SPEED;
        }
    } else {
        player.acc = glm::vec2(0.0f);
        float speed = glm::length(player.vel);
        if (speed > 0.0f) {
            float newSpeed = glm::max(0.0f, speed - PLAYER_DECELERATION * DT);
            player.vel = player.vel / speed * newSpeed;
        }
    }

    player.pos += player.vel * DT;

    glm::vec2 windowSize = getLogicalWindowSize();
    float halfW = player.width / 2.0f;
    float halfH = player.height / 2.0f;

    if (player.pos.x < halfW) {
        player.pos.x = halfW;
        player.vel.x = 0.0f;
    } else if (player.pos.x > windowSize.x - halfW) {
        player.pos.x = windowSize.x - halfW;
        player.vel.x = 0.0f;
    }

    if (player.pos.y < halfH) {
        player.pos.y = halfH;
        player.vel.y = 0.0f;
    } else if (player.pos.y > windowSize.y - halfH) {
        player.pos.y = windowSize.y - halfH;
        player.vel.y = 0.0f;
    }

    mouseDelta = glm::vec2(0.0f);
}
void animatePlayerProjectiles() {
    if (leftMouseDown) {
        playerFireTimer += DT;
        while (playerFireTimer >= PLAYER_FIRE_INTERVAL) {
            playerFireTimer -= PLAYER_FIRE_INTERVAL;
            Circle projectile(PROJECTILE_RADIUS, player.pos, red);
            projectile.vel = glm::vec2(0.0f, -PLAYER_PROJECTILE_SPEED);
            projectile.acc = glm::vec2(0.0f);
            proiectile_player.push_back(projectile);
        }
    } else {
        playerFireTimer = PLAYER_FIRE_INTERVAL;
    }

    for (Circle& projectile : proiectile_player) {
        projectile.pos += projectile.vel * DT;
    }

    std::erase_if(proiectile_player, [](const Circle& p) {
        return p.pos.y < -p.radius;
    });
}

void initScollingRectangles() {
    int numGridRectangles = (WINDOW_HEIGHT / VERTICAL_SCROLLING_GRID_SQUARE_SIZE) / 2 + 2;

    for (int i = 0; i < numGridRectangles; i++) {
        Rectangle newGridRectangleLeft, newGridRectangleRight;

        newGridRectangleLeft.width = newGridRectangleLeft.height = VERTICAL_SCROLLING_GRID_SQUARE_SIZE;
        newGridRectangleLeft.pos.x = newGridRectangleLeft.width / 2.0f;
        newGridRectangleLeft.pos.y = WINDOW_HEIGHT - newGridRectangleLeft.height / 2.0f - 2.0f * i * newGridRectangleLeft.height;
        newGridRectangleLeft.vel = glm::vec2(0.0f, VERTICAL_SCROLLING_SPEED);
        newGridRectangleLeft.color = SDL_Color{ 235, 235, 235, 255 };

        newGridRectangleRight.width = newGridRectangleRight.height = VERTICAL_SCROLLING_GRID_SQUARE_SIZE;
        newGridRectangleRight.pos.x = WINDOW_WIDTH - newGridRectangleRight.width / 2.0f;
        newGridRectangleRight.pos.y = WINDOW_HEIGHT - newGridRectangleRight.height / 2.0f - 2.0f * i * newGridRectangleRight.height;
        newGridRectangleRight.vel = glm::vec2(0.0f, VERTICAL_SCROLLING_SPEED);
        newGridRectangleRight.color = SDL_Color{ 235, 235, 235, 255 };

        scrollingGrid.push_back(newGridRectangleLeft);
        scrollingGrid.push_back(newGridRectangleRight);
    }
}

bool initWindow() {
    bool success{true};

    //Try to initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s\n", SDL_GetError());
        success = false;
    } else {
        //Try to create the window and renderer
        displayScale = SDL_GetDisplayContentScale(1);

        if (!SDL_CreateWindowAndRenderer(
            "SDL Hello World Example",
            static_cast<int>(displayScale * WINDOW_WIDTH),
            static_cast<int>(displayScale * WINDOW_HEIGHT),
            0,
            &window, &renderer)) {
            SDL_Log("Failed to create window and renderer: %s\n", SDL_GetError());
            success = false;
        } else {
            //Apply global display scaling to renderer
            SDL_SetRenderScale(renderer, displayScale, displayScale);

            //Set background color
            SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b,
                                   backgroundColor.a);

            //Apply background color
            SDL_RenderClear(renderer);
        }
    }
    SDL_SetWindowRelativeMouseMode(window, true);
    return success;
}

void resetGame() {
    player = Rectangle(50.0f, 50.0f, glm::vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 25.0f), green);
    player.vel = glm::vec2(0.0f);
    player.acc = glm::vec2(0.0f);
    enemySpawnTimer = ENEMY_SPAWN_INTERVAL;

    enemies.clear();
    proiectile_inamici.clear();
    proiectile_player.clear();

    gameOver = false;
    playerWon = false;

    killedEnemies = 0;
    mouseDelta = glm::vec2(0.0f);
    scrollingGrid.clear();
    initScollingRectangles();

}

void processEvents() {
    //Check for events in queue
    while (SDL_PollEvent(&currentEvent)) {
        //User requests quit
        if (currentEvent.type == SDL_EVENT_QUIT) {
            quit = true;
        }

        //Mouse event -> pressed button
        if (currentEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (currentEvent.button.button == SDL_BUTTON_LEFT) {
                leftMouseDown = true;
            }
        }

        if (currentEvent.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (currentEvent.button.button == SDL_BUTTON_LEFT) {
                leftMouseDown = false;
            }
        }

        //Mouse event -> mouse movement
        if (currentEvent.type == SDL_EVENT_MOUSE_MOTION) {
            mouseDelta.x += currentEvent.motion.xrel / displayScale;
            mouseDelta.y += currentEvent.motion.yrel / displayScale;
        }

        //Keyboard event
        if (currentEvent.type == SDL_EVENT_KEY_DOWN) {
            switch (currentEvent.key.key) {
                case SDLK_UP:
                    std::cout << "UP arrow key was pressed" << std::endl;
                    break;
                case SDLK_DOWN:
                    std::cout << "DOWN arrow key was pressed" << std::endl;
                    break;
                case SDLK_LEFT:
                    std::cout << "LEFT arrow key was pressed" << std::endl;
                    break;
                case SDLK_RIGHT:
                    std::cout << "RIGHT arrow key was pressed" << std::endl;
                    break;
                case SDLK_R:
                    std::cout << "Se reseteaza jocu adevarat" << std::endl;
                    if (gameOver)
                        resetGame();
                    break;
                case SDLK_S:
                    std::cout << "S key was pressed" << std::endl;
                    break;
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                default:
                    break;
            }
        }
    }
}

void spawnEnemies() {
    glm::vec2 windowSize = getLogicalWindowSize();
    float sliceWidth = windowSize.x / ENEMY_SLICES;
    float size = 50.0f;

    for (int slice = 0; slice < ENEMY_SLICES; slice++) {
        bool occupied = false;
        for (const Enemy& e : enemies) {
            if (e.slice == slice) {
                occupied = true;
                break;
            }
        }
        if (occupied) {
            continue;
        }

        float minX = slice * sliceWidth + size / 2.0f;
        float maxX = (slice + 1) * sliceWidth - size / 2.0f;
        float x = randomFloat(minX, maxX);

        Enemy enemy;
        enemy.body = Rectangle(size, size, glm::vec2(x, -size / 2.0f), orange);
        enemy.body.vel = glm::vec2(0.0f, randomFloat(ENEMY_MIN_SPEED, ENEMY_MAX_SPEED));
        enemy.body.acc = glm::vec2(0.0f);
        enemy.slice = slice;
        enemy.fireTimer = ENEMY_FIRST_SHOT_DELAY;
        enemies.push_back(enemy);
    }
}

void enemyFire(const Enemy& enemy) {
    glm::vec2 toPlayer = player.pos - enemy.body.pos;
    float len = glm::length(toPlayer);
    glm::vec2 dir = len > 0.0f ? toPlayer / len : glm::vec2(0.0f, 1.0f);

    float angles[3] = {0.0f, glm::radians(15.0f), glm::radians(-15.0f)};
    for (float angle : angles) {
        Circle projectile(PROJECTILE_RADIUS, enemy.body.pos, magenta);
        projectile.vel = rotateVec(dir, angle) * ENEMY_PROJECTILE_SPEED;
        projectile.acc = glm::vec2(0.0f);
        proiectile_inamici.push_back(projectile);
    }
}

void animateEnemies() {
    enemySpawnTimer += DT;
    if (enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
        enemySpawnTimer -= ENEMY_SPAWN_INTERVAL;
        spawnEnemies();
    }

    glm::vec2 windowSize = getLogicalWindowSize();

    for (Enemy& enemy : enemies) {
        enemy.body.pos += enemy.body.vel * DT;

        enemy.fireTimer -= DT;
        if (enemy.fireTimer <= 0.0f) {
            enemy.fireTimer += ENEMY_FIRE_INTERVAL;
            enemyFire(enemy);
        }
    }

    std::erase_if(enemies, [&](const Enemy& e) {
        return e.body.pos.y - e.body.height / 2.0f > windowSize.y;
    });

    for (Circle& projectile : proiectile_inamici) {
        projectile.pos += projectile.vel * DT;
    }

    std::erase_if(proiectile_inamici, [&](const Circle& p) {
        return p.pos.x < -p.radius || p.pos.x > windowSize.x + p.radius ||
               p.pos.y < -p.radius || p.pos.y > windowSize.y + p.radius;
    });
}

bool circleHitsRect(const Circle& c, const Rectangle& r) {
    float closestX = glm::clamp(c.pos.x, r.pos.x - r.width / 2.0f, r.pos.x + r.width / 2.0f);
    float closestY = glm::clamp(c.pos.y, r.pos.y - r.height / 2.0f, r.pos.y + r.height / 2.0f);
    return glm::length(c.pos - glm::vec2(closestX, closestY)) <= c.radius;
}

bool rectsOverlap(const Rectangle& a, const Rectangle& b) {
    return std::abs(a.pos.x - b.pos.x) < (a.width + b.width) / 2.0f &&
           std::abs(a.pos.y - b.pos.y) < (a.height + b.height) / 2.0f;
}

void endGame(bool won) {
    gameOver = true;
    playerWon = won;
    enemies.clear();
    proiectile_inamici.clear();
    proiectile_player.clear();
    player.vel = glm::vec2(0.0f);
    player.acc = glm::vec2(0.0f);
    player.color = won ? blue : red;
}

void handleCollisions() {
    for (size_t i = 0; i < proiectile_player.size();) {
        bool hit = false;
        for (size_t j = 0; j < enemies.size(); j++) {
            if (circleHitsRect(proiectile_player[i], enemies[j].body)) {
                enemies.erase(enemies.begin() + j);
                killedEnemies++;
                hit = true;
                break;
            }
        }
        if (hit) {
            proiectile_player.erase(proiectile_player.begin() + i);
        } else {
            i++;
        }
    }

    for (const Enemy& enemy : enemies) {
        if (rectsOverlap(player, enemy.body)) {
            endGame(false);
            return;
        }
    }

    for (const Circle& projectile : proiectile_inamici) {
        if (circleHitsRect(projectile, player)) {
            endGame(false);
            return;
        }
    }

    if (killedEnemies >= ENEMIES_TO_WIN) {
        endGame(true);
    }
}

void animate() {

    if (gameOver)
        return;

    // Scrolling grid
    for (int i = 0; i < scrollingGrid.size(); i++) {
        Rectangle& gridRectangle = scrollingGrid.at(i);

        gridRectangle.pos += gridRectangle.vel * DT;

        if (gridRectangle.pos.y > (WINDOW_HEIGHT + gridRectangle.height / 2.0f)) {
            gridRectangle.pos.y -= gridRectangle.height * scrollingGrid.size();
        }
    }

    animatePlayer();
    animatePlayerProjectiles();
    animateEnemies();
    handleCollisions();
}

void drawFrame() {
    //Clear the background
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    //Draw scene

    //Draw scrolling grid

    for (int i = 0; i < scrollingGrid.size(); i++) {
        Rectangle& gridRectangle = scrollingGrid.at(i);
        gridRectangle.draw(renderer);
    }

    for (Circle& projectile : proiectile_player) {
        projectile.draw(renderer);
    }

    for (Circle& projectile : proiectile_inamici) {
        projectile.draw(renderer);
    }

    for (Enemy& enemy : enemies) {
        enemy.body.draw(renderer);
    }

    player.draw(renderer);
    //Update window
    SDL_RenderPresent(renderer);
}

void cleanup() {
    //Destroy renderer
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    //Destroy window
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    //Quit SDL
    SDL_Quit();
}



int main() {
    //Initialize window
    if (!initWindow()) {
        std::cout << "Failed to initialize" << std::endl;
        return -1;
    }

    resetGame();

    SDL_zero(currentEvent);

	initScollingRectangles();

    //Game loop
    while (!quit) {
        startTime = SDL_GetTicks();

        processEvents();
        animate();
        drawFrame();

        endTime = SDL_GetTicks();
        DT = (endTime - startTime) / 1000.0f;
    }

    cleanup();
    return 0;
}
