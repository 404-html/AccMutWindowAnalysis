//
// Created by Sirui Lu on 2019-03-14.
//
#include "testinit.h"

TEST(stdio_positioning,
     RUN(
             int
             ret;
             auto fd = fopen("test-stdio1.txt", "w+");
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
             fclose(fd);
             check();
     ),
     RUN(
             unlink("test-stdio1.txt");
     )
)
