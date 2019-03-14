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
