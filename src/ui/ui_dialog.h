#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <raylib.h>

void dialog_text_input(Font font, const char *prompt, char *buf, int len);
int  dialog_list_select(Font font, const char *title, const char **items,
                        int count, int default_sel);

#endif
