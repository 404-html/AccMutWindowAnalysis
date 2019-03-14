//
// Created by Sirui Lu on 2019-03-14.
//
#include "testinit.h"

TEST(open,
     RUN(
             auto
             fd = fopen("test-open.txt", "w");
             fputs("abc", fd);
             check();
             fclose(fd);
     ),
     RUN(
             system("rm -f test-open.txt");
     )
)
