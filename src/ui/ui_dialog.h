#ifndef UI_DIALOG_H
#define UI_DIALOG_H
#include <SFML/Graphics.h>
void dialog_text_input(sfRenderWindow *win, sfFont *font,
                       const char *prompt, char *buf, int len);
int  dialog_list_select(sfRenderWindow *win, sfFont *font,
                        const char *title, const char **items,
                        int count, int default_sel);
#endif
