//
// Created by Sirui Lu on 2019-03-20.
//

#include <llvm/AccMutRuntimeV2/fs/accmut_fs_simulate.h>
#include <map>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

std::map<std::string, ino_t> &get_s2i() {
    static std::map<std::string, ino_t> _s2i;
    return _s2i;
}

std::map<ino_t, std::shared_ptr<mem_file>> &get_i2f() {
    static std::map<ino_t, std::shared_ptr<mem_file>> _i2f;
    return _i2f;
}

#define s2i get_s2i()
#define i2f get_i2f()


ino_t nowino = ~((ino_t) 0);

char pathbuf[PATH_MAX];

static char *get_enclosing_dir(const char *str, char *out) {
    char _pathbuf[PATH_MAX];
    strcpy(_pathbuf, str);
    char *pos = strrchr(_pathbuf, '/');
    if (pos != NULL)
        *pos = 0;
    return realpath(str, out);
}

static int fs_cache(const char *str) {
    if (get_enclosing_dir(str, pathbuf) == NULL) {
        return -1;
    }
    const char *pos = strrchr(str, '/');
    strcat(pathbuf, pos);
    fprintf(stderr, "%s\n", pathbuf);
    struct stat sb;
    if (lstat(pathbuf, &sb) < 0) {
        return -1;
    }

}

static int _fs_delete(const char *str, int(*stat_func)(const char *, struct stat *)) {
    fprintf(stderr, "delete %s\n", str);
    auto iter = s2i.find(str);
    if (iter == s2i.end()) {
        // find on real file sys and mark as deleted
        struct stat buf;
        if (stat_func(str, &buf) < 0) {
            // error finding on real file sys
            return -1;
        }
        // ino_t ino = buf.st_ino;
        s2i[str] = ACCMUT_DELETED_INO;
        // i2v[ino] = nullptr;
        return 0;
    } else {
        // delete in memory file sys
        s2i[str] = ACCMUT_DELETED_INO;
        // i2f[iter->second] = nullptr;
        return 0;
    }
}

int fs_ldelete(const char *str) {
    return _fs_delete(str, lstat);
}

int fs_delete(const char *str) {
    return _fs_delete(str, stat);
}

int fs_rename(const char *ori, const char *after) {
    auto iter = s2i.find(ori);
    if (iter == s2i.end()) {
        // find on real file sys, mark ori as deleted
        struct stat buf;
        if (lstat(ori, &buf) < 0) {
            return -1;
        }
        ino_t ino = buf.st_ino;
        s2i[ori] = ACCMUT_DELETED_INO;
        s2i[after] = ino;
    } else {
        s2i[after] = s2i[ori];
        s2i[ori] = ACCMUT_DELETED_INO;
    }
    return 0;
}

std::shared_ptr<mem_file> fs_query(ino_t ino, int flags) {
    auto iter = i2f.find(ino);
    if (ino == ACCMUT_DELETED_INO)
        goto notfound;
    if (iter == i2f.end()) {
        goto notfound;
    } else {
        if (flags & O_TRUNC) {
            iter->second->size = 0;
        }
        return iter->second;
    }
    notfound:
    if (flags & O_CREAT) {
        iter->second = std::make_shared<mem_file>();
        return iter->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<mem_file> fs_query(const char *str, int flags) {
    auto siter = s2i.find(str);
    if (siter == s2i.end()) {
        // not found in simulated fs
        int fd = open(str, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "FAIL open\n");
            goto fail;
        }
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            fprintf(stderr, "FAIL fstat\n");
            close(fd);
            goto fail;
        }
        auto file = std::make_shared<mem_file>();
        auto buf = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!buf) {
            fprintf(stderr, "FAIL mmap\n");
            close(fd);
            goto fail;
        }
        file->data.resize(sb.st_size + 10);
        memcpy(file->data.data(), buf, sb.st_size);
        file->size = sb.st_size;
        munmap(buf, sb.st_size);
        close(fd);
        s2i[str] = sb.st_ino;
        i2f[sb.st_ino] = file;
        return file;
    } else {
        // found in simulated fs
        fprintf(stderr, "IN SIMULATED %lld\n", siter->second);
        return fs_query(siter->second, flags);
    }

    fail:
    s2i[str] = ACCMUT_DELETED_INO;
    return nullptr;
}

bool fs_have_cache(const char *str) {
    return s2i.find(str) != s2i.end();
}

ino_t fs_cached_ino(const char *str) {
    auto iter = s2i.find(str);
    if (iter == s2i.end()) {
        return ACCMUT_DELETED_INO;
    }
    return iter->second;
}

void fs_link(const char *path, const char *path2) {
    // assume fs_have_cache(path)
    s2i[path2] = s2i[path];
}

