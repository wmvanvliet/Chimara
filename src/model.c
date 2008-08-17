#include "glk.h"

static winid_t mainwin = NULL;

void glk_main(void)
{
    /* Open the main window. */
    mainwin = glk_window_open(0, 0, 0, wintype_TextBuffer, 1);
    if (!mainwin) {
        /* It's possible that the main window failed to open. There's
            nothing we can do without it, so exit. */
        return; 
    }
    
    char buffer[256];
    int i;
    for(i = 0; i < 256; i++)
    	buffer[i] = (char)glk_char_to_upper(i);
    
    /*frefid_t f = glk_fileref_create_temp(fileusage_BinaryMode, 0);
    if(f) 
    {*/
    
    char memorybuffer[100];
    
	strid_t s = glk_stream_open_memory(memorybuffer, 100, 
		filemode_ReadWrite, 0);
	glk_stream_set_current(s);
	glk_put_char('X');
	glk_put_string("Philip en Marijn zijn vet goed.\n");
	glk_put_buffer(buffer, 256);

	glk_stream_set_position(s, 0, seekmode_Start);
	glk_set_window(mainwin);
	glk_put_char( glk_get_char_stream(s) );
	glk_put_char('\n');
	g_printerr("Line read: %d\n", glk_get_line_stream(s, buffer, 256));
	glk_put_string(buffer);
	int count = glk_get_buffer_stream(s, buffer, 256);
	g_printerr("Buffer read: %d\n", count);
	glk_put_buffer(buffer, count);		
	
	stream_result_t result;
	glk_stream_close(s, &result);
	
	g_printerr("Read count: %d\nWrite count: %d\n", result.readcount,
		result.writecount);
/*
		glk_fileref_destroy(f);
	}*/
	
	/* Bye bye */
	glk_exit();
}
