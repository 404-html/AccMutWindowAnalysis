//
// Created by Sirui Lu on 2019-03-14.
//

#ifndef LLVM_TESTDEF_H
#define LLVM_TESTDEF_H

#define ADD_FUNC(type, name) type<__test_tplarg> name

#define TEST(__name, __prog, __cleanup) \
template <int __test_tplarg> \
struct __name ## _test {\
    ALL_ADDED();\
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
struct __name ## runner {\
    inline __name ## runner() {\
        fprintf(stderr, "---- Start running test %s\n", #__name);\
        fprintf(stderr, "---- Running ori\n");\
        __name ## _test<0> oritest;\
        pid_t pid = fork();\
        int status;\
        if (pid != 0) {\
            /* child */\
            oritest.cleanup();\
            oritest();\
            exit(0);\
        }\
        wait(&status);\
        if (!WIFEXITED(status))\
            fprintf(stderr, "**** ori not exited normally\n");\
        fprintf(stderr, "---- Running new\n");\
        __name ## _test<1> newtest;\
        pid = fork();\
        if (pid != 0) {\
            newtest.cleanup();\
            newtest();\
            exit(0);\
        }\
        wait(&status);\
        if (!WIFEXITED(status))\
            fprintf(stderr, "**** new not exited normally\n");\
        fprintf(stderr, "---- Finish running test %s\n", #__name);\
    }\
};


#endif //LLVM_TESTDEF_H
