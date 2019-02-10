#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "llvm/AccMutRuntime/accmut_config.h"
#include "llvm/AccMutRuntime/accmut_io.h"

/*************************************************/

#define ACCMUT_IO_DEBUG 0

//INPUT BUF
#define MAX_IN_BUF_SIZE (1<<22)

//4M OUTPUT BUF
#define MAX_STDOUT_BUF_SIZE (1<<22)
#define MAX_STDERR_BUF_SIZE (1<<22)
#define MAX_FILE_BUF_SIZE (1<<22)
/*************************************************/

# define EOF (-1)

/***********************************************************/

char STDOUT_BUFF[MAX_STDOUT_BUF_SIZE];
char STDERR_BUFF[MAX_STDERR_BUF_SIZE];
size_t CUR_STDOUT = 0;
size_t CUR_STDERR = 0;

/***********************************************************/

#define DEF_STDFILE(NAME, FD, BUF, MAXSIZE, FLAGS, ORINAME) \
    ACCMUT_FILE NAME = {FLAGS, FD, BUF, (BUF + MAXSIZE), BUF, BUF, 0, NULL, 0, ORINAME}

DEF_STDFILE(stdfile_0, 0, (char *) NULL, 0, 0, "stdin");    //unimplemented stdin
DEF_STDFILE(stdfile_1, 1, STDOUT_BUFF, MAX_STDOUT_BUF_SIZE, O_WRONLY, "stdout");
DEF_STDFILE(stdfile_2, 2, STDERR_BUFF, MAX_STDERR_BUF_SIZE, O_WRONLY, "stderr");

ACCMUT_FILE *accmut_stdin = &stdfile_0;
ACCMUT_FILE *accmut_stdout = &stdfile_1;
ACCMUT_FILE *accmut_stderr = &stdfile_2;

int __real_fprintf(FILE *fp, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    return vfprintf(fp, format, ap);
    va_end(ap);
}

#define NOCALLLOG

#ifndef NOCALLLOG

#define _CALLLOG(dummy, fp, ...) \
    if (MUTATION_ID == 0) {\
        __real_fprintf(stderr, "pid: %d\n", getpid());\
        if ((fp) != NULL) {\
            __real_fprintf(stderr, "\033[32mCalling %s on %s\n\033[0m", __FUNCTION__, (fp)->filename);\
        } else {\
            __real_fprintf(stderr, "\033[32mCalling %s\n\033[0m", __FUNCTION__);\
        }\
    }

#define CALLLOG(...) _CALLLOG(0, ##__VA_ARGS__, (ACCMUT_FILE*)(NULL))

#else

#define CALLLOG(...)

#endif


/*********************** FILE OPTIONS ****************************************/


ACCMUT_FILE *__accmut__fopen(const char *path, const char *mode) {

    int omode;
    int oflags = 0;
    int oprot = 0666;
    const char *orimode = mode;
    int needTrunc = 0;

    switch (*mode++) {
        case 'r':
            omode = O_RDONLY;
            break;
        case 'w':
            omode = O_WRONLY;
            oflags = O_CREAT | O_TRUNC;
            needTrunc = 1;
            break;
            // case 'a':
            // 	0flags = O_WRONLY | O_APPEND;
            // 	break;
        default:
            fprintf(stderr, "ERROR FOPEN MODE: %s ! TID: %d, MUT: %d\n", mode, TEST_ID, MUTATION_ID);
            exit(0);
    }

    // if (m[0] == '+' || (m[0] == 'b' && m[1] == '+')){
    // 	omode = O_RDONLY | O_WRONLY;
    // }

    /** new **/
    ACCMUT_FILE *fp = (ACCMUT_FILE *) malloc(sizeof(ACCMUT_FILE));
    if (fp == NULL)
        return NULL;

    fp->filename = (char *) malloc(strlen(path) + 10);
    strcpy(fp->filename, path);
    CALLLOG(fp);

    fp->orifile = NULL;

    int _fd;
    if (MUTATION_ID != 0 && needTrunc) {
        fp->usetmp = 1;
        fp->orifile = tmpfile();
        _fd = fileno(fp->orifile);
    } else {
        // can truncate
        _fd = open(path, omode | oflags, oprot);
        fp->usetmp = 0;
    }

    if (_fd < 0) {
#if ACCMUT_IO_DEBUG
        fprintf(stdout, "OPEN ERROR, PATH: \"%s\", MODE: %d  @__accmut__fopen. TID: %d , MUT: %d\n", path, omode, TEST_ID, MUTATION_ID);
#endif
        free(fp->filename);
        free(fp);
        return NULL;
    }
    /** new **/
    /** original
    int _fd = open(path, omode | oflags, oprot);

    if(_fd < 0){
#if ACCMUT_IO_DEBUG
        fprintf(stdout, "OPEN ERROR, PATH: \"%s\", MODE: %d  @__accmut__fopen. TID: %d , MUT: %d\n", path, omode, TEST_ID, MUTATION_ID);
#endif
        return NULL;
    }

    ACCMUT_FILE *fp = (ACCMUT_FILE *)malloc(sizeof(ACCMUT_FILE));

    if(fp == NULL)
        return NULL;
    fp->orifile = NULL;
    fp->usetmp = 0;

    /** end original **/


    if (omode == O_RDONLY) {

        struct stat sb;
        if (fstat(_fd, &sb) == -1) {
            return NULL;
        }
        if (sb.st_size > MAX_IN_BUF_SIZE) {
#if ACCMUT_IO_DEBUG
            fprintf(stderr, "INPUT FILE : %s IS TOO BIG. TID: %d, MUT: %d\n", path, TEST_ID, MUTATION_ID);
#endif
            return NULL;
        }
        fp->flags = O_RDONLY;
        fp->fd = _fd;
        fp->bufbase = fp->read_cur = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, _fd, 0);
        fp->write_cur = NULL;
        fp->fsize = sb.st_size;
        fp->bufend = fp->bufbase + fp->fsize;
        if (MUTATION_ID == 0) {
            fp->orifile = fopen(path, orimode);
            if (fp->orifile == NULL) {
                if (fp->read_cur) {
                    munmap(fp->read_cur, sb.st_size);
                }
                free(fp);
                return NULL;
            }
        }

    } else if (omode == O_WRONLY) {

        fp->flags = O_WRONLY;
        fp->fd = _fd;
        fp->bufbase = fp->write_cur = (char *) calloc(1, MAX_FILE_BUF_SIZE * (sizeof(char)));
        fp->read_cur = NULL;
        fp->fsize = 0;
        fp->bufend = fp->bufbase + MAX_FILE_BUF_SIZE * (sizeof(char));
        if (MUTATION_ID == 0) {
            fp->orifile = fopen(path, orimode);
            if (fp->orifile == NULL) {
                ERRMSG("ok?");
                if (fp->write_cur) {
                    free(fp->write_cur);
                }
                free(fp);
                return NULL;
            }
            ERRMSG("ok");
        }
    }

    return fp;
}


int __accmut__fclose(ACCMUT_FILE *fp) {
    CALLLOG(fp);
    //if (fp->filename)
    //    free(fp->filename);
    int status = 0;
    if (fp->usetmp) {
        return fclose(fp->orifile);
    }
    if (fp->orifile) {
        status = fclose(fp->orifile);
    }
    if (fp->flags == O_RDONLY) {
        status = status & munmap(fp->bufbase, fp->fsize);
        status = status & close(fp->fd);
        free(fp);
    } else if (fp->flags == O_WRONLY) {

        // if(fp == accmut_stdout || fp == accmut_stderr){
        // 	fp->bufend = fp->bufbase;	// TODO
        // 	status = close(fp->fd);
        // 	return status;
        // }

        status = status & close(fp->fd);

        //fprintf(stderr, "  Close : %d  %d  %d\n", fp->fd, status, MUTATION_ID);
        // TODO: save results
        if (fp->fd != 1 && fp->fd != 2) {
            free(fp->bufbase);
            free(fp);
        }
    }
    return status;
}


int __accmut__feof(ACCMUT_FILE *fp) {
    CALLLOG(fp);

    if (fp == NULL) {
        return EOF;
    }
    if ((fp->flags & O_RDONLY) == 0) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "OPLY SUPPORT O_RDONLY MODE @__accmut__feof\n");
#endif
        return EOF;
    }

    int result = (fp->flags & _IO_EOF_SEEN) != 0;
    return result;
}

int __accmut__fseek(ACCMUT_FILE *fp, size_t offset, int loc) {
    CALLLOG(fp);
    //TODO
    return 0;
}

int __accmut__ferror(ACCMUT_FILE *fp) {
    CALLLOG(fp);
    //TODO:
    return 0;
}

int __accmut__fileno(ACCMUT_FILE *fp) {
    CALLLOG(fp);
    return fp->fd;
}

ACCMUT_FILE *__accmut__freopen(const char *path, const char *mode, ACCMUT_FILE *fp) {
    CALLLOG(fp);
    ACCMUT_FILE *newfp = __accmut__fopen(path, mode);
    if (newfp == NULL) {
        return NULL;
    }

    int status = close(fp->fd);

    if (fp->fd > 2) {//not stdin, stdout, stderr
        if (fp->flags == O_RDONLY) {
            status = munmap(fp->bufbase, fp->fsize);
            status = status & close(fp->fd);
        } else if (fp->flags == O_WRONLY) {
            status = close(fp->fd);
            free(fp->bufbase);
        }
    }

    if (status < 0) {
        return NULL;
    }

    // fp->flags = newfp->flags;
    // fp->fd = newfp->fd;
    // fp->bufbase = newfp->bufbase;
    // fp->write_cur = newfp->write_cur;
    // fp->read_cur = newfp->read_cur;
    // fp->bufend = newfp->bufend;
    // fp->fsize = newfp->fsize;

    if (fp->fd == 1) {
        accmut_stdout = newfp;
    } else if (fp->fd == 2) {
        accmut_stderr = newfp;
    } else {
        fprintf(stderr, "FREOPEN ERROR @__accmut__freopen. TID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
    }

    return newfp;
}

/*********************** POSIX FILE ****************************************/
int __accmut__unlink(const char *pathname) {
    CALLLOG();

    if (MUTATION_ID == 0) {// only main process can unlink the tmp file
        return unlink(pathname);
    } else {
        return 0;
    }
}

/*********************** INPUT ****************************************/

char *__accmut__fgets(char *buf, int size, ACCMUT_FILE *fp) {
    CALLLOG(fp);
    if (size <= 0)
        return NULL;

    if (fp->read_cur - fp->bufbase >= fp->fsize) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "READ OVERFLOW @ __accmut__fgets, TID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        fp->flags |= _IO_EOF_SEEN;
        return NULL;
    }

    size_t len = size - 1;

    char *t = (char *) memchr((void *) fp->read_cur, '\n', len);

    if (t != NULL) {
        len = t - fp->read_cur;
        ++t;
        ++len;
    }

    if (len == 0) {
        return NULL;
    }

    memcpy((void *) buf, (void *) fp->read_cur, len);

    fp->read_cur = fp->read_cur + len;

    *(buf + len) = '\0';

    if (MUTATION_ID == 0) {
        char intenalbuf[size + 1];
        //ERRMSG(fp->orifile->_IO_read_ptr);
        int before = 0, after = 0;
        // ERRMSG("\033[33mBegin dump content\n\033[0m");

        // ERRMSG2("base", (int)(fp->orifile->_IO_read_base));
        // ERRMSG2("ptr", (int)(fp->orifile->_IO_read_ptr));
        // ERRMSG("\033[32mBegin dump\n\033[0m");
        /* if (fp->orifile->_IO_read_base)
            ERRMSG2("len of base before", (int)strlen(fp->orifile->_IO_read_base));
        if (fp->orifile->_IO_read_ptr)
            ERRMSG2("len of cur before", (before = (int)strlen(fp->orifile->_IO_read_ptr)));
        */
        fgets(intenalbuf, size, fp->orifile);
        /*
        if (fp->orifile->_IO_read_base)
            ERRMSG2("len of base after", (int)strlen(fp->orifile->_IO_read_base));
        if (fp->orifile->_IO_read_ptr)
            ERRMSG2("len of cur after", (after = (int)strlen(fp->orifile->_IO_read_ptr)));
        ERRMSG("\033[33mBegin dump content\n\033[0m");

        ERRMSG2("base", (int)(fp->orifile->_IO_read_base));
        ERRMSG2("ptr", (int)(fp->orifile->_IO_read_ptr));
        //ERRMSG("\033[32m");
        //ERRMSG("\033[0m");
        ERRMSG(buf);
        ERRMSG(intenalbuf);
        */
        if (strcmp(buf, intenalbuf) != 0) {
            // panic
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
    }


    return buf;
}

int __accmut__getc(ACCMUT_FILE *fp) {
    CALLLOG(fp);
    if (fp->read_cur - fp->bufbase >= fp->fsize) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "READ OVERFLOW @ __accmut__getc, TID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        fp->flags |= _IO_EOF_SEEN;
        return EOF;
    }

    int ret = *((unsigned char *) fp->read_cur++);

    if (MUTATION_ID == 0) {
        if (getc(fp->orifile) != ret) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
    }

    return ret;
}

int __accmut__fgetc(ACCMUT_FILE *fp) {
    CALLLOG(fp);
    return __accmut__getc(fp);
}

size_t __accmut__fread(void *buf, size_t size, size_t count, ACCMUT_FILE *fp) {
    CALLLOG(fp);
    ssize_t bytes_requested = size * count;
    if (bytes_requested == 0)
        return 0;

    //fprintf(stderr, "%d %d %d\n", fp->fd, bytes_requested, fp->bufend - fp->read_cur);

    char *s = buf;

    if (fp->read_cur + bytes_requested > fp->bufend) {

#if ACCMUT_IO_DEBUG
        fprintf(stderr, "READ OVERFLOW @ __accmut__fread\n");
#endif

        memcpy(s, fp->read_cur, fp->bufend - fp->read_cur);
        size_t res = (fp->bufend - fp->read_cur) / size;
        fp->read_cur = fp->bufend;
        //fprintf(stderr, "%s\n", s);
        fp->flags |= _IO_EOF_SEEN;

        if (MUTATION_ID == 0) {
            char internalbuf[size * count];
            size_t internalres = fread(internalbuf, size, count, fp->orifile);
            if (res != internalres ||
                memcmp(buf, internalbuf, size * res) != 0) {
                ERRMSG("\033[31mNo sync with stdio\033[0m");
                ERRMSG("\033[33m");
                ERRMSG2("requested", count);
                ERRMSG2("real", res);
                ERRMSG2("internal", internalres);
                ERRMSG("\033[0m");
                exit(EXIT_FAILURE);
            }
        }
        return res;
    }
    memcpy(s, fp->read_cur, bytes_requested);
    fp->read_cur += bytes_requested;
    if (MUTATION_ID == 0) {
        char internalbuf[size * count];
        size_t internalres = fread(internalbuf, size, count, fp->orifile);

        if (count != internalres ||
            memcmp(buf, internalbuf, size * count) != 0) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            ERRMSG("\033[33m");
            ERRMSG2("requested", count);
            ERRMSG2("real", count);
            ERRMSG2("internal", internalres);
            ERRMSG("\033[0m");
            exit(EXIT_FAILURE);
        }
    }
    return count;
}

int __accmut__ungetc(int c, ACCMUT_FILE *fp) {
    CALLLOG(fp);

    if (c == EOF)
        return EOF;

    if (fp->flags != O_RDONLY) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "UNGETC OF NON-READABLE-FILE @ __accmut__ungetc, TID: %d, MUT: %d\n", TEST_ID, MUTATION_ID);
#endif
        return EOF;
    }

    int res;
    if (fp->read_cur > fp->bufbase
        && (unsigned char) fp->read_cur[-1] == (unsigned char) c) {
        (fp->read_cur)--;
        res = (unsigned char) c;
    } else
        res = EOF;

    if (res != EOF)
        fp->flags &= ~_IO_EOF_SEEN;

    if (MUTATION_ID == 0) {
        int r = ungetc(c, fp->orifile);
        if (r != res) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
    }

    return res;
}

/********************INNER DENINES AND FUNCTIONS FOR SCANF ***************************/
#define FL_LJUST    0x0001      /* left-justify field */
#define FL_SIGN     0x0002      /* sign in signed conversions */
#define FL_SPACE    0x0004      /* space in signed conversions */
#define FL_ALT      0x0008      /* alternate form */
#define FL_ZEROFILL 0x0010      /* fill with zero's */
#define FL_SHORT    0x0020      /* optional h */
#define FL_LONG     0x0040      /* optional l */
#define FL_LONGDOUBLE   0x0080      /* optional L */
#define FL_WIDTHSPEC    0x0100      /* field width is specified */
#define FL_PRECSPEC 0x0200      /* precision is specified */
#define FL_SIGNEDCONV   0x0400      /* may contain a sign */
#define FL_NOASSIGN 0x0800      /* do not assign (in scanf) */
#define FL_NOMORE   0x1000      /* all flags collected */

#define NUMLEN  512
#define NR_CHARS    256

static char Xtable[NR_CHARS];
static char inp_buf[NUMLEN];

static char *__accmut__o_collect(register int c, register ACCMUT_FILE *stream, char type,
                                 unsigned int width, int *basep) {
    register char *bufp = inp_buf;
    register int base;

    switch (type) {
        case 'i':   /* i means octal, decimal or hexadecimal */
        case 'p':
        case 'x':
        case 'X':
            base = 16;
            break;
        case 'd':
        case 'u':
            base = 10;
            break;
        case 'o':
            base = 8;
            break;
        case 'b':
            base = 2;
            break;
    }

    if (c == '-' || c == '+') {
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
    }

    if (width && c == '0' && base == 16) {
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
        if (c != 'x' && c != 'X') {
            if (type == 'i') base = 8;
        } else if (width) {
            *bufp++ = c;
            if (--width)
                c = __accmut__getc(stream);
        }
    } else if (type == 'i') base = 10;

    while (width) {
        if (((base == 10) && isdigit(c))
            || ((base == 16) && isxdigit(c))
            || ((base == 8) && isdigit(c) && (c < '8'))
            || ((base == 2) && isdigit(c) && (c < '2'))) {
            *bufp++ = c;
            if (--width)
                c = __accmut__getc(stream);
        } else break;
    }

    if (width && c != EOF) __accmut__ungetc(c, stream);
    if (type == 'i') base = 0;
    *basep = base;
    *bufp = '\0';
    return bufp - 1;
}


#ifndef NOFLOAT

/* The function f_collect() reads a string that has the format of a
 * floating-point number. The function returns as soon as a format-error
 * is encountered, leaving the offending character in the input. This means
 * that 1.el leaves the 'l' in the input queue. Since all detection of
 * format errors is done here, _doscan() doesn't call strtod() when it's
 * not necessary, although the use of the width field can cause incomplete
 * numbers to be passed to strtod(). (e.g. 1.3e+)
 */
static char *__accmut__f_collect(register int c, register ACCMUT_FILE *stream, register unsigned int width) {
    register char *bufp = inp_buf;
    int digit_seen = 0;

    if (c == '-' || c == '+') {
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
    }

    while (width && isdigit(c)) {
        digit_seen++;
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
    }
    if (width && c == '.') {
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
        while (width && isdigit(c)) {
            digit_seen++;
            *bufp++ = c;
            if (--width)
                c = __accmut__getc(stream);
        }
    }

    if (!digit_seen) {
        if (width && c != EOF) __accmut__ungetc(c, stream);
        return inp_buf - 1;
    } else digit_seen = 0;

    if (width && (c == 'e' || c == 'E')) {
        *bufp++ = c;
        if (--width)
            c = __accmut__getc(stream);
        if (width && (c == '+' || c == '-')) {
            *bufp++ = c;
            if (--width)
                c = __accmut__getc(stream);
        }
        while (width && isdigit(c)) {
            digit_seen++;
            *bufp++ = c;
            if (--width)
                c = __accmut__getc(stream);
        }
        if (!digit_seen) {
            if (width && c != EOF) __accmut__ungetc(c, stream);
            return inp_buf - 1;
        }
    }

    if (width && c != EOF) __accmut__ungetc(c, stream);
    *bufp = '\0';
    return bufp - 1;
}

#endif  /* NOFLOAT */

static int __accmut___doscan(register ACCMUT_FILE *stream, const char *format, va_list ap) {
    int done = 0;    /* number of items done */
    int nrchars = 0;    /* number of characters read */
    int conv = 0;    /* # of conversions */
    int base;        /* conversion base */
    unsigned long val;        /* an integer value */
    register char *str;        /* temporary pointer */
    char *tmp_string;    /* ditto */
    unsigned width = 0;    /* width of field */
    int flags;        /* some flags */
    int reverse;    /* reverse the checking in [...] */
    int kind;
    register int ic = EOF;    /* the input character */
#ifndef    NOFLOAT
    long double ld_val;
#endif

    if (!*format) return 0;

    while (1) {
        if (isspace(*format)) {
            while (isspace(*format))
                format++;    /* skip whitespace */
            ic = __accmut__getc(stream);
            nrchars++;
            while (isspace (ic)) {
                ic = __accmut__getc(stream);
                nrchars++;
            }
            if (ic != EOF) __accmut__ungetc(ic, stream);
            nrchars--;
        }
        if (!*format) break;    /* end of format */

        if (*format != '%') {
            ic = __accmut__getc(stream);
            nrchars++;
            if (ic != *format++) break;    /* error */
            continue;
        }
        format++;
        if (*format == '%') {
            ic = __accmut__getc(stream);
            nrchars++;
            if (ic == '%') {
                format++;
                continue;
            } else break;
        }
        flags = 0;
        if (*format == '*') {
            format++;
            flags |= FL_NOASSIGN;
        }
        if (isdigit (*format)) {
            flags |= FL_WIDTHSPEC;
            for (width = 0; isdigit (*format);)
                width = width * 10 + *format++ - '0';
        }

        switch (*format) {
            case 'h':
                flags |= FL_SHORT;
                format++;
                break;
            case 'l':
                flags |= FL_LONG;
                format++;
                break;
            case 'L':
                flags |= FL_LONGDOUBLE;
                format++;
                break;
        }
        kind = *format;
        if ((kind != 'c') && (kind != '[') && (kind != 'n')) {
            do {
                ic = __accmut__getc(stream);
                nrchars++;
            } while (isspace(ic));
            if (ic == EOF) break;        /* outer while */
        } else if (kind != 'n') {        /* %c or %[ */
            ic = __accmut__getc(stream);
            if (ic == EOF) break;        /* outer while */
            nrchars++;
        }
        switch (kind) {
            default:
                /* not recognized, like %q */
                return conv || (ic != EOF) ? done : EOF;
                break;
            case 'n':
                if (!(flags & FL_NOASSIGN)) {    /* silly, though */
                    if (flags & FL_SHORT)
                        *va_arg(ap, short *) = (short) nrchars;
                    else if (flags & FL_LONG)
                        *va_arg(ap, long *) = (long) nrchars;
                    else
                        *va_arg(ap, int *) = (int) nrchars;
                }
                break;
            case 'p':        /* pointer */
                //  set_pointer(flags);
                /* fallthrough */
            case 'b':        /* binary */
            case 'd':        /* decimal */
            case 'i':        /* general integer */
            case 'o':        /* octal */
            case 'u':        /* unsigned */
            case 'x':        /* hexadecimal */
            case 'X':        /* ditto */
                if (!(flags & FL_WIDTHSPEC) || width > NUMLEN)
                    width = NUMLEN;
                if (!width) return done;

                str = __accmut__o_collect(ic, stream, kind, width, &base);
                if (str < inp_buf
                    || (str == inp_buf
                        && (*str == '-'
                            || *str == '+')))
                    return done;

                /*
                 * Although the length of the number is str-inp_buf+1
                 * we don't add the 1 since we counted it already
                 */
                nrchars += str - inp_buf;

                if (!(flags & FL_NOASSIGN)) {
                    if (kind == 'd' || kind == 'i')
                        val = strtol(inp_buf, &tmp_string, base);
                    else
                        val = strtoul(inp_buf, &tmp_string, base);
                    if (flags & FL_LONG)
                        *va_arg(ap, unsigned long *) = (unsigned long) val;
                    else if (flags & FL_SHORT)
                        *va_arg(ap, unsigned short *) = (unsigned short) val;
                    else
                        *va_arg(ap, unsigned *) = (unsigned) val;
                }
                break;
            case 'c':
                if (!(flags & FL_WIDTHSPEC))
                    width = 1;
                if (!(flags & FL_NOASSIGN))
                    str = va_arg(ap, char *);
                if (!width) return done;

                while (width && ic != EOF) {
                    if (!(flags & FL_NOASSIGN))
                        *str++ = (char) ic;
                    if (--width) {
                        ic = __accmut__getc(stream);
                        nrchars++;
                    }
                }

                if (width) {
                    if (ic != EOF) __accmut__ungetc(ic, stream);
                    nrchars--;
                }
                break;
            case 's':
                if (!(flags & FL_WIDTHSPEC))
                    width = 0xffff;
                if (!(flags & FL_NOASSIGN))
                    str = va_arg(ap, char *);
                if (!width) return done;

                while (width && ic != EOF && !isspace(ic)) {
                    if (!(flags & FL_NOASSIGN))
                        *str++ = (char) ic;
                    if (--width) {
                        ic = __accmut__getc(stream);
                        nrchars++;
                    }
                }
                /* terminate the string */
                if (!(flags & FL_NOASSIGN))
                    *str = '\0';
                if (width) {
                    if (ic != EOF) __accmut__ungetc(ic, stream);
                    nrchars--;
                }
                break;
            case '[':
                if (!(flags & FL_WIDTHSPEC))
                    width = 0xffff;
                if (!width) return done;

                if (*++format == '^') {
                    reverse = 1;
                    format++;
                } else
                    reverse = 0;

                for (str = Xtable; str < &Xtable[NR_CHARS]; str++)
                    *str = 0;

                if (*format == ']') Xtable[*format++] = 1;

                while (*format && *format != ']') {
                    Xtable[*format++] = 1;
                    if (*format == '-') {
                        format++;
                        if (*format
                            && *format != ']'
                            && *(format) >= *(format - 2)) {
                            int c;

                            for (c = *(format - 2) + 1; c <= *format; c++)
                                Xtable[c] = 1;
                            format++;
                        } else Xtable['-'] = 1;
                    }
                }
                if (!*format) return done;

                if (!(Xtable[ic] ^ reverse)) {
                    /* MAT 8/9/96 no match must return character */
                    __accmut__ungetc(ic, stream);
                    return done;
                }

                if (!(flags & FL_NOASSIGN))
                    str = va_arg(ap, char *);

                do {
                    if (!(flags & FL_NOASSIGN))
                        *str++ = (char) ic;
                    if (--width) {
                        ic = __accmut__getc(stream);
                        nrchars++;
                    }
                } while (width && ic != EOF && (Xtable[ic] ^ reverse));

                if (width) {
                    if (ic != EOF) __accmut__ungetc(ic, stream);
                    nrchars--;
                }
                if (!(flags & FL_NOASSIGN)) {    /* terminate string */
                    *str = '\0';
                }
                break;
#ifndef    NOFLOAT
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                if (!(flags & FL_WIDTHSPEC) || width > NUMLEN)
                    width = NUMLEN;

                if (!width) return done;
                str = __accmut__f_collect(ic, stream, width);

                if (str < inp_buf
                    || (str == inp_buf
                        && (*str == '-'
                            || *str == '+')))
                    return done;

                /*
                 * Although the length of the number is str-inp_buf+1
                 * we don't add the 1 since we counted it already
                 */
                nrchars += str - inp_buf;

                if (!(flags & FL_NOASSIGN)) {
                    ld_val = strtod(inp_buf, &tmp_string);
                    if (flags & FL_LONGDOUBLE)
                        *va_arg(ap, long double *) = (long double) ld_val;
                    else if (flags & FL_LONG)
                        *va_arg(ap, double *) = (double) ld_val;
                    else
                        *va_arg(ap, float *) = (float) ld_val;
                }
                break;
#endif
        }        /* end switch */
        conv++;
        if (!(flags & FL_NOASSIGN) && kind != 'n') done++;
        format++;
    }
    return conv || (ic != EOF) ? done : EOF;
}


int __accmut__fscanf(ACCMUT_FILE *fp, const char *format, ...) {
    CALLLOG(fp);
    if (fp->flags & _IO_EOF_SEEN != 0) {
        return EOF;
    }

    if (fp->read_cur - fp->bufbase >= fp->fsize) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "READ OVERFLOW @ __accmut__getc, TID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        fp->flags |= _IO_EOF_SEEN;
        return EOF;
    }

    va_list ap;
    int retval;

    va_start(ap, format);

    retval = __accmut___doscan(fp, format, ap);

    va_end(ap);

    if (MUTATION_ID == 0) {
        va_start(ap, format);
        int r = vfscanf(fp->orifile, format, ap);
        va_end(ap);
        if (r != retval) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
    }

    return retval;
}

/*********************** OUTPUT ****************************************/

int __accmut__fputc(int c, ACCMUT_FILE *fp) {
    CALLLOG(fp);

    if (fp->write_cur >= fp->bufend) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "OUTPUT OVERFLOW @ __accmut__fputc, ID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        //TODO: need something to fflush
        return EOF;
    }
    // fp->fsize++;
    *(fp->write_cur) = c;
    (fp->write_cur)++;
    if (MUTATION_ID == 0) {
        if (fputc(c, fp->orifile) != c) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
        fflush(fp->orifile);
    }
    return (unsigned char) c;
}


int __accmut__fputs(const char *s, ACCMUT_FILE *fp) {
    CALLLOG(fp);
    int result = EOF;
    size_t len = strlen(s);
    if (fp->write_cur + len >= fp->bufend) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "OUTPUT OVERFLOW @ __accmut__fputs, TID: %d, MUT: %d\n", TEST_ID, MUTATION_ID);
#endif
    } else {
        memcpy(fp->write_cur, s, len);
        fp->write_cur += len;
        // fp->fsize += len;
        result = 1;
        if (MUTATION_ID == 0) {
            if (fputs(s, fp->orifile) == EOF) {
                ERRMSG("\033[31mNo sync with stdio\033[0m");
                exit(EXIT_FAILURE);
            }
            fflush(fp->orifile);
        }
    }
    return result;
}

int __accmut__puts(const char *s) {
    CALLLOG(accmut_stdout);
    int result = EOF;
    size_t len = strlen(s);
    if ((accmut_stdout->write_cur + len + 1) >= accmut_stdout->bufend) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "STDOUT OVERFLOW @ __accmut__puts, TID: %d, MUT: %d\n", TEST_ID, MUTATION_ID);
#endif
    } else {
        memcpy(accmut_stdout->write_cur, s, len);
        accmut_stdout->write_cur += len;
        *accmut_stdout->write_cur = '\n';
        accmut_stdout->write_cur++;
        //*accmut_stdout->write_cur = '\0';
        //accmut_stdout->write_cur++;
        // accmut_stdout->fsize += len + 1;
        result = 1;
        if (MUTATION_ID == 0) {
            if (puts(s) == EOF) {
                ERRMSG("\033[31mNo sync with stdio\033[0m");
                exit(EXIT_FAILURE);
            }
            fflush(stdout);
        }
    }
    return result;
}


//ORI fprintf
// int __accmut__fprintf(FILE *stream, const char *format, ...){
// 	int ret;
// 	va_list ap;
// 	va_start(ap, format);
// 	ret = vfprintf(stream, format, ap);
// 	va_end(ap);
// 	return 0;
// }

int __accmut__fprintf(ACCMUT_FILE *fp, const char *format, ...) {
    CALLLOG(fp);

    if (fp == NULL) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "NULL ACCMUT FILE  !!! @__accmut__fprintf, TID: %d, MUT: %d\n", TEST_ID, MUTATION_ID);
#endif
        return 0;
    }

    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsprintf(fp->write_cur, format, ap);    //TODO:: use (STDOUT_BUFF + CUR_STDOUT) instead of tmp
    va_end(ap);

    int max;
    switch (fp->fd) {
        case 0:
            max = 0;
            break;
        case 1:
            max = MAX_STDOUT_BUF_SIZE;
            break;
        case 2:
            max = MAX_STDERR_BUF_SIZE;
            break;
        default:
            max = MAX_FILE_BUF_SIZE;
            break;
    }

    if ((fp->write_cur - fp->bufbase) + ret > max) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "ACCMUT BUFFER OVERFLOW !  @__accmut__fprintf. TID:%d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        return 0;
    }
    // ERRMSG("???");
    if (MUTATION_ID == 0) {
        // ERRMSG("???1");
        va_start(ap, format);
        if (ret != vfprintf(fp->orifile, format, ap)) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
        fflush(fp->orifile);
        va_end(ap);
    }
    // ERRMSG("???");

    fp->write_cur += ret;
    return ret;
}

int __accmut__printf(const char *format, ...) {
    CALLLOG(accmut_stdout);
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsprintf(accmut_stdout->write_cur, format, ap);    //TODO:: use (STDOUT_BUFF + CUR_STDOUT) instead of tmp
    va_end(ap);

    if ((accmut_stdout->write_cur - accmut_stdout->bufbase) + ret > MAX_STDOUT_BUF_SIZE) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "ACCMUT STDOUT BUF OVERFLOW !  @__accmut__printf, TID: %d, MUT: %d\n", TEST_ID, MUTATION_ID);
#endif
        return 0;
    }

    if (MUTATION_ID == 0) {
        va_start(ap, format);
        if (ret != vprintf(format, ap)) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
        va_end(ap);
        fflush(stdout);
    }

    accmut_stdout->write_cur += ret;
    return ret;
}

size_t __accmut__fwrite(const void *buf, size_t size, size_t count, ACCMUT_FILE *fp) {
    CALLLOG(fp);
    size_t request = size * count;
    if (fp->write_cur + request > fp->bufend) {
#if ACCMUT_IO_DEBUG
        fprintf(stderr, "ACCMUT WRITE OVERFLOW !  @__accmut__fwrite, TID: %d, MUT: %d, fd: %d\n", TEST_ID, MUTATION_ID, fp->fd);
#endif
        return 0;
    }
    memcpy(fp->write_cur, buf, request);
    fp->write_cur += request;
    if (MUTATION_ID == 0) {
        if (request != fwrite(buf, size, count, fp->orifile)) {
            ERRMSG("\033[31mNo sync with stdio\033[0m");
            exit(EXIT_FAILURE);
        }
        fflush(fp->orifile);
    }
    return count;
}

/*********************** BUF UTILS ****************************************/


/*
void __accmut__filedump(ACCMUT_FILE *fp);
void __accmut__oracledump();
int __accmut__checkoutput(){
    if(ORACLESIZE != CUR_STDOUT){
        return 1;
    }
    return memcmp(STDOUT_BUFF, ORACLEBUFF, CUR_STDOUT);
}
void __accmut__exit_check_output(){
    int res = __accmut__checkoutput();
    if(res != 0){
        fprintf(stderr, "TEST: %d KILL MUT: %d\n", TEST_ID, MUTATION_ID);
        //__accmut__filedump();

    }

    // if(MUTATION_ID == 0){
    // 	__accmut__oracledump();
    // }
}
void __accmut__oracal_bufinit(){
    char path[120];
    sprintf(path, "%st%d", ORACLEDIR, TEST_ID);
    int fd = open(path, O_RDONLY);
    if(fd == -1){
        fprintf(stderr, "ORACLEDIR OPEN ERROR !!!!!!\n");
        fprintf(stderr, "ORACLEDIR PATH : %s\n", path);
    }
    struct stat sb;
    if(fstat(fd, &sb) == -1){
        fprintf(stderr, "fstat ERROR !!!!!!\n");
    }
    ORACLEBUFF = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(ORACLEBUFF == MAP_FAILED){
        fprintf(stderr, "mmap ERROR !!!!!!\n");
    }
    ORACLESIZE = sb.st_size;

    //regist the exit handler function of a process
    if(atexit(__accmut__exit_check_output) != 0){
        fprintf(stderr, "____accmut__exit_check_output REGSITER ERROR\n");
        exit(0);
    }

    // if(atexit(__accmut__exit_time) != 0){
    // 	fprintf(stderr, "__accmut__exit_time REGSITER ERROR\n");
    // }
}*/


void __accmut__setout(int id) {
    char path[120];
    strcpy(path, getenv("HOME"));
    strcat(path, "/tmp/accmut/output/");
    strcat(path, PROJECT);
    strcat(path, "/t");
    sprintf(path, "%s%d_%d", path, TEST_ID, id);
    //printf("PATH : %s\n", path);
    if (freopen(path, "w", stdout) == NULL) {
        fprintf(stderr, "STDOUT REDIR ERR! : %s\n", path);
    }
}

/*
void __accmut__oracledump(){
    fprintf(stderr, "\n********** TID:%d  ORI BUFFER SIZE: %d ***********\n", TEST_ID, ORACLESIZE);
    int i;
    for(i = 0; i < ORACLESIZE; i++){
        fprintf(stderr, "%c", ORACLEBUFF[i]);
    }
    fprintf(stderr, "\n************ END OF ORACLE ***************\n\n");
}
void __accmut__filedump(ACCMUT_FILE *fp){

    if(fp == NULL){
        fprintf(stderr, "NULL ACCMUT_FILE !!! @__accmut__filedump\n");
        exit(0);
    }

    size_t sz;
    if(fp->flags == O_RDONLY){
        sz = fp->fsize;
    }else if(fp->flags == O_WRONLY){
        sz = fp->write_cur - fp->bufbase;
    }else{
        fprintf(stderr, "ERROR FILE FLAGS @ __accmut__filedump\n");
        exit(0);
    }

    fprintf(stderr, "\n********** TID:%d  MID:%d  FD:%d  SIZE:%d ***********\n", \
		TEST_ID, MUTATION_ID, fp->fd, sz);

    int i;
    for(i = 0; i < sz; i++){
        fprintf(stderr, "%c", *(fp->bufbase + i) );
    }

    fprintf(stderr, "\n**** END **** TID:%d  MID:%d  FD:%d ***************\n\n", \
		TEST_ID, MUTATION_ID, fp->fd);
}*/

void __accmut__perror(const char *s) {
    CALLLOG();
    __accmut__fprintf(accmut_stderr, "%s\n", s);
}

int __accmut__fflush(ACCMUT_FILE *stream) {
    CALLLOG(stream);
    //all in memory, do not need flush
    return 0;
}
