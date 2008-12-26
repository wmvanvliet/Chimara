#include "stdio.h"
#include "glk.h"

void glk_main(void)
{
    event_t ev;
    winid_t mainwin = glk_window_open(0, 0, 0, wintype_TextBuffer, 0);
    if(!mainwin)
        return;
    
    glk_set_window(mainwin);
    glk_put_string("Philip en Marijn zijn vet goed.\n");
    glk_put_string("A veeeeeeeeeeeeeeeeeeeeeeeeeeeery looooooooooooooooooooooooong striiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiing.\n");
    
    guint32 width, height;
    glk_window_get_size(mainwin, &width, &height);
    fprintf(stderr, "\nWidth: %d\nHeight: %d\n", width, height);
    
    glk_request_char_event(mainwin);
    while(1) {
        glk_select(&ev);
        if(ev.type == evtype_CharInput) {
            glk_window_get_size(mainwin, &width, &height);
            fprintf(stderr, "\nWidth: %d\nHeight: %d\n", width, height);
        }
    }
}