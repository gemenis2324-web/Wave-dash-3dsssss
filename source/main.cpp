#include <citro2d.h>
#include <citro3d.h>
#include <ctru.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <deque>

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

enum GameState {
    STATE_MENU,
    STATE_LEVEL_SELECT,
    STATE_CUSTOMIZE,
    STATE_PLAYING,
    STATE_INFINITE,
    STATE_GAMEOVER,
    STATE_VICTORY
};

struct TrailPoint {
    float x, y;
};

struct Obstacle {
    float x, y, w, h;
};

struct WavePlayer {
    float x, y;
    float speed;
    bool goingUp;
    int shapeID;      // 0: Normal, 1: Flecha, 2: Mini
    int waveColorIdx;
    int trailColorIdx;
    std::deque<TrailPoint> trail;
};

// Paleta de colores disponible
const u32 PALETTE[] = {
    C2D_Color32(0, 255, 255, 255),   // Cian
    C2D_Color32(0, 255, 100, 255),   // Verde Neon
    C2D_Color32(255, 50, 150, 255),  // Rosa
    C2D_Color32(255, 220, 0, 255),   // Amarillo
    C2D_Color32(180, 50, 255, 255),  // Morado
    C2D_Color32(255, 255, 255, 255)  // Blanco
};
const char* COLOR_NAMES[] = {"Cian", "Verde", "Rosa", "Amarillo", "Morado", "Blanco"};
const char* SHAPE_NAMES[] = {"Clasico", "Flecha", "Mini Wave"};
const int NUM_COLORS = 6;
const int NUM_SHAPES = 3;

// Generador determinista de los 10 niveles (misma semilla por nivel para poder memorizarlos)
float buildLevel(int level, std::vector<Obstacle>& obstacles) {
    obstacles.clear();
    srand(level * 1337); // Semilla fija por nivel

    int numSections = 15 + (level * 5); // Nivel 1: 20 secciones, Nivel 10: 65 secciones
    float cursorX = 350.0f;
    float gapSize = 115.0f - (level * 5.5f); // Cada vez más estrecho (de 109px a 60px)
    if (gapSize < 52.0f) gapSize = 52.0f;

    float lastGapY = (SCREEN_HEIGHT - gapSize) / 2.0f;

    for (int i = 0; i < numSections; i++) {
        // Variación de altura del pasillo
        float maxStep = 25.0f + (level * 4.0f);
        float delta = ((rand() % 200) / 100.0f - 1.0f) * maxStep;
        float gapY = lastGapY + delta;

        if (gapY < 15.0f) gapY = 15.0f;
        if (gapY > SCREEN_HEIGHT - gapSize - 15.0f) gapY = SCREEN_HEIGHT - gapSize - 15.0f;
        lastGapY = gapY;

        float blockWidth = 45.0f + (rand() % 40);

        // Bloque superior
        if (gapY > 2.0f) {
            obstacles.push_back({cursorX, 0.0f, blockWidth, gapY});
        }
        // Bloque inferior
        float bottomY = gapY + gapSize;
        if (bottomY < SCREEN_HEIGHT - 2.0f) {
            obstacles.push_back({cursorX, bottomY, blockWidth, SCREEN_HEIGHT - bottomY});
        }

        // En niveles altos (5+), añadir bloques flotantes centrales ocasionales
        if (level >= 5 && i % 4 == 2 && gapSize > 65.0f) {
            obstacles.push_back({cursorX + blockWidth + 35.0f, gapY + (gapSize / 2.0f) - 10.0f, 20.0f, 20.0f});
            cursorX += 60.0f;
        }

        float spacing = 95.0f - (level * 3.0f);
        if (spacing < 55.0f) spacing = 55.0f;
        cursorX += blockWidth + spacing;
    }

    return cursorX + 100.0f; // Distancia total del nivel
}

bool checkCollision(float px, float py, float pr, const Obstacle& obs) {
    // Hitbox cuadrada ajustada al centro del Wave (justa como en GD)
    float left = px - pr;
    float right = px + pr;
    float top = py - pr;
    float bottom = py + pr;

    return (right > obs.x && left < obs.x + obs.w &&
            bottom > obs.y && top < obs.y + obs.h);
}

void drawWave(const WavePlayer& wave) {
    u32 wClr = PALETTE[wave.waveColorIdx];
    u32 tClr = PALETTE[wave.trailColorIdx];

    // 1. Dibujar Estela (Trail) conectando puntos
    if (wave.trail.size() > 1) {
        float thickness = (wave.shapeID == 2) ? 3.0f : 5.0f;
        for (size_t i = 0; i < wave.trail.size() - 1; i++) {
            float x1 = wave.trail[i].x;
            float y1 = wave.trail[i].y;
            float x2 = wave.trail[i + 1].x;
            float y2 = wave.trail[i + 1].y;
            C2D_DrawLine(x1, y1, tClr, x2, y2, tClr, thickness, 0.4f);
        }
    }

    // 2. Dibujar Nave Wave (Triángulo orientado según dirección)
    float size = (wave.shapeID == 2) ? 6.0f : 9.0f;
    float angle = wave.goingUp ? -0.785f : 0.785f; // +-45 grados

    float x1 = wave.x + cosf(angle) * (size * 1.4f);
    float y1 = wave.y + sinf(angle) * (size * 1.4f);

    float x2 = wave.x + cosf(angle + 2.3f) * size;
    float y2 = wave.y + sinf(angle + 2.3f) * size;

    float x3 = wave.x + cosf(angle - 2.3f) * size;
    float y3 = wave.y + sinf(angle - 2.3f) * size;

    C2D_DrawTriangle(x1, y1, wClr, x2, y2, wClr, x3, y3, wClr, 0.6f);

    // Detalle interior si es forma "Flecha"
    if (wave.shapeID == 1) {
        u32 coreClr = C2D_Color32(20, 20, 35, 255);
        C2D_DrawCircleSolid(wave.x, wave.y, 0.7f, 2.5f, coreClr);
    }
}

int main(int argc, char* argv[]) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* topScreen = C2D_CreateTarget(SCREEN_WIDTH, SCREEN_HEIGHT, GFX_TOP, GFX_LEFT);

    GameState state = STATE_MENU;
    GameState lastMode = STATE_PLAYING;

    WavePlayer wave = {70.0f, 120.0f, 3.5f, false, 0, 0, 0, {}};
    int currentLevel = 1;
    int customCursor = 0; // 0: Forma, 1: Color Wave, 2: Color Estela

    std::vector<Obstacle> obstacles;
    float levelLength = 1000.0f;
    float distanceTraveled = 0.0f;
    int infiniteScore = 0;
    int infiniteBest = 0;
    float infiniteNextSpawn = 300.0f;
    float infiniteLastGapY = 80.0f;

    bool needsConsoleRefresh = true;

    auto resetRun = [&](bool isInfinite) {
        wave.x = 70.0f;
        wave.y = SCREEN_HEIGHT / 2.0f;
        wave.goingUp = false;
        wave.trail.clear();
        distanceTraveled = 0.0f;

        if (!isInfinite) {
            wave.speed = 2.8f + (currentLevel * 0.28f);
            levelLength = buildLevel(currentLevel, obstacles);
            lastMode = STATE_PLAYING;
            state = STATE_PLAYING;
        } else {
            wave.speed = 3.2f;
            obstacles.clear();
            infiniteScore = 0;
            infiniteNextSpawn = 320.0f;
            infiniteLastGapY = 80.0f;
            lastMode = STATE_INFINITE;
            state = STATE_INFINITE;
        }
        needsConsoleRefresh = true;
    };

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break;

        // --- LÓGICA POR ESTADO ---
        if (state == STATE_MENU) {
            if (kDown & KEY_A) { state = STATE_LEVEL_SELECT; needsConsoleRefresh = true; }
            if (kDown & KEY_Y) { resetRun(true); }
            if (kDown & KEY_X) { state = STATE_CUSTOMIZE; needsConsoleRefresh = true; }
        }
        else if (state == STATE_LEVEL_SELECT) {
            if (kDown & KEY_DOWN) { currentLevel = (currentLevel % 10) + 1; needsConsoleRefresh = true; }
            if (kDown & KEY_UP)   { currentLevel = (currentLevel == 1) ? 10 : currentLevel - 1; needsConsoleRefresh = true; }
            if (kDown & KEY_A)    { resetRun(false); }
            if (kDown & KEY_B)    { state = STATE_MENU; needsConsoleRefresh = true; }
        }
        else if (state == STATE_CUSTOMIZE) {
            if (kDown & KEY_DOWN) { customCursor = (customCursor + 1) % 3; needsConsoleRefresh = true; }
            if (kDown & KEY_UP)   { customCursor = (customCursor + 2) % 3; needsConsoleRefresh = true; }

            if (kDown & KEY_RIGHT) {
                if (customCursor == 0) wave.shapeID = (wave.shapeID + 1) % NUM_SHAPES;
                if (customCursor == 1) wave.waveColorIdx = (wave.waveColorIdx + 1) % NUM_COLORS;
                if (customCursor == 2) wave.trailColorIdx = (wave.trailColorIdx + 1) % NUM_COLORS;
                needsConsoleRefresh = true;
            }
            if (kDown & KEY_LEFT) {
                if (customCursor == 0) wave.shapeID = (wave.shapeID - 1 + NUM_SHAPES) % NUM_SHAPES;
                if (customCursor == 1) wave.waveColorIdx = (wave.waveColorIdx - 1 + NUM_COLORS) % NUM_COLORS;
                if (customCursor == 2) wave.trailColorIdx = (wave.trailColorIdx - 1 + NUM_COLORS) % NUM_COLORS;
                needsConsoleRefresh = true;
            }
            if (kDown & KEY_B) { state = STATE_MENU; needsConsoleRefresh = true; }
        }
        else if (state == STATE_PLAYING || state == STATE_INFINITE) {
            if (kDown & KEY_B) { state = STATE_MENU; needsConsoleRefresh = true; }

            // Física del Wave (45 grados exactos: dy == dx)
            wave.goingUp = (kHeld & (KEY_A | KEY_R | KEY_TOUCH));
            float verticalSpeed = (wave.shapeID == 2) ? wave.speed * 1.25f : wave.speed; // Mini Wave es más agudo

            if (wave.goingUp) {
                wave.y -= verticalSpeed;
            } else {
                wave.y += verticalSpeed;
            }

            // Actualizar estela
            for (auto& pt : wave.trail) {
                pt.x -= wave.speed;
            }
            wave.trail.push_back({wave.x, wave.y});
            if (wave.trail.size() > 65) {
                wave.trail.pop_front();
            }

            distanceTraveled += wave.speed;

            // Colisión con techo o suelo
            float hitRadius = (wave.shapeID == 2) ? 3.5f : 5.0f;
            if (wave.y - hitRadius <= 4.0f || wave.y + hitRadius >= SCREEN_HEIGHT - 4.0f) {
                state = STATE_GAMEOVER;
                needsConsoleRefresh = true;
            }

            // Mover obstáculos y comprobar colisiones
            for (auto& obs : obstacles) {
                obs.x -= wave.speed;
                if (checkCollision(wave.x, wave.y, hitRadius, obs)) {
                    state = STATE_GAMEOVER;
                    needsConsoleRefresh = true;
                }
            }

            // Limpiar obstáculos que ya salieron de pantalla
            while (!obstacles.empty() && obstacles.front().x + obstacles.front().w < -20.0f) {
                obstacles.erase(obstacles.begin());
            }

            // Progreso de Niveles 1-10
            if (state == STATE_PLAYING) {
                if (distanceTraveled >= levelLength) {
                    state = STATE_VICTORY;
                    needsConsoleRefresh = true;
                }
            }
            // Generación procedural del Modo Infinito
            else if (state == STATE_INFINITE) {
                infiniteScore = (int)(distanceTraveled / 20.0f);
                if (infiniteScore > infiniteBest) infiniteBest = infiniteScore;

                // Aumentar velocidad gradualmente
                wave.speed = 3.2f + (infiniteScore * 0.002f);
                if (wave.speed > 6.5f) wave.speed = 6.5f;

                infiniteNextSpawn -= wave.speed;
                if (infiniteNextSpawn <= SCREEN_WIDTH) {
                    float gap = 95.0f - (infiniteScore * 0.05f);
                    if (gap < 54.0f) gap = 54.0f;

                    float delta = ((rand() % 200) / 100.0f - 1.0f) * 45.0f;
                    float gapY = infiniteLastGapY + delta;
                    if (gapY < 15.0f) gapY = 15.0f;
                    if (gapY > SCREEN_HEIGHT - gap - 15.0f) gapY = SCREEN_HEIGHT - gap - 15.0f;
                    infiniteLastGapY = gapY;

                    float w = 45.0f + (rand() % 30);
                    obstacles.push_back({(float)SCREEN_WIDTH, 0.0f, w, gapY});
                    obstacles.push_back({(float)SCREEN_WIDTH, gapY + gap, w, SCREEN_HEIGHT - (gapY + gap)});

                    infiniteNextSpawn = SCREEN_WIDTH + w + 75.0f;
                }
            }
        }
        else if (state == STATE_GAMEOVER || state == STATE_VICTORY) {
            if (kDown & KEY_A) {
                if (state == STATE_VICTORY && lastMode == STATE_PLAYING && currentLevel < 10) {
                    currentLevel++;
                }
                resetRun(lastMode == STATE_INFINITE);
            }
            if (kDown & KEY_B) {
                state = STATE_MENU;
                needsConsoleRefresh = true;
            }
        }

        // --- CONSOLA INFERIOR (HUD e Interfaz limpia sin parpadeos) ---
        if (needsConsoleRefresh || state == STATE_PLAYING || state == STATE_INFINITE) {
            if (needsConsoleRefresh) {
                consoleClear();
                needsConsoleRefresh = false;
            }

            if (state == STATE_MENU) {
                printf("\x1b[3;8H==============================");
                printf("\x1b[4;10H     WAVE DASH 3DS v1.0");
                printf("\x1b[5;8H==============================");
                printf("\x1b[8;8H[A] Seleccionar Nivel (1 - 10)");
                printf("\x1b[10;8H[Y] Modo Infinito (Record: %d)", infiniteBest);
                printf("\x1b[12;8H[X] Personalizar Wave y Estela");
                printf("\x1b[16;8H[START] Salir al Homebrew");
            }
            else if (state == STATE_LEVEL_SELECT) {
                printf("\x1b[3;8H=== SELECCION DE NIVEL ===");
                printf("\x1b[6;8HUsa ARRIBA / ABAJO para elegir:");
                printf("\x1b[9;10H>> NIVEL %d / 10 <<", currentLevel);
                printf("\x1b[11;10HDificultad: %s",
                    currentLevel <= 3 ? "Facil" :
                    currentLevel <= 6 ? "Normal" :
                    currentLevel <= 8 ? "Dificil" : "Demon");
                printf("\x1b[15;8H[A] Jugar Nivel   |   [B] Volver");
            }
            else if (state == STATE_CUSTOMIZE) {
                printf("\x1b[3;8H=== PERSONALIZAR WAVE ===");
                printf("\x1b[6;6H%s Forma:        < %s >", (customCursor == 0 ? "->" : "  "), SHAPE_NAMES[wave.shapeID]);
                printf("\x1b[8;6H%s Color Wave:   < %s >", (customCursor == 1 ? "->" : "  "), COLOR_NAMES[wave.waveColorIdx]);
                printf("\x1b[10;6H%s Color Estela: < %s >", (customCursor == 2 ? "->" : "  "), COLOR_NAMES[wave.trailColorIdx]);
                printf("\x1b[14;6HArriba/Abajo: Opcion | Izq/Der: Cambiar");
                printf("\x1b[16;6H[B] Guardar y Volver");
            }
            else if (state == STATE_PLAYING) {
                int pct = (int)((distanceTraveled / levelLength) * 100.0f);
                if (pct > 100) pct = 100;
                printf("\x1b[4;8HNIVEL %d   |   Progreso: %3d%%  ", currentLevel, pct);
                printf("\x1b[8;8HManten [A] / [R] / Tactil para subir");
                printf("\x1b[10;8HSuelta para bajar en picada");
                printf("\x1b[15;8H[B] Rendirse y volver al menu");
            }
            else if (state == STATE_INFINITE) {
                printf("\x1b[4;8HMODO INFINITO");
                printf("\x1b[7;8HPuntuacion Actual: %d   ", infiniteScore);
                printf("\x1b[9;8HMejor Puntuacion:  %d   ", infiniteBest);
                printf("\x1b[15;8H[B] Volver al menu");
            }
            else if (state == STATE_GAMEOVER) {
                printf("\x1b[5;12H*** CRASH! ***");
                if (lastMode == STATE_PLAYING) {
                    int pct = (int)((distanceTraveled / levelLength) * 100.0f);
                    printf("\x1b[8;10HProgreso Nivel %d: %d%%", currentLevel, pct);
                } else {
                    printf("\x1b[8;10HPuntuacion: %d (Best: %d)", infiniteScore, infiniteBest);
                }
                printf("\x1b[12;8H[A] Reintentar   |   [B] Menu");
            }
            else if (state == STATE_VICTORY) {
                printf("\x1b[5;10H*** NIVEL %d COMPLETADO! ***", currentLevel);
                if (currentLevel < 10) {
                    printf("\x1b[9;8H[A] Siguiente Nivel (%d)", currentLevel + 1);
                } else {
                    printf("\x1b[9;8H¡Has completado los 10 niveles!");
                    printf("\x1b[11;8H[A] Jugar de nuevo");
                }
                printf("\x1b[14;8H[B] Volver al Menu");
            }
        }

        // --- RENDERIZADO PANTALLA SUPERIOR ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Color de fondo dinámico según el nivel
        u32 bgClr = C2D_Color32(15 + (currentLevel * 3), 18, 38 + (currentLevel * 4), 255);
        C2D_TargetClear(topScreen, bgClr);
        C2D_SceneBegin(topScreen);

        // Bordes superior e inferior de la pista
        u32 borderClr = PALETTE[wave.waveColorIdx];
        C2D_DrawRectangle(0, 0, 0.2f, SCREEN_WIDTH, 4, borderClr, borderClr, borderClr, borderClr);
        C2D_DrawRectangle(0, SCREEN_HEIGHT - 4, 0.2f, SCREEN_WIDTH, 4, borderClr, borderClr, borderClr, borderClr);

        if (state == STATE_CUSTOMIZE) {
            // Vista previa animada del Wave y su estela en la pantalla superior
            wave.x = 200.0f;
            wave.y = 120.0f + sinf(osGetTime() * 0.005f) * 35.0f;
            wave.goingUp = (cosf(osGetTime() * 0.005f) < 0);
            for (auto& pt : wave.trail) pt.x -= 3.0f;
            wave.trail.push_back({wave.x, wave.y});
            if (wave.trail.size() > 50) wave.trail.pop_front();
            drawWave(wave);
        }
        else if (state == STATE_PLAYING || state == STATE_INFINITE || state == STATE_GAMEOVER) {
            // Dibujar obstáculos con borde brillante
            u32 obsFill = C2D_Color32(25, 25, 45, 255);
            u32 obsEdge = C2D_Color32(255, 60, 80, 255);

            for (const auto& obs : obstacles) {
                C2D_DrawRectangle(obs.x, obs.y, 0.3f, obs.w, obs.h, obsEdge, obsEdge, obsEdge, obsEdge);
                if (obs.w > 4 && obs.h > 4) {
                    C2D_DrawRectangle(obs.x + 2, obs.y + 2, 0.35f, obs.w - 4, obs.h - 4, obsFill, obsFill, obsFill, obsFill);
                }
            }

            // Barra de progreso superior en los niveles 1-10
            if (state == STATE_PLAYING) {
                float ratio = distanceTraveled / levelLength;
                if (ratio > 1.0f) ratio = 1.0f;
                C2D_DrawRectangle(50, 10, 0.8f, 300, 6, C2D_Color32(50, 50, 70, 200), C2D_Color32(50, 50, 70, 200), C2D_Color32(50, 50, 70, 200), C2D_Color32(50, 50, 70, 200));
                C2D_DrawRectangle(50, 10, 0.85f, 300 * ratio, 6, borderClr, borderClr, borderClr, borderClr);
            }

            drawWave(wave);
        }

        C3D_FrameEnd();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
