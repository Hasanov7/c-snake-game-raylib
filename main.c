#include "raylib.h"

#define CELL_SIZE 24
#define GRID_COLS 40
#define GRID_ROWS 23
#define HUD_HEIGHT 96

#define SCREEN_WIDTH (GRID_COLS * CELL_SIZE)
#define SCREEN_HEIGHT (GRID_ROWS * CELL_SIZE + HUD_HEIGHT)

#define MAX_SNAKE_LENGTH (GRID_COLS * GRID_ROWS)

typedef struct Cell {
    int x;
    int y;
} Cell;

typedef enum SceneState {
    SCENE_TITLE,
    SCENE_PLAYING,
    SCENE_GAME_OVER
} SceneState;

typedef struct GameData {
    Cell snake[MAX_SNAKE_LENGTH];
    int snakeLength;
    int pendingGrowth;
    Cell direction;
    Cell nextDirection;

    Cell foodCell;

    int score;
    float moveTimer;
    float moveDelay;

    SceneState scene;
} GameData;

static int CellsEqual(Cell a, Cell b) {
    return (a.x == b.x && a.y == b.y);
}

static Cell AddCells(Cell a, Cell b) {
    Cell result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static int IsOppositeDirection(Cell a, Cell b) {
    return (a.x == -b.x && a.y == -b.y);
}

static int IsSnakeCell(const GameData *game, Cell c, int skipTail) {
    int limit = game->snakeLength;
    int i;

    if (skipTail && limit > 0) {
        limit -= 1;
    }

    for (i = 0; i < limit; i++) {
        if (CellsEqual(game->snake[i], c)) {
            return 1;
        }
    }
    return 0;
}

static Cell RandomFreeCell(const GameData *game) {
    Cell c;

    do {
        c.x = GetRandomValue(0, GRID_COLS - 1);
        c.y = GetRandomValue(0, GRID_ROWS - 1);
    } while (IsSnakeCell(game, c, 0));

    return c;
}

static void SpawnFood(GameData *game) {
    game->foodCell = RandomFreeCell(game);
}

static void ResetRound(GameData *game) {
    int midX = GRID_COLS / 2;
    int midY = GRID_ROWS / 2;

    game->snakeLength = 5;
    game->pendingGrowth = 0;
    game->snake[0] = (Cell){midX, midY};
    game->snake[1] = (Cell){midX - 1, midY};
    game->snake[2] = (Cell){midX - 2, midY};
    game->snake[3] = (Cell){midX - 3, midY};
    game->snake[4] = (Cell){midX - 4, midY};

    game->direction = (Cell){1, 0};
    game->nextDirection = (Cell){1, 0};

    game->score = 0;
    game->moveTimer = 0.0f;
    game->moveDelay = 0.15f;

    SpawnFood(game);
}

static void InitGame(GameData *game) {
    ResetRound(game);
    game->scene = SCENE_TITLE;
}

static void HandleDirectionInput(GameData *game) {
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        Cell wanted = (Cell){0, -1};
        if (!IsOppositeDirection(game->direction, wanted)) {
            game->nextDirection = wanted;
        }
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        Cell wanted = (Cell){0, 1};
        if (!IsOppositeDirection(game->direction, wanted)) {
            game->nextDirection = wanted;
        }
    }
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        Cell wanted = (Cell){-1, 0};
        if (!IsOppositeDirection(game->direction, wanted)) {
            game->nextDirection = wanted;
        }
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        Cell wanted = (Cell){1, 0};
        if (!IsOppositeDirection(game->direction, wanted)) {
            game->nextDirection = wanted;
        }
    }
}

static void ApplyFoodEffects(GameData *game) {
    game->pendingGrowth += 1;
    game->score += 10;
    SpawnFood(game);
}

static void StepSnake(GameData *game) {
    Cell newHead;
    int i;
    int tailWillMove;

    game->direction = game->nextDirection;
    newHead = AddCells(game->snake[0], game->direction);

    if (newHead.x < 0) {
        newHead.x = GRID_COLS - 1;
    } else if (newHead.x >= GRID_COLS) {
        newHead.x = 0;
    }
    if (newHead.y < 0) {
        newHead.y = GRID_ROWS - 1;
    } else if (newHead.y >= GRID_ROWS) {
        newHead.y = 0;
    }

    tailWillMove = (game->pendingGrowth == 0);
    if (IsSnakeCell(game, newHead, tailWillMove)) {
        game->scene = SCENE_GAME_OVER;
        return;
    }

    if (game->pendingGrowth > 0 && game->snakeLength < MAX_SNAKE_LENGTH) {
        game->snakeLength += 1;
        game->pendingGrowth -= 1;
    }

    for (i = game->snakeLength - 1; i > 0; i--) {
        game->snake[i] = game->snake[i - 1];
    }
    game->snake[0] = newHead;

    if (CellsEqual(newHead, game->foodCell)) {
        ApplyFoodEffects(game);
    }
}

static float ComputeMoveDelay(const GameData *game) {
    float base = 0.15f;
    float lengthBonus = (float)(game->snakeLength - 5) * 0.002f;

    if (lengthBonus > 0.06f) {
        lengthBonus = 0.06f;
    }

    base -= lengthBonus;

    if (base < 0.055f) {
        base = 0.055f;
    }
    return base;
}

static void UpdatePlaying(GameData *game, float dt) {
    HandleDirectionInput(game);

    game->moveDelay = ComputeMoveDelay(game);
    game->moveTimer += dt;

    while (game->moveTimer >= game->moveDelay && game->scene == SCENE_PLAYING) {
        game->moveTimer -= game->moveDelay;
        StepSnake(game);
    }
}

static void DrawTextCustom(Font font, const char *text, int x, int y, int size, Color color) {
    DrawTextEx(font, text, (Vector2){(float)x, (float)y}, (float)size, 1.0f, color);
}

static Rectangle CellToRect(Cell c) {
    Rectangle r;
    r.x = (float)(c.x * CELL_SIZE);
    r.y = (float)(HUD_HEIGHT + c.y * CELL_SIZE);
    r.width = (float)CELL_SIZE;
    r.height = (float)CELL_SIZE;
    return r;
}

static void DrawBoardBackground(void) {
    ClearBackground((Color){252, 252, 252, 255});
    DrawRectangle(0, 0, SCREEN_WIDTH, HUD_HEIGHT, (Color){245, 245, 245, 255});
}

static void DrawSnake(const GameData *game) {
    int i;

    for (i = game->snakeLength - 1; i >= 0; i--) {
        Rectangle r = CellToRect(game->snake[i]);
        float cx = r.x + r.width * 0.5f;
        float cy = r.y + r.height * 0.5f;
        float radius = (CELL_SIZE * 0.45f);
        Color bodyColor = (Color){103, 163, 255, 255};
        if (i == 0) {
            bodyColor = (Color){64, 124, 227, 255};
            radius = (CELL_SIZE * 0.48f);
        }
        DrawCircle((int)cx, (int)cy, radius, bodyColor);
    }
}

static void DrawFood(const GameData *game) {
    Rectangle r = CellToRect(game->foodCell);
    DrawCircle((int)(r.x + r.width * 0.5f), (int)(r.y + r.height * 0.5f),
               CELL_SIZE * 0.35f, (Color){239, 209, 109, 255});
}

static void DrawHud(const GameData *game, Font uiFont) {
    Color text = (Color){33, 119, 68, 255};
    DrawTextCustom(uiFont, "Nokia's Snake Game", 18, 14, 44, (Color){20, 95, 51, 255});
    DrawTextCustom(uiFont, TextFormat("Score: %d", game->score), 22, 56, 30, text);
    DrawTextCustom(uiFont, TextFormat("Length: %d", game->snakeLength), 220, 56, 30, text);
    DrawTextCustom(uiFont, "Food: +10 and +1 length", 425, 56, 30, text);
}

static void DrawTitleScreen(const GameData *game, Font uiFont) {
    DrawBoardBackground();
    DrawFood(game);
    DrawSnake(game);
    DrawHud(game, uiFont);

    DrawRectangle(155, 176, 650, 230, (Color){255, 255, 255, 245});
    DrawRectangleLines(155, 176, 650, 230, (Color){180, 180, 180, 255});
    DrawTextCustom(uiFont, "WASD / Arrow Keys to move", 240, 220, 42, (Color){20, 95, 51, 255});
    DrawTextCustom(uiFont, "Eat food to grow and score points", 265, 276, 31, (Color){33, 119, 68, 255});
    DrawTextCustom(uiFont, "Wrap edges, but don't hit yourself", 265, 315, 31, (Color){33, 119, 68, 255});
    DrawTextCustom(uiFont, "Press ENTER to start", 320, 360, 44, (Color){20, 95, 51, 255});
}

static void DrawGameOverOverlay(const GameData *game, Font uiFont) {
    DrawRectangle(210, 210, 540, 180, (Color){255, 255, 255, 248});
    DrawRectangleLines(210, 210, 540, 180, (Color){170, 170, 170, 255});
    DrawTextCustom(uiFont, "GAME OVER", 360, 242, 58, (Color){20, 95, 51, 255});
    DrawTextCustom(uiFont, TextFormat("Final score: %d", game->score), 340, 300, 38, (Color){33, 119, 68, 255});
    DrawTextCustom(uiFont, "Press ENTER to restart", 295, 342, 37, (Color){33, 119, 68, 255});
}

int main(void) {
    GameData game;
    Font uiFont;
    int loadedCustomFont = 0;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Nokia's Snake Game");
    SetTargetFPS(60);
    InitGame(&game);

    uiFont = GetFontDefault();
    if (FileExists("resources/VT323-Regular.ttf")) {
        uiFont = LoadFontEx("resources/VT323-Regular.ttf", 64, 0, 0);
        loadedCustomFont = 1;
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (game.scene == SCENE_TITLE) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetRound(&game);
                game.scene = SCENE_PLAYING;
            }
        } else if (game.scene == SCENE_PLAYING) {
            UpdatePlaying(&game, dt);
        } else if (game.scene == SCENE_GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetRound(&game);
                game.scene = SCENE_PLAYING;
            }
        }

        BeginDrawing();

        if (game.scene == SCENE_TITLE) {
            DrawTitleScreen(&game, uiFont);
        } else {
            DrawBoardBackground();
            DrawFood(&game);
            DrawSnake(&game);
            DrawHud(&game, uiFont);

            if (game.scene == SCENE_GAME_OVER) {
                DrawGameOverOverlay(&game, uiFont);
            }
        }

        EndDrawing();
    }

    if (loadedCustomFont) {
        UnloadFont(uiFont);
    }

    CloseWindow();
    return 0;
}
