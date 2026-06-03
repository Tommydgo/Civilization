#include "ui/ui_dialog.h"
#include <string.h>
void dialog_text_input(sfRenderWindow *w, sfFont *f,
                       const char *p, char *buf, int len)
    { (void)w;(void)f;(void)p; strncpy(buf,"",len); }
int dialog_list_select(sfRenderWindow *w, sfFont *f,
                       const char *t, const char **i, int c, int d)
    { (void)w;(void)f;(void)t;(void)i;(void)c; return d; }
