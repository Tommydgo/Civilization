#include "ui/ui_dialog.h"
#include "ui/ui_layout.h"
#include <SFML/Graphics.h>
#include <string.h>
#include <stdio.h>

static void fill_rect_d(sfRenderWindow *w, float x, float y,
                        float rw, float rh, sfColor col)
{
    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setPosition(r, (sfVector2f){x, y});
    sfRectangleShape_setSize(r, (sfVector2f){rw, rh});
    sfRectangleShape_setFillColor(r, col);
    sfRenderWindow_drawRectangleShape(w, r, NULL);
    sfRectangleShape_destroy(r);
}

static void draw_text_d(sfRenderWindow *w, sfFont *f, const char *str,
                        float x, float y, unsigned int sz, sfColor col)
{
    sfText *t = sfText_create();
    sfText_setFont(t, f);
    sfText_setString(t, str);
    sfText_setCharacterSize(t, sz);
    sfText_setFillColor(t, col);
    sfText_setPosition(t, (sfVector2f){x, y});
    sfRenderWindow_drawText(w, t, NULL);
    sfText_destroy(t);
}

void dialog_text_input(sfRenderWindow *win, sfFont *font,
                       const char *prompt, char *buf, int len)
{
    char input[128] = {0};
    int  cursor = 0;
    sfEvent ev;

    while (sfRenderWindow_isOpen(win)) {
        while (sfRenderWindow_pollEvent(win, &ev)) {
            if (ev.type == sfEvtClosed) {
                strncpy(buf, "", (size_t)len);
                return;
            }
            if (ev.type == sfEvtKeyPressed) {
                if (ev.key.code == sfKeyEnter) {
                    strncpy(buf, input, (size_t)(len - 1));
                    buf[len - 1] = '\0';
                    return;
                }
                if (ev.key.code == sfKeyEscape) {
                    strncpy(buf, "", (size_t)len);
                    return;
                }
                if (ev.key.code == sfKeyBackspace && cursor > 0) {
                    input[--cursor] = '\0';
                }
            }
            if (ev.type == sfEvtTextEntered) {
                sfUint32 c = ev.text.unicode;
                if (c >= 32 && c < 127 && cursor < len - 2) {
                    input[cursor++] = (char)c;
                    input[cursor]   = '\0';
                }
            }
        }

        float pw = 500, ph = 120;
        float px = (WIN_W - pw) / 2;
        float py = (WIN_H - ph) / 2;

        fill_rect_d(win, 0, 0, WIN_W, WIN_H,
                    sfColor_fromRGBA(0, 0, 0, 160));
        fill_rect_d(win, px, py, pw, ph,
                    sfColor_fromRGB(26, 26, 46));

        sfRectangleShape *border = sfRectangleShape_create();
        sfRectangleShape_setPosition(border, (sfVector2f){px, py});
        sfRectangleShape_setSize(border, (sfVector2f){pw, ph});
        sfRectangleShape_setFillColor(border, sfColor_fromRGBA(0, 0, 0, 0));
        sfRectangleShape_setOutlineColor(border, sfColor_fromRGB(74, 74, 200));
        sfRectangleShape_setOutlineThickness(border, 1.f);
        sfRenderWindow_drawRectangleShape(win, border, NULL);
        sfRectangleShape_destroy(border);

        draw_text_d(win, font, prompt, px + 12, py + 14,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 215, 0));

        fill_rect_d(win, px + 12, py + 50, pw - 24, 30,
                    sfColor_fromRGB(10, 10, 30));

        char display[130];
        snprintf(display, sizeof(display), "%s_", input);
        draw_text_d(win, font, display, px + 16, py + 56,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 255, 255));

        draw_text_d(win, font, "[Entree] confirmer   [Echap] annuler",
                    px + 12, py + 92, FONT_SIZE_SM,
                    sfColor_fromRGB(120, 120, 140));

        sfRenderWindow_display(win);
    }
    strncpy(buf, "", (size_t)len);
}

int dialog_list_select(sfRenderWindow *win, sfFont *font,
                       const char *title, const char **items,
                       int count, int default_sel)
{
    int sel = default_sel;
    sfEvent ev;

    while (sfRenderWindow_isOpen(win)) {
        while (sfRenderWindow_pollEvent(win, &ev)) {
            if (ev.type == sfEvtClosed)
                return -1;
            if (ev.type == sfEvtKeyPressed) {
                if (ev.key.code == sfKeyUp   && sel > 0)         sel--;
                if (ev.key.code == sfKeyDown && sel < count - 1) sel++;
                if (ev.key.code == sfKeyEnter)  return sel;
                if (ev.key.code == sfKeyEscape) return default_sel;
            }
        }

        float pw = 600;
        float ph = 60 + count * 22 + 30;
        if (ph > WIN_H - 40) ph = (float)(WIN_H - 40);
        float px = (WIN_W - pw) / 2;
        float py = (WIN_H - ph) / 2;

        fill_rect_d(win, 0, 0, WIN_W, WIN_H,
                    sfColor_fromRGBA(0, 0, 0, 160));
        fill_rect_d(win, px, py, pw, ph,
                    sfColor_fromRGB(20, 20, 40));

        sfRectangleShape *border = sfRectangleShape_create();
        sfRectangleShape_setPosition(border, (sfVector2f){px, py});
        sfRectangleShape_setSize(border, (sfVector2f){pw, ph});
        sfRectangleShape_setFillColor(border, sfColor_fromRGBA(0, 0, 0, 0));
        sfRectangleShape_setOutlineColor(border, sfColor_fromRGB(74, 74, 200));
        sfRectangleShape_setOutlineThickness(border, 1.f);
        sfRenderWindow_drawRectangleShape(win, border, NULL);
        sfRectangleShape_destroy(border);

        draw_text_d(win, font, title, px + 12, py + 10,
                    FONT_SIZE_MD, sfColor_fromRGB(255, 215, 0));
        draw_text_d(win, font,
                    "Haut/Bas pour naviguer, Entree pour confirmer",
                    px + 12, py + 32,
                    FONT_SIZE_SM, sfColor_fromRGB(120, 120, 140));

        for (int i = 0; i < count; i++) {
            float iy = py + 58 + i * 22;
            if (iy + 22 > py + ph - 4) break;
            if (i == sel) {
                fill_rect_d(win, px + 8, iy - 2, pw - 16, 20,
                            sfColor_fromRGB(40, 40, 100));
            }
            sfColor ic = (i == sel) ? sfColor_fromRGB(255, 255, 255)
                                    : sfColor_fromRGB(180, 180, 200);
            draw_text_d(win, font, items[i], px + 14, iy,
                        FONT_SIZE_SM, ic);
        }
        sfRenderWindow_display(win);
    }
    return default_sel;
}
