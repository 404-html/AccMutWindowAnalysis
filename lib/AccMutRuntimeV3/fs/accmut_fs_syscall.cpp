//
// Created by Sirui Lu on 2019-03-22.
//

#include <llvm/AccMutRuntimeV2/fs/accmut_fs_syscall.h>
#include <llvm/AccMutRuntimeV2/fs/accmut_fs_simulate.h>
#include <fcntl.h>

static char name_buf[PATH_MAX];

int __accmutv2__rename(const char *old, const char *newname) {
    int ret = fs_rename(old, newname);
    int ori_ret = only_origin(::rename(old, newname));
    check_eq(ori_ret, ret);
    return ret;
}

int __accmutv2__unlink(const char *path) {
    int ret = fs_ldelete(path);
    int ori_ret = only_origin(::remove(path));
    check_eq(ori_ret, ret);
    return ret;
}

int __accmutv2__remove(const char *path) {
    // TODO: dir not handled
    return __accmutv2__unlink(path);
}

int __accmutv2__link(const char *path, const char *path2) {
    int ret;
    if (fs_have_cache(path)) {
        if (fs_cached_ino(path) != ACCMUT_DELETED_INO) {
            fs_link(path, path2);
            ret = 0;
        }
    } else {
        char *t = realpath(path, name_buf);
        if (!t) {
            errno = EACCES;
            ret = -1;
            goto check;
        }
        auto ptr = fs_query(name_buf, 0);
        if (!ptr) {
            // error in query, can't open file
            errno = EACCES;
            ret = -1;
            goto check;
        }
        fs_link(name_buf, path);
        fs_link(name_buf, path2);
        ret = 0;
    }

    check:
    int ori_ret = only_origin(::link(path, path2));
    check_eq(ori_ret, ret);
    return ret;
}

int __accmutv2__access(const char *path, int mode) {
    if (mode != F_OK) {
        check_assert(false, "access now only supports F_OK\n");
        exit(-1);
    }
    int ret;
    if (!fs_have_cache(path)) {
        fs_query(path, mode);
    }
    if (fs_cached_ino(path) != ACCMUT_DELETED_INO)
        ret = 0;
    else
        ret = -1;
    int ori_ret = only_origin(::access(path, mode));
    check_eq(ret, ori_ret);
    return ret;
}
