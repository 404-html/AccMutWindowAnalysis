//
// Created by Sirui Lu on 2019-03-14.
//
#include "testinit.h"

TEST(stdio,
     RUN(
             auto
             fd = fopen("test-fopen.txt", "w");
             fputs("abc", fd);
             check();
             fclose(fd);
     ),
     RUN(
             system("rm -f test-fopen.txt");
     )
)
