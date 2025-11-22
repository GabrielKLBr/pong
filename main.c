#include "raylib.h"
#include "win_wrapper.h"
#include <stdbool.h>
#include <math.h>
#include "resource.h"

#define INITIAL_WIDTH 800
#define INITIAL_HEIGHT 600
#define PADDLE_WIDTH 15
#define PADDLE_HEIGHT 100
#define BALL_SIZE 25 
#define BALL_RADIUS (BALL_SIZE/2.0f)
#define BASE_BALL_SPEED 7.0f
#define MAX_BALL_SPEED (BASE_BALL_SPEED * 2.0f) // Limit set to 14.0f

float EaseInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
}

typedef struct {
    float x, y, width, height;
} Arena;

int main(void)
{
    SetRandomSeed(GetTime());
    InitWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "Pong");
    SetTargetFPS(60);

    WinHandle mainWinHandle = GetWindowHandle();
    Win32_ApplyEmbeddedIcon();
    Win32_UseDarkMode(true);
    int monitorW = GetMonitorWidth(0);
    int monitorH = GetMonitorHeight(0);
    
    // Animation and Arena Variables
    Vector2 initialPos = GetWindowPosition();
    Arena currentArena = { initialPos.x, initialPos.y, (float)INITIAL_WIDTH, (float)INITIAL_HEIGHT };
    Arena startArena = currentArena;
    Arena targetArena = { 0, 0, (float)monitorW, (float)monitorH };
    
    // Raylib Window Variables
    Vector2 windowStartPos = { 0, 0 };
    Vector2 windowTargetPos = { monitorW/2.0f - INITIAL_WIDTH/2.0f, monitorH/2.0f - INITIAL_HEIGHT/2.0f };

    // Secondary Windows (Handles)
    WinHandle hPaddle1 = Win32_CreateWindow(0, 0, PADDLE_WIDTH, PADDLE_HEIGHT, mainWinHandle);
    WinHandle hPaddle2 = Win32_CreateWindow(0, 0, PADDLE_WIDTH, PADDLE_HEIGHT, mainWinHandle);
    WinHandle hBall    = Win32_CreateWindow(0, 0, BALL_SIZE, BALL_SIZE, mainWinHandle);
    Win32_MakeRound(hBall, BALL_SIZE, BALL_SIZE);

    // Set the focus back to the main Raylib window
    // Corrected: Using Win32 function instead of the undefined Raylib flag
    Win32_SetForegroundWindow(mainWinHandle);

    // Game Entities
    Rectangle p1 = { 50, INITIAL_HEIGHT/2.0f - PADDLE_HEIGHT/2.0f, PADDLE_WIDTH, PADDLE_HEIGHT };
    Rectangle p2 = { INITIAL_WIDTH - 50 - PADDLE_WIDTH, INITIAL_HEIGHT/2.0f - PADDLE_HEIGHT/2.0f, PADDLE_WIDTH, PADDLE_HEIGHT };
    Vector2 ballPos = { INITIAL_WIDTH / 2.0f, INITIAL_HEIGHT / 2.0f };
    // Initializing with base speed
    Vector2 ballSpeed = { BASE_BALL_SPEED, BASE_BALL_SPEED }; 
    
    float p1LockedWorldY = 0;
    float p2LockedWorldY = 0;

    // Game/Animation States
    bool gameStarted = false;
    bool isExpanded = false; 
    bool isAnimating = false; 
    bool isLocked = false; 

    // Counters and Timers
    int score1 = 0, score2 = 0;
    int playerHits = 0; 
    int aiHitsTotal = 0; 
    float animTimer = 0.0f;
    float animDuration = 2.5f; 
    
    // AI Difficulty Variables
    float targetY = INITIAL_HEIGHT / 2.0f; 
    float reactionTimer = 0.0f;
    float reactionDelay = 0.2f; 

    while (!WindowShouldClose())
    {
        // 1. ANIMATION AND LOCKING LOGIC
        if (isAnimating) {
            animTimer += GetFrameTime();
            float t = animTimer / animDuration;
            
            if (t >= 1.0f) { 
                t = 1.0f; 
                isAnimating = false; 
                isExpanded = true;
                isLocked = true; 
                SetWindowPosition((int)windowTargetPos.x, (int)windowTargetPos.y); 
            }
            
            float val = EaseInOutCubic(t);

            // Interpolation for Arena Bounds Expansion
            currentArena.x = startArena.x + (targetArena.x - startArena.x) * val;
            currentArena.y = startArena.y + (targetArena.y - startArena.y) * val;
            currentArena.width = startArena.width + (targetArena.width - startArena.width) * val;
            currentArena.height = startArena.height + (targetArena.height - startArena.height) * val;
            
            // Interpolation for Raylib Window Position
            int newX = (int)(windowStartPos.x + (windowTargetPos.x - windowStartPos.x) * val);
            int newY = (int)(windowStartPos.y + (windowTargetPos.y - windowStartPos.y) * val);
            SetWindowPosition(newX, newY); 
            
            // Update Paddle Positions
            p1.y = p1LockedWorldY - currentArena.y;
            p2.y = p2LockedWorldY - currentArena.y;
            p2.x = currentArena.width - 50 - PADDLE_WIDTH;
            
        } 
        else if (isLocked) {
            // LOCKING TO THE CENTER
            SetWindowPosition((int)windowTargetPos.x, (int)windowTargetPos.y);
            currentArena = targetArena;
        }
        else if (!isExpanded) {
            // Drag Synchronization (Initial Phase)
            Vector2 winPos = GetWindowPosition();
            currentArena.x = winPos.x;
            currentArena.y = winPos.y;
        }


        // 2. INPUT (Player 1)
        float moveSpeed = 9.0f;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { 
            p1.y -= moveSpeed; gameStarted = true;
            if (isAnimating || isLocked) p1LockedWorldY -= moveSpeed;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { 
            p1.y += moveSpeed; gameStarted = true;
            if (isAnimating || isLocked) p1LockedWorldY += moveSpeed;
        }

        // 3. GAME AND AI LOGIC
        if (gameStarted) {
            
            // --- 3A. ADAPTIVE DIFFICULTY CALCULATION BY SCORE ---
            float aiDifficulty;
            float scoreDiff = (float)score1 - (float)score2;

            if (aiHitsTotal <= 2) {
                // 1. GUARANTEED INITIAL HIT
                aiDifficulty = 1.0f; 
            } 
            else if (scoreDiff <= -3.0f) {
                // 2. FACILITATE (Player 1 losing by 3 or more)
                aiDifficulty = 0.4f; 
            } 
            else if (scoreDiff >= 3.0f) {
                // 3. INVINCIBLE (Player 1 winning by 3 or more)
                aiDifficulty = 1.0f; 
            }
            else {
                // 4. NORMAL PROGRESSIVE (Score is close)
                float baseDiff = 0.7f;
                float scalingFactor = 0.03f;
                float hitsForCalc = (float)aiHitsTotal - 2.0f;
                
                aiDifficulty = baseDiff + sqrtf(hitsForCalc) * scalingFactor;
                if (aiDifficulty > 0.95f) aiDifficulty = 0.95f; 
            }
            
            // Flag to check if the AI should be perfect (max difficulty)
            bool isPerfect = (aiDifficulty == 1.0f);
            // -------------------------------------------------------------------
            
            // --- 3B. CONDITIONAL AI MOVEMENT LOGIC (P2) ---
            
            if (isPerfect) 
            {
                // ** PERFECT/INVINCIBLE LOGIC (Direct Movement) **
                // Tracks the ball perfectly, without delay, error, or damping
                float aiSpeedPerfect = 9.5f; 
                float centerP2 = p2.y + PADDLE_HEIGHT / 2.0f;

                if (ballPos.y > centerP2) {
                    p2.y += aiSpeedPerfect;
                    if (isAnimating || isLocked) p2LockedWorldY += aiSpeedPerfect;
                }
                else if (ballPos.y < centerP2) {
                    p2.y -= aiSpeedPerfect;
                    if (isAnimating || isLocked) p2LockedWorldY -= aiSpeedPerfect;
                }
                
            } else {
                
                // ** ADAPTIVE LOGIC (Error, Reaction, and Smoothing) **

                // 1. Reaction Time: Update the target
                reactionTimer += GetFrameTime();
                if (reactionTimer >= reactionDelay) {
                    reactionTimer = 0.0f;

                    float maxError = 100.0f * (1.0f - aiDifficulty); 
                    float errorOffset = (float)GetRandomValue(-(int)maxError/2, (int)maxError/2); 

                    targetY = ballPos.y + BALL_RADIUS + errorOffset;
                    
                    float minTarget = PADDLE_HEIGHT / 2.0f;
                    float maxTarget = currentArena.height - PADDLE_HEIGHT / 2.0f;
                    if (targetY < minTarget) targetY = minTarget;
                    if (targetY > maxTarget) targetY = maxTarget;
                }

                // 2. Proportional Smooth Movement (Damping)
                float aiSpeed = 5.0f + (4.0f * aiDifficulty); 
                float centerP2 = p2.y + PADDLE_HEIGHT / 2.0f;
                
                float diff = targetY - centerP2;
                float moveStep = 0.0f;
                
                float smoothingFactor = 0.15f; 
                moveStep = diff * smoothingFactor;

                if (fabsf(moveStep) > aiSpeed) {
                    moveStep = (moveStep > 0) ? aiSpeed : -aiSpeed;
                }

                if (fabsf(diff) > 0.01f) 
                {
                    p2.y += moveStep;
                    if (isAnimating || isLocked) {
                        p2LockedWorldY += moveStep;
                    }
                }
            }
            // -------------------------------------------------------------------

            // --- 3C. PHYSICS, COLLISION, AND SCORING ---
            
            ballPos.x += ballSpeed.x;
            ballPos.y += ballSpeed.y;

            if (ballPos.y <= 0 || ballPos.y + BALL_SIZE >= currentArena.height) ballSpeed.y *= -1;
            
            // SCORING (Ball goes left)
            if (ballPos.x < 0) { 
                score2++; 
                ballPos = (Vector2){currentArena.width/2, currentArena.height/2}; 
                gameStarted = false; 
                playerHits = 0; 
                
                // Reset Paddle Positions and Speed
                float paddleY = currentArena.height/2.0f - PADDLE_HEIGHT/2.0f;
                p1.y = paddleY;
                p2.y = paddleY;
                p1LockedWorldY = currentArena.y + p1.y;
                p2LockedWorldY = currentArena.y + p2.y;
                targetY = currentArena.height/2.0f;
                reactionTimer = 0.0f;
                ballSpeed = (Vector2){ BASE_BALL_SPEED, BASE_BALL_SPEED }; // Reset speed
            }
            
            // SCORING (Ball goes right)
            if (ballPos.x > currentArena.width) { 
                score1++; 
                ballPos = (Vector2){currentArena.width/2, currentArena.height/2}; 
                gameStarted = false; 
                playerHits = 0; 
                
                // Reset Paddle Positions and Speed
                float paddleY = currentArena.height/2.0f - PADDLE_HEIGHT/2.0f;
                p1.y = paddleY;
                p2.y = paddleY;
                p1LockedWorldY = currentArena.y + p1.y;
                p2LockedWorldY = currentArena.y + p2.y;
                targetY = currentArena.height/2.0f;
                reactionTimer = 0.0f;
                ballSpeed = (Vector2){ BASE_BALL_SPEED, BASE_BALL_SPEED }; // Reset speed
            }

            // Player 1 Collision
            bool hitP1 = (ballPos.x < p1.x + p1.width && ballPos.x + BALL_SIZE > p1.x && ballPos.y < p1.y + p1.height && ballPos.y + BALL_SIZE > p1.y);
            if (hitP1) { 
                ballSpeed.x *= -1; 
                ballPos.x = p1.x + p1.width + 1; 
                
                // Increase speed
                ballSpeed.x += (ballSpeed.x > 0) ? 1.0f : -1.0f;
                
                // NEW: Limit speed to the maximum allowed
                if (fabsf(ballSpeed.x) > MAX_BALL_SPEED) {
                    ballSpeed.x = (ballSpeed.x > 0) ? MAX_BALL_SPEED : -MAX_BALL_SPEED;
                }
                
                if (!isExpanded && !isAnimating) {
                    playerHits++;
                    if (playerHits >= 2) {
                        isAnimating = true;
                        Vector2 winPos = GetWindowPosition();
                        startArena = (Arena){ winPos.x, winPos.y, (float)INITIAL_WIDTH, (float)INITIAL_HEIGHT };
                        windowStartPos = winPos;
                        p1LockedWorldY = startArena.y + p1.y;
                        p2LockedWorldY = startArena.y + p2.y;

                        Win32_SetVisibleMode(hPaddle1, true);
                        Win32_SetVisibleMode(hPaddle2, true);
                        Win32_SetVisibleMode(hBall, true);
                        Win32_SetTopMost(hPaddle1, true);
                        Win32_SetTopMost(hPaddle2, true);
                        Win32_SetTopMost(hBall, true);
                    }
                }
            }
            
            // Player 2 (AI) Collision
            bool hitP2 = (ballPos.x < p2.x + p2.width && ballPos.x + BALL_SIZE > p2.x && ballPos.y < p2.y + p2.height && ballPos.y + BALL_SIZE > p2.y);
            if (hitP2) { 
                ballSpeed.x *= -1; 
                ballPos.x = p2.x - BALL_SIZE - 1;
                aiHitsTotal++; 
            }
        }

        // 4. Clamping
        if (p1.y < 0) { p1.y = 0; if(isAnimating || isLocked) p1LockedWorldY = currentArena.y; }
        if (p1.y + PADDLE_HEIGHT > currentArena.height) { p1.y = currentArena.height - PADDLE_HEIGHT; if(isAnimating || isLocked) p1LockedWorldY = currentArena.y + p1.y; }
        if (isExpanded && !isAnimating) p2.x = currentArena.width - 50 - PADDLE_WIDTH;
        if (p2.y < 0) { p2.y = 0; if(isAnimating || isLocked) p2LockedWorldY = currentArena.y; }
        if (p2.y + PADDLE_HEIGHT > currentArena.height) { p2.y = currentArena.height - PADDLE_HEIGHT; if(isAnimating || isLocked) p2LockedWorldY = currentArena.y + p2.y; }


        // 5. UPDATE SECONDARY WINDOWS
        int globalOffsetX = (int)currentArena.x;
        int globalOffsetY = (int)currentArena.y;
        
        int titleBarOffset = 0;
        if (!isExpanded) titleBarOffset = 35; 
        
        if (isAnimating) {
            float t = animTimer / animDuration;
            float val = EaseInOutCubic(t);
            titleBarOffset = (int)(35.0f * (1.0f - val));
        }

        Win32_SetWindowPos(hPaddle1, globalOffsetX + (int)p1.x, globalOffsetY + (int)p1.y + titleBarOffset, PADDLE_WIDTH, PADDLE_HEIGHT);
        Win32_SetWindowPos(hPaddle2, globalOffsetX + (int)p2.x, globalOffsetY + (int)p2.y + titleBarOffset, PADDLE_WIDTH, PADDLE_HEIGHT);
        Win32_SetWindowPos(hBall, globalOffsetX + (int)ballPos.x - 12, globalOffsetY + (int)ballPos.y, BALL_SIZE, BALL_SIZE);
        
        Win32_ProcessMessages();

        // 6. RAYLIB DRAWING
        BeginDrawing();
            ClearBackground(BLACK);
            
            // Scoreboard 
            DrawText(TextFormat("%d", score1), INITIAL_WIDTH/4, 50, 60, WHITE);
            DrawText(TextFormat("%d", score2), 3*INITIAL_WIDTH/4, 50, 60, WHITE);

            DrawLine(INITIAL_WIDTH/2, 0, INITIAL_WIDTH/2, INITIAL_HEIGHT, DARKGRAY);
            
            // Raylib Drawing only during windowed phase (Fake)
            if (!isExpanded && !isAnimating) {
                DrawRectangleRec(p1, WHITE);
                DrawRectangleRec(p2, WHITE);
                DrawCircle(ballPos.x + BALL_RADIUS - 12, ballPos.y + BALL_RADIUS, BALL_RADIUS, WHITE);
            }
            
        EndDrawing();
    }

    Win32_DestroyWindow(hPaddle1);
    Win32_DestroyWindow(hPaddle2);
    Win32_DestroyWindow(hBall);
    CloseWindow();

    return 0;
}