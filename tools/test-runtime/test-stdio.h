//
// Created by Sirui Lu on 2019-03-14.
//
#include "testinit.h"

TEST(stdio_positioning,
     RUN(
             int
             ret;
             char *retp;
             auto fd = fopen("test-stdio1.txt", "w+");
             char buf[100];
             check_nonnull(fd);
             ret = fputs("abcde", fd);
             check_neq(ret, EOF);
             ret = ftell(fd);
             check_eq(ret, 5);
             ret = fseek(fd, 0, SEEK_CUR);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 5);
             ret = fseek(fd, 0, SEEK_SET);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 0);
             ret = fseek(fd, 0, SEEK_END);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 5);
             ret = fputs("fghij", fd);
             check_neq(ret, EOF);
             ret = ftell(fd);
             check_eq(ret, 10);
             ret = fseek(fd, 0, SEEK_CUR);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 10);
             ret = fseek(fd, 0, SEEK_SET);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 0);
             ret = fseek(fd, 0, SEEK_END);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 10);
             ret = fseek(fd, 3, SEEK_SET);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 3);
             check();
             ret = fputs("12345", fd);
             check_neq(ret, EOF);
             ret = fseek(fd, 0, SEEK_END);
             check_eq(ret, 0);
             ret = ftell(fd);
             check_eq(ret, 10);
             check();
             rewind(fd);
             fclose(fd);
             fd = fopen("test-stdio1.txt", "a+");
             rewind(fd);
             check_nonnull(fd);
             retp = fgets(buf, 10, fd);
             check_eq((long) retp, (long) buf);

             check_eq(strlen(buf), 9);
             check_mem(buf, "abc12345i\0", 10);

             rewind(fd);
             check_nonnull(fd);
             retp = fgets(buf, 100, fd);
             check_eq((long) retp, (long) buf);
             check_eq(strlen(buf), 10);
             check_mem(buf, "abc12345ij", 10);
             fclose(fd);
             check();
     ),
     RUN(
             unlink("test-stdio1.txt");
     )
)

TEST(stdio_file_status,
     RUN(
             int
             ret;
             char *retp;
             auto fd = fopen("test-stdio2.txt", "w+");
             char buf[100];
             ret = fputs("1234567890", fd);
             check_neq(ret, EOF);
             ret = feof(fd);
             check_eq(ret, 0);
             retp = fgets(buf, 100, fd);
             ret = feof(fd);
             check_eq(ret, 1);
             fclose(fd);
     ),
     RUN(
             unlink("test-stdio2.txt");
     )
)

TEST(stdio_unix_interoperate,
     RUN(
             int
             ret;
             int fd;
             auto fp = fopen("test-stdio3.txt", "w+");
             char buf[100];
             fd = fileno(fp);
             check_eq(fd, 3);
             ret = write(fd, "abcd", 4);
             check_eq(ret, 4);
             ret = fputs("1234", fp);
             check_eq(ret, 4);
             check();
             rewind(fp);
             fgets(buf, 100, fp);
             check_mem(buf, "abcd1234\0", 9);
             rewind(fp);
             memset(buf, 0, sizeof(buf));
             read(fd, buf, 100);
             check_mem(buf, "abcd1234\0", 9);

             check();
             ret = fclose(fp);
             check_eq(ret, 0);
             check();
     ),
     RUN(
             unlink("test-stdio3.txt");
     )
)

TEST(stdio_input,
     RUN(
             int
             ret;
             auto fp = fopen("test-stdio4.txt", "w+");
             char buf[100];
             ret = fputs("abcdefghijklmnopqrstuvwxyz", fp);
             check_neq(ret, EOF);

             ret = ungetc('1', fp);
             check_eq(ret, '1');
             fgets(buf, 8, fp);
             check_streq(buf, "1");
             check();

             rewind(fp);
             ret = ungetc('1', fp);
             check_eq(ret, '1');
             fgets(buf, 8, fp);
             check_streq(buf, "1abcdef");
             check();

             rewind(fp);
             ret = ftell(fp);
             check_eq(ret, 0);
             ret = ungetc('1', fp);
             check_eq(ret, '1');
             fgets(buf, 8, fp);
             check_streq(buf, "1abcdef");
             check();

             rewind(fp);
             ret = ungetc('1', fp);
             check_eq(ret, '1');
             ret = fseek(fp, 0, SEEK_CUR);
             check_eq(ret, -1);
             ret = fseek(fp, 0, SEEK_SET);
             check_eq(ret, 0);
             fgets(buf, 8, fp);
             check_streq(buf, "abcdefg");
             check();

             rewind(fp);
             ungetc('1', fp);
             ret = getc(fp);
             check_eq(ret, '1');
             ret = getc(fp);
             check_eq(ret, 'a');
             ungetc('1', fp);
             ret = getc(fp);
             check_eq(ret, '1');
             ret = getc(fp);
             check_eq(ret, 'b');
             check();

             fclose(fp);
     ),
     RUN(
             unlink("test-stdio4.txt");
     )
)
