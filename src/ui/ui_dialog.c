#include "ui/ui_dialog.h"
#include "ui/ui_layout.h"
#include <string.h>
#include <stdio.h>

static void fill_rect_d(float x, float y, float rw, float rh, Color col)
{
    DrawRectangleRec((Rectangle){x, y, rw, rh}, col);
}

static void draw_text_d(Font f, const char *str, float x, float y,
                        int sz, Color col)
{
    DrawTextEx(f, str, (Vector2){x, y}, (float)sz, 1.0f, col);
}

void dialog_text_input(Font font, const char *prompt, char *buf, int len)
{
    char input[128] = {0};
    int cursor = 0;
    float pw = 500;
    float ph = 120;
    float px = (WIN_W - pw) / 2;
    float py = (WIN_H - ph) / 2;

    while (!WindowShouldClose()) {
        int c = GetCharPressed();
        while (c > 0) {
            if (c >= 32 && c < 127 && cursor < len - 2) {
                input[cursor++] = (char)c;
                input[cursor] = '\0';
            }
            c = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && cursor > 0)
            input[--cursor] = '\0';
        if (IsKeyPressed(KEY_ENTER)) {
            strncpy(buf, input, (size_t)(len - 1));
            buf[len - 1] = '\0';
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            buf[0] = '\0';
            return;
        }

        BeginDrawing();
        ClearBackground(COL_BG);
        fill_rect_d(0, 0, WIN_W, WIN_H, (Color){0, 0, 0, 160});
        fill_rect_d(px, py, pw, ph, (Color){26, 26, 46, 255});
        DrawRectangleLinesEx((Rectangle){px, py, pw, ph}, 1.0f,
                             (Color){74, 74, 200, 255});
        draw_text_d(font, prompt, px + 12, py + 14,
                    FONT_SIZE_MD, (Color){255, 215, 0, 255});
        fill_rect_d(px + 12, py + 50, pw - 24, 30,
                    (Color){10, 10, 30, 255});

        char display[130];
        snprintf(display, sizeof(display), "%s_", input);
        draw_text_d(font, display, px + 16, py + 56,
                    FONT_SIZE_MD, (Color){255, 255, 255, 255});
        draw_text_d(font, "[Entree] confirmer   [Echap] annuler",
                    px + 12, py + 92, FONT_SIZE_SM,
                    (Color){120, 120, 140, 255});
        EndDrawing();
    }
    buf[0] = '\0';
}

int dialog_list_select(Font font, const char *title, const char **items,
                       int count, int default_sel)
{
    int sel = default_sel;
    float pw = 600;
    float ph = 60 + count * 22 + 30;
    if (ph > WIN_H - 40)
        ph = (float)(WIN_H - 40);
    float px = (WIN_W - pw) / 2;
    float py = (WIN_H - ph) / 2;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_UP)    && sel > 0)         sel--;
        if (IsKeyPressed(KEY_DOWN)  && sel < count - 1) sel++;
        if (IsKeyPressed(KEY_ENTER))  return sel;
        if (IsKeyPressed(KEY_ESCAPE)) return default_sel;

        BeginDrawing();
        ClearBackground(COL_BG);
        fill_rect_d(0, 0, WIN_W, WIN_H, (Color){0, 0, 0, 160});
        fill_rect_d(px, py, pw, ph, (Color){20, 20, 40, 255});
        DrawRectangleLinesEx((Rectangle){px, py, pw, ph}, 1.0f,
                             (Color){74, 74, 200, 255});
        draw_text_d(font, title, px + 12, py + 10,
                    FONT_SIZE_MD, (Color){255, 215, 0, 255});
        draw_text_d(font, "Haut/Bas pour naviguer, Entree pour confirmer",
                    px + 12, py + 32, FONT_SIZE_SM,
                    (Color){120, 120, 140, 255});

        int i = 0;
        while (i < count) {
            float iy = py + 58 + i * 22;
            if (iy + 22 > py + ph - 4) break;
            if (i == sel)
                fill_rect_d(px + 8, iy - 2, pw - 16, 20,
                            (Color){40, 40, 100, 255});
            Color ic = (i == sel) ? (Color){255, 255, 255, 255}
                                  : (Color){180, 180, 200, 255};
            draw_text_d(font, items[i], px + 14, iy, FONT_SIZE_SM, ic);
            i++;
        }
        EndDrawing();
    }
    return default_sel;
}
