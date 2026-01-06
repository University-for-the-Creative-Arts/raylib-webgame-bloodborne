#include "raylib.h"
#include <algorithm>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/fetch.h>
#endif

#include <string>
#include <vector>

// Window size
const int screenWidth = 800;
const int screenHeight = 600;

// Player properties
Rectangle player = { screenWidth/2 - 25, screenHeight - 60, 50, 50 };
float playerSpeed = 400.0f;

// Falling objects
struct FallingObject {
    Rectangle rect;
    float speed;
};
std::vector<FallingObject> fallingObjects;

// Game states
enum GameState { START, PLAYING, GAME_OVER };
GameState gameState = START;

// Quote to display after death
std::string quote = "Fetching inspirational quote...";

// Timing for spawning objects
float spawnTimer = 0;
float spawnInterval = 1.0f;

// Function declarations
void SpawnFallingObject();
bool CheckCollisionPlayer(const Rectangle& obj);
void StartGame();
void UpdateGame(float dt);
void DrawGame();
void FetchQuote();

#if defined(PLATFORM_WEB)
void FetchQuoteSuccess(emscripten_fetch_t *fetch) {
    // Copy the fetched data (JSON string)
    std::string jsonStr(fetch->data, fetch->numBytes);

    // Basic JSON parsing to extract quote and author (very simple, no external libs)
    size_t contentPos = jsonStr.find("\"content\":\"");
    size_t authorPos = jsonStr.find("\"author\":\"");
    if(contentPos != std::string::npos && authorPos != std::string::npos) {
        contentPos += 10; // length of "\"content\":\""
        authorPos += 9;   // length of "\"author\":\""

        size_t contentEnd = jsonStr.find("\"", contentPos);
        size_t authorEnd = jsonStr.find("\"", authorPos);

        if(contentEnd != std::string::npos && authorEnd != std::string::npos) {
            std::string content = jsonStr.substr(contentPos, contentEnd - contentPos);
            std::string author = jsonStr.substr(authorPos, authorEnd - authorPos);
            quote = "\"" + content + "\"\n- " + author;
        }
    } else {
        quote = "Failed to parse quote.";
    }

    emscripten_fetch_close(fetch);
}

void FetchQuoteFail(emscripten_fetch_t *fetch) {
    quote = "Failed to fetch quote.";
    emscripten_fetch_close(fetch);
}

void FetchQuote() {
    const char* url = "https://api.quotable.io/random";

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess = FetchQuoteSuccess;
    attr.onerror = FetchQuoteFail;
    emscripten_fetch(&attr, url);
}
#else
// Desktop fallback: just use a fixed quote (no HTTP fetch)
void FetchQuote() {
    quote = "\"The only limit to our realization of tomorrow is our doubts of today.\"\n- Franklin D. Roosevelt";
}
#endif

void SpawnFallingObject() {
    float size = 30 + GetRandomValue(0, 20);
    float x = (float)GetRandomValue(0, screenWidth - (int)size);
    float speed = 150 + GetRandomValue(0, 150);
    fallingObjects.push_back({ Rectangle{ x, -size, size, size }, speed });
}

bool CheckCollisionPlayer(const Rectangle& obj) {
    return CheckCollisionRecs(player, obj);
}

void StartGame() {
    fallingObjects.clear();
    player.x = screenWidth / 2 - player.width / 2;
    spawnTimer = 0;
    quote = "";
    gameState = PLAYING;
}

void UpdateGame(float dt) {
    // Move player with arrow keys
    if (IsKeyDown(KEY_LEFT)) player.x -= playerSpeed * dt;
    if (IsKeyDown(KEY_RIGHT)) player.x += playerSpeed * dt;

    // Clamp player inside screen
    if (player.x < 0) player.x = 0;
    if (player.x + player.width > screenWidth) player.x = screenWidth - player.width;

    // Spawn falling objects over time
    spawnTimer += dt;
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0;
        SpawnFallingObject();
    }

    // Update falling objects position
    for (int i = 0; i < (int)fallingObjects.size(); i++) {
        fallingObjects[i].rect.y += fallingObjects[i].speed * dt;
    }

    // Remove objects that fell off screen
    fallingObjects.erase(std::remove_if(fallingObjects.begin(), fallingObjects.end(),
        [](const FallingObject& obj) { return obj.rect.y > screenHeight; }), fallingObjects.end());

    // Check collisions
    for (const auto& obj : fallingObjects) {
        if (CheckCollisionPlayer(obj.rect)) {
            gameState = GAME_OVER;
            FetchQuote();
            break;
        }
    }
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (gameState == START) {
        DrawText("Avoid the falling objects!\nUse LEFT and RIGHT arrows.\nPress SPACE to start.", 150, 250, 20, DARKGRAY);
    }
    else if (gameState == PLAYING) {
        DrawRectangleRec(player, BLUE);

        for (const auto& obj : fallingObjects) {
            DrawRectangleRec(obj.rect, RED);
        }

        DrawText("Score: ", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("%d", (int)GetTime()), 80, 10, 20, DARKGRAY);
    }
    else if (gameState == GAME_OVER) {
        DrawText("GAME OVER!", screenWidth / 2 - 80, 200, 40, RED);
        DrawText("Inspirational Quote:", 250, 260, 20, DARKGRAY);
        DrawText(quote.c_str(), 100, 300, 20, BLACK);
        DrawText("Press R to restart", 300, 550, 20, DARKGRAY);
    }

    EndDrawing();
}

int main() {
    InitWindow(screenWidth, screenHeight, "Avoid Falling Objects with Quotes");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (gameState == START) {
            if (IsKeyPressed(KEY_SPACE)) StartGame();
        }
        else if (gameState == PLAYING) {
            UpdateGame(dt);
        }
        else if (gameState == GAME_OVER) {
            if (IsKeyPressed(KEY_R)) {
                gameState = START;
                quote = "";
            }
        }

        DrawGame();
    }

    CloseWindow();
    return 0;
}
