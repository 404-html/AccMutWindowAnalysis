//
// Created by lusirui on 19-3-11.
//

#ifndef LLVM_ACCMUT_IO_STDIO_H
#define LLVM_ACCMUT_IO_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ACCMUTV2_FILE {
    int flags;
    int fd;
    FILE *orifile;
    int unget_char;
    int eof_seen;
} ACCMUTV2_FILE;

extern ACCMUTV2_FILE *accmutv2_stdin;
extern ACCMUTV2_FILE *accmutv2_stdout;
extern ACCMUTV2_FILE *accmutv2_stderr;

#ifdef __APPLE__
#define restrict __restrict
#endif

/** File management **/
ACCMUTV2_FILE *__accmutv2__fopen(const char *path, const char *mode);
int __accmutv2__fclose(ACCMUTV2_FILE *fp);
ACCMUTV2_FILE *__accmutv2__freopen(const char *path, const char *mode, ACCMUTV2_FILE *fp);

/** File status **/
int __accmutv2__feof(ACCMUTV2_FILE *fp);
int __accmutv2__fileno(ACCMUTV2_FILE *fp);
int __accmutv2__fflush(ACCMUTV2_FILE *fp);
int __accmutv2__ferror(ACCMUTV2_FILE *fp);

/** File position **/
int __accmutv2__fseek(ACCMUTV2_FILE *fp, long offset, int whence);
long __accmutv2__ftell(ACCMUTV2_FILE *fp);
int __accmutv2__fseeko(ACCMUTV2_FILE *fp, off_t offset, int whence);
off_t __accmutv2__ftello(ACCMUTV2_FILE *fp);
void __accmutv2__rewind(ACCMUTV2_FILE *fp);

/** Input **/
size_t __accmutv2__fread(void *restrict ptr, size_t size, size_t nitems, ACCMUTV2_FILE restrict *stream);
char *__accmutv2__fgets(char *buf, int size, ACCMUTV2_FILE *fp);
int __accmutv2__fgetc(ACCMUTV2_FILE *fp);
int __accmutv2__getc(ACCMUTV2_FILE *fp);
int __accmutv2__ungetc(int c, ACCMUTV2_FILE *fp);

/** Output **/
int __accmutv2__fputs(const char *buf, ACCMUTV2_FILE *fp);
size_t __accmutv2__fwrite(const void *restrict ptr, size_t size, size_t nitems, ACCMUTV2_FILE *restrict stream);
int __accmutv2__vfprintf(ACCMUTV2_FILE *restrict stream, const char *restrict format, va_list ap);
int __accmutv2__fprintf(ACCMUTV2_FILE *restrict stream, const char *restrict format, ...);
int __accmutv2__printf(const char *format, ...);
void __accmutv2__perror(const char *s);

#ifdef __cplusplus
}
#endif

#endif //LLVM_ACCMUT_IO_STDIO_H
