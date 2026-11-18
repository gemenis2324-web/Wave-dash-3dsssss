#include <citro2d.h>
#include <citro3d.h>
#include <ctru.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

// Estados del juego
enum GameState {
    STATE_MENU,
    STATE_LEVEL_SELECT,
    STATE_CUSTOMIZE,
    STATE_PLAYING,
    STATE_INFINITE
};

// Estructura del Jugador (Wave)
struct WavePlayer {
    float x, y;
    float speed;
    bool goingUp;
    u32 trailColor;
    int skinID;
};

// Estructura de Obstáculos
struct Obstacle {
    float x, y, width, height;
    bool active;
};

int main(int argc, char* argv[]) {
    // Inicializar servicios de 3DS
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);

    C3D_RenderTarget* topScreen = C2D_CreateTarget(SCREEN_WIDTH, SCREEN_HEIGHT, GFX_TOP, GFX_LEFT);

    GameState state = STATE_MENU;
    WavePlayer wave = {50.0f, 120.0f, 4.0f, false, C2D_Color32(0, 255, 255, 255), 0};
    
    int currentLevel = 1;
    std::vector<Obstacle> obstacles;

    // Colores base
    u32 clrBg = C2D_Color32(20, 20, 40, 255);
    u32 clrWave = C2D_Color32(0, 255, 255, 255);

    // Bucle principal
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break; // Salir al menú de Homebrew

        // --- LÓGICA DE ESTADOS ---
        if (state == STATE_MENU) {
            if (kDown & KEY_A) state = STATE_LEVEL_SELECT;
            if (kDown & KEY_X) state = STATE_CUSTOMIZE;
            if (kDown & KEY_Y) state = STATE_INFINITE;
        } 
        else if (state == STATE_LEVEL_SELECT) {
            if (kDown & KEY_DOWN && currentLevel < 10) currentLevel++;
            if (kDown & KEY_UP && currentLevel > 1) currentLevel--;
            if (kDown & KEY_A) {
                // Cargar nivel (A mayor nivel, mayor velocidad de obstáculos)
                wave.y = 120.0f;
                wave.speed = 3.0f + (currentLevel * 0.3f);
                obstacles.clear();
                // Generar obstáculos de prueba para el nivel
                obstacles.push_back({200.0f, 100.0f, 20.0f, 60.0f, true});
                state = STATE_PLAYING;
            }
            if (kDown & KEY_B) state = STATE_MENU;
        }
        else if (state == STATE_CUSTOMIZE) {
            if (kDown & KEY_LEFT) wave.skinID = (wave.skinID - 1 + 3) % 3;
            if (kDown & KEY_RIGHT) wave.skinID = (wave.skinID + 1) % 3;
            if (kDown & KEY_B) state = STATE_MENU;
        }
        else if (state == STATE_PLAYING || state == STATE_INFINITE) {
            // Física del Wave: Mantener presionado para subir en diagonal, soltar para bajar
            if (kHeld & KEY_A || kHeld & KEY_TOUCH) {
                wave.y -= wave.speed;
            } else {
                wave.y += wave.speed;
            }

            // Límites de pantalla superior
            if (wave.y < 0) wave.y = 0;
            if (wave.y > SCREEN_HEIGHT - 10) wave.y = SCREEN_HEIGHT - 10;

            // Movimiento de obstáculos
            for (auto& obs : obstacles) {
                obs.x -= wave.speed;
            }

            // Modo Infinito: Generar obstáculos proceduralmente
            if (state == STATE_INFINITE && (rand() % 60 == 0)) {
                float h = 40.0f + (rand() % 80);
                float yPos = (rand() % 2 == 0) ? 0 : SCREEN_HEIGHT - h;
                obstacles.push_back({(float)SCREEN_WIDTH, yPos, 20.0f, h, true});
            }

            if (kDown & KEY_B) state = STATE_MENU;
        }

        // --- RENDERIZADO ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topScreen, clrBg);
        C2D_SceneBegin(topScreen);

        if (state == STATE_MENU) {
            printf("\x1b[10;10H=== WAVE DASH 3DS ===");
            printf("\x1b[12;10H[A] Jugar Niveles (1-10)");
            printf("\x1b[13;10H[Y] Modo Infinito");
            printf("\x1b[14;10H[X] Personalizar Wave/Estela");
            printf("\x1b[16;10H[START] Salir");
        } 
        else if (state == STATE_LEVEL_SELECT) {
            printf("\x1b[10;10HSelecciona Nivel (1 al 10):");
            printf("\x1b[12;15H--> Nivel: %d <---", currentLevel);
            printf("\x1b[15;10H[A] Iniciar | [B] Volver");
        }
        else if (state == STATE_CUSTOMIZE) {
            printf("\x1b[10;10HPersonalizacion de Estela:");
            printf("\x1b[12;10HSkin actual ID: %d (Usa Izq/Der)", wave.skinID);
            printf("\x1b[15;10H[B] Volver al Menu");
        }
        else if (state == STATE_PLAYING || state == STATE_INFINITE) {
            // Dibujar jugador (Wave representado como un triángulo o bloque básico)
            C2D_DrawRectangle(wave.x, wave.y, 0.5f, 10.0f, 10.0f, clrWave, clrWave, clrWave, clrWave);

            // Dibujar obstáculos
            for (const auto& obs : obstacles) {
                if (obs.active) {
                    C2D_DrawRectangle(obs.x, obs.y, 0.5f, obs.width, obs.height, 
                                      C2D_Color32(255, 50, 50, 255), 
                                      C2D_Color32(255, 50, 50, 255), 
                                      C2D_Color32(255, 50, 50, 255), 
                                      C2D_Color32(255, 50, 50, 255));
                }
            }
        }

        C3D_FrameEnd();
    }

    // Limpieza de recursos
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
