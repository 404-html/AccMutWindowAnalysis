//#ifndef ACCMUT_UNDEF_H
//#define ACCMUT_UNDEF_H

/* USING IN THE MAIN FUNCTION, TO SWITCH FROM ACCMUT_IO TO STDIO */

//#ifdef USE_ACCMUT_IO //TODO:: use the same MACRO with accmut_redefine.d ? 

#undef FILE
#undef stdout
#undef stderr
#undef stdin

#undef fopen
#undef fclose
#undef ferror
#undef freopen

#undef unlink
#undef rename

#undef fget
#undef getc
#undef fread

#undef fputc
#undef putc
#undef fputs
#undef puts
#undef fwrite
#undef fprintf
#undef printf

//#endif

//#endif
