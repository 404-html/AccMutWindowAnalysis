//
// Created by lusirui on 19-3-10.
//

#ifndef LLVM_ACCMUT_IO_H
#define LLVM_ACCMUT_IO_H

#ifdef __cplusplus
extern "C" {
#endif

int __accmutv2__open(const char *pathname, int flags, ...);
int __accmutv2__creat(const char *pathname, mode_t mode);
int __accmutv2__close(int fd);
ssize_t __accmutv2__lseek(int fd, off_t offset, int whence);
ssize_t __accmutv2__read(int fd, void *buf, size_t count);
ssize_t __accmutv2__write(int fd, const void *buf, size_t count);
void __accmutv2__checkfd_func();
void __accmutv2__dump__text(int fd);
void __accmutv2__dump__bin(int fd);

typedef struct ACCMUTV2_FILE {
    int flags;
    int fd;
    FILE *orifile;
} ACCMUTV2_FILE;

extern ACCMUTV2_FILE *accmutv2_stdin;
extern ACCMUTV2_FILE *accmutv2_stdout;
extern ACCMUTV2_FILE *accmutv2_stderr;

ACCMUTV2_FILE *__accmutv2__fopen(const char *path, const char *mode);
int __accmutv2__fclose(ACCMUTV2_FILE *fp);
int __accmutv2__feof(ACCMUTV2_FILE *fp);
int __accmutv2__fileno(ACCMUTV2_FILE *fp);
ACCMUTV2_FILE *__accmut__fgets(char *buf, int size, ACCMUTV2_FILE *fp);
size_t __accmut__puts(char *buf, int size, ACCMUTV2_FILE *fp);

#ifdef __cplusplus
};
#endif

#define __accmutv2__checkfd() do {\
    if (MUTATION_ID == 0) {\
        fprintf(stderr, "Check fd at %s:%s:%d:\n", __FILE__, __FUNCTION__, __LINE__);\
        __accmutv2__checkfd_func();\
    }\
} while (0)\

#endif //LLVM_ACCMUT_IO_H
