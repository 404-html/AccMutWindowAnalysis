//
// Created by Sirui Lu on 2019-03-14.
//

#ifndef LLVM_TESTDEF_H
#define LLVM_TESTDEF_H

#define TEST(__name, __prog, __cleanup) \
template <int __test_tplarg> \
struct __name ## _test {\
    ALL_ADDED()\
    inline void cleanup() {\
        __cleanup;\
    }\
    inline void check() {\
        if (__test_tplarg == 1) {\
            __accmutv2__checkfd();\
        }\
    }\
    inline void operator()() {\
        __prog;\
    }\
};\
struct __name ## _runner {\
    inline __name ## _runner() {\
        fprintf(stderr, "---- Start running test %s\n", #__name);\
        fprintf(stderr, "---- Running ori\n");\
        __name ## _test<0> oritest;\
        pid_t pid = fork();\
        int status;\
        int retst;\
        if (pid == 0) {\
            /* child */\
            oritest.cleanup();\
            oritest();\
            exit(0);\
        }\
        wait(&status);\
        if (WIFEXITED(status)) {\
            if ((retst = WEXITSTATUS(status)) != 0) {\
                fprintf(stderr, "**** ori not exited normally with %d\n", retst);\
            }\
        } else if (WIFSIGNALED(status)) {\
            fprintf(stderr, "**** ori not exited normally with signal %d\n", WTERMSIG(status));\
        }\
        fprintf(stderr, "---- Running new\n");\
        __name ## _test<1> newtest;\
        pid = fork();\
        if (pid == 0) {\
            newtest.cleanup();\
            newtest();\
            exit(0);\
        }\
        wait(&status);\
        if (WIFEXITED(status)) {\
            if ((retst = WEXITSTATUS(status)) != 0) {\
                fprintf(stderr, "**** ori not exited normally with %d\n", retst);\
            }\
        } else if (WIFSIGNALED(status)) {\
            fprintf(stderr, "**** ori not exited normally with signal %d\n", WTERMSIG(status));\
        }\
        fprintf(stderr, "---- Finish running test %s\n", #__name);\
    }\
};


#endif //LLVM_TESTDEF_H
