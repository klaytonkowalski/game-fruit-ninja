//////////////////////////////////////////////////////////////////////
// LICENSE
//////////////////////////////////////////////////////////////////////

// MIT License

// Copyright (c) 2021 Klayton Kowalski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// https://github.com/klaytonkowalski/game-fruit-ninja

//////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////

#include "raylib.h"

#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// DEFINES
//////////////////////////////////////////////////////////////////////

#define MAX_FRUIT_COUNT 48
#define MAX_PARTICLE_COUNT 16

//////////////////////////////////////////////////////////////////////
// ENUMERATIONS
//////////////////////////////////////////////////////////////////////

typedef enum GameState
{
    startState,
    playState,
    loseState
}
GameState;

typedef enum FruitType
{
    appleType,
    bananaType,
    cherryType,
    donutType
}
FruitType;

//////////////////////////////////////////////////////////////////////
// STRUCTURES
//////////////////////////////////////////////////////////////////////

typedef struct Fruit
{
    FruitType type;
    Vector2 position;
    Vector2 velocity;
    bool enabled;
}
Fruit;

typedef struct Particle
{
    Vector2 position;
    float elapsed;
    bool enabled;
}
Particle;

//////////////////////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////////////////////

static const int screenWidth = 960;
static const int screenHalfWidth = screenWidth * 0.5;
static const int screenHeight = 540;
static const int targetFPS = 60;
static const int appleScore = 1;
static const int bananaScore = appleScore * 3;
static const int cherryScore = bananaScore * 3;
static const int largeTextSize = 40;
static const int normalTextSize = largeTextSize * 0.5;
static const int fruitRadius = 32;
static const int appleSpawnCeiling = 50;
static const int bananaSpawnCeiling = 75;
static const int cherrySpawnCeiling = 85;
static const int donutSpawnCeiling = 100;
static const int mouseRadius = 8;
static const float minimumFruitThrust = 5;
static const float maximumFruitThrust = 20;
static const float minimumFruitStrafe = -5;
static const float maximumFruitStrafe = 5;
static const float minimumSpawnRate = 1;
static const float maximumSpawnRate = 0.1;
static const float maximumElapsed = 30;
static const float gravity = -10.0 / targetFPS;
static const float particleMaximumElapsed = 0.1;

//////////////////////////////////////////////////////////////////////
// LOADED PROPERTIES
//////////////////////////////////////////////////////////////////////

static Texture2D backgroundTexture;
static Texture2D appleTexture;
static Texture2D bananaTexture;
static Texture2D cherryTexture;
static Texture2D donutTexture;
static Music music;
static Sound fruitSpawnSound;
static Sound fruitSlashSound;
static Sound donutSlashSound;

//////////////////////////////////////////////////////////////////////
// PROPERTIES
//////////////////////////////////////////////////////////////////////

static GameState state;
static Fruit fruits[MAX_FRUIT_COUNT];
static Particle particles[MAX_PARTICLE_COUNT];
static int nextFruitIndex;
static int nextParticleIndex;
static int score;
static int fruitsSlashed;
static float spawnElapsed;
static float totalElapsed;
static bool slashing;

//////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////

static void Initialize();
static void Update();
static void Draw();
static void Terminate();
static void UpdateStartState();
static void UpdatePlayState();
static void UpdateLoseState();
static void DrawStartState();
static void DrawPlayState();
static void DrawLoseState();
static void FromStartToPlayState();
static void FromPlayToLoseState();
static void FromLoseToStartState();
static void SpawnFruit();
static void SlashFruit(Fruit *fruit);

//////////////////////////////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    Initialize();
    while (!WindowShouldClose())
    {
        Update();
        Draw();
    }
    Terminate();
    return 0;
}

static void Initialize()
{
    InitWindow(screenWidth, screenHeight, "Fruit Ninja");
    SetTargetFPS(targetFPS);
    backgroundTexture = LoadTexture("Background.png");
    appleTexture = LoadTexture("Apple.png");
    bananaTexture = LoadTexture("Banana.png");
    cherryTexture = LoadTexture("Cherry.png");
    donutTexture = LoadTexture("Donut.png");
    InitAudioDevice();
    music = LoadMusicStream("Music.wav");
    PlayMusicStream(music);
    fruitSlashSound = LoadSound("FruitSlash.wav");
    fruitSpawnSound = LoadSound("FruitSpawn.wav");
    donutSlashSound = LoadSound("DonutSlash.wav");
    state = startState;
    for (int i = 0; i < MAX_FRUIT_COUNT; ++i)
    {
        fruits[i].enabled = false;
    }
    for (int i = 0; i < MAX_PARTICLE_COUNT; ++i)
    {
        particles[i].enabled = false;
    }
    nextFruitIndex = 0;
    nextParticleIndex = 0;
    fruitsSlashed = 0;
    spawnElapsed = 0;
    totalElapsed = 0;
    slashing = false;
    HideCursor();
}

static void Update()
{
    UpdateMusicStream(music);
    if (IsKeyPressed(KEY_M))
    {
        IsMusicPlaying(music) ? PauseMusicStream(music) : ResumeMusicStream(music);
    }
    if (state == startState)
    {
        UpdateStartState();
    }
    else if (state == playState)
    {
        UpdatePlayState();
    }
    else if (state == loseState)
    {
        UpdateLoseState();
    }
}

static void Draw()
{
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(backgroundTexture, 0, 0, WHITE);
    const Vector2 mousePosition = GetMousePosition();
    DrawCircle(mousePosition.x, mousePosition.y, mouseRadius, slashing ? GREEN : WHITE);
    if (state == startState)
    {
        DrawStartState();
    }
    else if (state == playState)
    {
        DrawPlayState();
    }
    else if (state == loseState)
    {
        DrawLoseState();
    }
    EndDrawing();
}

static void Terminate()
{
    UnloadTexture(backgroundTexture);
    UnloadTexture(appleTexture);
    UnloadTexture(bananaTexture);
    UnloadTexture(cherryTexture);
    UnloadTexture(donutTexture);
    UnloadMusicStream(music);
    UnloadSound(fruitSlashSound);
    UnloadSound(fruitSpawnSound);
    UnloadSound(donutSlashSound);
    CloseAudioDevice();
    CloseWindow();
}

static void UpdateStartState()
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        FromStartToPlayState();
    }
}

static void UpdatePlayState()
{
    totalElapsed += GetFrameTime();
    spawnElapsed += GetFrameTime();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        slashing = true;
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        slashing = false;
    }
    if (slashing)
    {
        particles[nextParticleIndex].position = GetMousePosition();
        particles[nextParticleIndex].elapsed = 0;
        particles[nextParticleIndex].enabled = true;
        nextParticleIndex = (nextParticleIndex + 1) % MAX_PARTICLE_COUNT;
    }
    for (int i = 0; i < MAX_PARTICLE_COUNT; ++i)
    {
        if (particles[i].enabled)
        {
            particles[i].elapsed += GetFrameTime();
            if (particles[i].elapsed > particleMaximumElapsed)
            {
                particles[i].enabled = false;
            }
        }
    }
    const float spawnElapsedThreshold = minimumSpawnRate - totalElapsed / maximumElapsed;
    if (spawnElapsed > (spawnElapsedThreshold < maximumSpawnRate ? maximumSpawnRate : spawnElapsedThreshold))
    {
        spawnElapsed = 0;
        SpawnFruit();
    }
    for (int i = 0; i < MAX_FRUIT_COUNT; ++i)
    {
        if (fruits[i].enabled)
        {
            if (fruits[i].position.y > screenHeight)
            {
                fruits[i].enabled = false;
            }
            else if (slashing && CheckCollisionPointCircle(GetMousePosition(), (Vector2) { fruits[i].position.x + fruitRadius, fruits[i].position.y + fruitRadius }, fruitRadius))
            {
                SlashFruit(&fruits[i]);
            }
            else
            {
                fruits[i].position.x += fruits[i].velocity.x;
                fruits[i].position.y += fruits[i].velocity.y;
                fruits[i].velocity.y -= gravity;
            }
        }
    }
}

static void UpdateLoseState()
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        FromLoseToStartState();
    }
}

static void DrawStartState()
{
    DrawText("Fruit Ninja", screenHalfWidth - MeasureText("Fruit Ninja", largeTextSize) * 0.5, screenHeight * 0.4 - largeTextSize * 0.5, largeTextSize, WHITE);
    DrawText("Press SLASH To Play!", screenHalfWidth - MeasureText("Press SLASH To Play!", normalTextSize) * 0.5, screenHeight * 0.6 - largeTextSize * 0.5, normalTextSize, WHITE);
}

static void DrawPlayState()
{
    for (int i = 0; i < MAX_PARTICLE_COUNT; ++i)
    {
        if (particles[i].enabled)
        {
            DrawCircle(particles[i].position.x, particles[i].position.y, mouseRadius, GREEN);
        }
    }
    for (int i = 0; i < MAX_FRUIT_COUNT; ++i)
    {
        if (fruits[i].enabled)
        {
            if (fruits[i].type == appleType)
            {
                DrawTextureV(appleTexture, fruits[i].position, WHITE);
            }
            else if (fruits[i].type == bananaType)
            {
                DrawTextureV(bananaTexture, fruits[i].position, WHITE);
            }
            else if (fruits[i].type == cherryType)
            {
                DrawTextureV(cherryTexture, fruits[i].position, WHITE);
            }
            else if (fruits[i].type == donutType)
            {
                DrawTextureV(donutTexture, fruits[i].position, WHITE);
            }
        }
    }
}

static void DrawLoseState()
{
    DrawText("You Slashed A Donut!", screenHalfWidth - MeasureText("You Slashed A Donut!", largeTextSize) * 0.5, screenHeight * 0.4 - largeTextSize * 0.5, largeTextSize, WHITE);
    char slashedBuffer[32];
    sprintf(slashedBuffer, "Fruits Slashed: %d", fruitsSlashed);
    DrawText(slashedBuffer, screenHalfWidth - MeasureText(slashedBuffer, normalTextSize) * 0.5, screenHeight * 0.6 - largeTextSize * 0.5, normalTextSize, WHITE);
    char scoreBuffer[32];
    sprintf(scoreBuffer, "Score: %d", score);
    DrawText(scoreBuffer, screenHalfWidth - MeasureText(scoreBuffer, normalTextSize) * 0.5, screenHeight * 0.6 - normalTextSize * 1.5 - largeTextSize * 0.5, normalTextSize, WHITE);
}

static void FromStartToPlayState()
{
    state = playState;
}

static void FromPlayToLoseState()
{
    state = loseState;
    for (int i = 0; i < MAX_FRUIT_COUNT; ++i)
    {
        fruits[i].enabled = false;
    }
    for (int i = 0; i < MAX_PARTICLE_COUNT; ++i)
    {
        particles[i].enabled = false;
    }
    spawnElapsed = 0;
    totalElapsed = 0;
    slashing = false;
}

static void FromLoseToStartState()
{
    state = startState;
    fruitsSlashed = 0;
    score = 0;
}

static void SpawnFruit()
{
    PlaySound(fruitSpawnSound);
    const int spawnValue = GetRandomValue(1, 100);
    if (spawnValue <= appleSpawnCeiling)
    {
        fruits[nextFruitIndex].type = appleType;
    }
    else if (spawnValue <= bananaSpawnCeiling)
    {
        fruits[nextFruitIndex].type = bananaType;
    }
    else if (spawnValue <= cherrySpawnCeiling)
    {
        fruits[nextFruitIndex].type = cherryType;
    }
    else if (spawnValue <= donutSpawnCeiling)
    {
        fruits[nextFruitIndex].type = donutType;
    }
    fruits[nextFruitIndex].position = (Vector2) { GetRandomValue(screenWidth * 0.25, screenWidth * 0.75), screenHeight };
    fruits[nextFruitIndex].velocity = (Vector2) { GetRandomValue(minimumFruitStrafe, maximumFruitStrafe), -GetRandomValue(minimumFruitThrust, maximumFruitThrust) };
    fruits[nextFruitIndex].enabled = true;
    nextFruitIndex = (nextFruitIndex + 1) % MAX_FRUIT_COUNT;
}

static void SlashFruit(Fruit *fruit)
{
    fruit->enabled = false;
    if (fruit->type == appleType)
    {
        ++fruitsSlashed;
        score += appleScore;
        PlaySound(fruitSlashSound);
    }
    else if (fruit->type == bananaType)
    {
        ++fruitsSlashed;
        score += bananaScore;
        PlaySound(fruitSlashSound);
    }
    else if (fruit->type == cherryType)
    {
        ++fruitsSlashed;
        score += cherryScore;
        PlaySound(fruitSlashSound);
    }
    else if (fruit->type == donutType)
    {
        PlaySound(donutSlashSound);
        FromPlayToLoseState();
    }
}