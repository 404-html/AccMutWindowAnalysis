//
// Created by Sirui Lu on 2019-03-14.
//

#ifndef LLVM_TEST_UNIX_H
#define LLVM_TEST_UNIX_H

#include "testinit.h"

TEST(unix,
     RUN(
             int
             ret;
             char buf[100];
             int fd = open("test-unix1.txt", O_RDWR | O_CREAT, 0666);
             check_eq(fd, 3);
             ret = write(fd, "abcd\n", 5);
             check_eq(ret, 5);
             ret = write(fd, "efgh\n", 5);
             check_eq(ret, 5);
             ret = lseek(fd, 0, SEEK_CUR);
             check_eq(ret, 10);
             ret = lseek(fd, -5, SEEK_SET);
             check_eq(ret, -1);
             ret = lseek(fd, 0, SEEK_CUR);
             check_eq(ret, 10);
             ret = lseek(fd, 0, SEEK_END);
             check_eq(ret, 10);
             ret = lseek(fd, 5, SEEK_END);
             check_eq(ret, 15);
             ret = write(fd, "qwer\n", 5);
             check_eq(ret, 5);
             ret = lseek(fd, 0, SEEK_SET);
             check_eq(ret, 0);
             ret = read(fd, buf, 10);
             check_eq(ret, 10);
             check_mem(buf, "abcd\nefgh\n", 10);
             ret = read(fd, buf, 5);
             check_eq(ret, 5);
             check_mem(buf, "\0\0\0\0\0", 5);
             ret = read(fd, buf, 10);
             check_eq(ret, 5);
             check_mem(buf, "qwer\n", 5);
             ret = read(fd, buf, 10);
             check_eq(ret, 0);
             check();
             ret = close(fd);
             check_eq(ret, 0);
             fd = open("test-unix1.txt", O_RDONLY);
             check_eq(fd, 3);
             ret = write(fd, "abcd\n", 5);
             check_eq(ret, -1);
             ret = read(fd, buf, 40);
             check_eq(ret, 20);
             check_mem(buf, "abcd\nefgh\n\0\0\0\0\0qwer\n", 20);
             check();
             ret = close(3);
             check_eq(ret, 0);
             fd = creat("test-unix1.txt", 0666);
             check_eq(fd, 3);
             check();
             ret = close(3);
             check_eq(ret, 0);
             fd = open("test-unix1.txt", O_RDONLY);
             check_eq(fd, 3);
             ret = read(fd, buf, 10);
             check_eq(ret, 0);
             check();
             ret = close(fd);
             check_eq(ret, 0);
     ),
     RUN(
             unlink("test-unix1.txt");
     )
)

#endif //LLVM_TEST_UNIX_H
