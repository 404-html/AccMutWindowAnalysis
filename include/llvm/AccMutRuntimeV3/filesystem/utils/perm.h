//
// Created by Sirui Lu on 2019-04-10.
//

#ifndef LLVM_PERM_H
#define LLVM_PERM_H


inline bool check_read_perm(struct stat &s) {
    if (getuid() == s.st_uid) {
        return s.st_mode & S_IRUSR;
    } else if (getgid() == s.st_gid) {
        return s.st_mode & S_IRGRP;
    } else {
        return s.st_mode & S_IROTH;
    }
}

inline bool check_write_perm(struct stat &s) {
    if (getuid() == s.st_uid) {
        return s.st_mode & S_IWUSR;
    } else if (getgid() == s.st_gid) {
        return s.st_mode & S_IWGRP;
    } else {
        return s.st_mode & S_IWOTH;
    }
}

inline bool check_execute_perm(struct stat &s) {
    if (getuid() == s.st_uid) {
        return s.st_mode & S_IXUSR;
    } else if (getgid() == s.st_gid) {
        return s.st_mode & S_IXGRP;
    } else {
        return s.st_mode & S_IXOTH;
    }
}

#endif //LLVM_PERM_H
