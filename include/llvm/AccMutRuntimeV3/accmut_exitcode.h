#ifndef ACCMUT_ERRNO_H
#define ACCMUT_ERRNO_H

//exit code

enum ACCMUT_EXIT_CODE {
    SUCC = 0,
    ENV_ERR,
    ILL_STATE_ERR,
    MUT_TP_ERR,
    MELLOC_ERR,
    FOPEN_ERR,
    OPCD_ERR,
    TIMEOUT_ERR,
    SIGSEGV_ERR,
    SIGABRT_ERR,
    SIGFPE_ERR

};

#endif

