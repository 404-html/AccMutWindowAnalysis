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

ACCMUTV2_FILE *__accmutv2__fopen(const char *path, const char *mode);
int __accmutv2__fclose(ACCMUTV2_FILE *fp);
int __accmutv2__feof(ACCMUTV2_FILE *fp);
int __accmutv2__fileno(ACCMUTV2_FILE *fp);
char *__accmutv2__fgets(char *buf, int size, ACCMUTV2_FILE *fp);
int __accmutv2__fputs(const char *buf, ACCMUTV2_FILE *fp);
int __accmutv2__ungetc(int c, ACCMUTV2_FILE *fp);
int __accmutv2__fseek(ACCMUTV2_FILE *fp, long offset, int whence);
long __accmutv2__ftell(ACCMUTV2_FILE *fp);
void __accmutv2__rewind(ACCMUTV2_FILE *fp);

#ifdef __cplusplus
}
#endif

#endif //LLVM_ACCMUT_IO_STDIO_H
