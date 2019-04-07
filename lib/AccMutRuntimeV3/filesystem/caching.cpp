//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/caching.h>
#include <llvm/AccMutRuntimeV3/filesystem/inode.h>
#include <llvm/AccMutRuntimeV3/checking/panic.h>
#include <llvm/AccMutRuntimeV3/filesystem/DirectoryFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/RegularFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/SymbolicLinkFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/UnsupportedFile.h>

#include <unistd.h>
#include <sys/param.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <dirent.h>

static char cwdbuff[MAXPATHLEN];
static std::map<ino_t, std::shared_ptr<inode>> inomap;

// use cwdbuff
static std::vector<std::string> split_str(const char *str) {
    char buf[MAXPATHLEN];
    strcpy(buf, str);
    char *b = buf;
    char *e = b;
    std::vector<std::string> ret;
    while (*e != 0) {
        if (*e == '/') {
            if (b == e) {
                b = ++e;
                continue;
            }
            *e = 0;
            ret.emplace_back(b);
            b = ++e;
        } else {
            ++e;
        }
    }
    if (b != e)
        ret.emplace_back(b);
    return ret;
}

static ino_t cache_path(const char* path) {
    struct stat st;
    if (lstat(path, &st) < 0)
        return 0;
    if (inomap.find(st.st_ino) != inomap.end())
        return st.st_ino;
    // assume no write only files...
    if (S_ISREG(st.st_mode)) {
        int fd = open(path, O_RDONLY);
        if (!fd)
            return 0;
        auto buf = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!buf) {
            close(fd);
            return 0;
        }
        std::vector<char> tmp;
        tmp.resize(st.st_size + 10);
        memcpy(tmp.data(), buf, st.st_size);
        auto prf = std::make_shared<RegularFile>(std::move(tmp), st.st_size);
        inomap[st.st_ino] = std::make_shared<inode>(st, std::move(prf));
    } else if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir)
            return 0;
        std::list<dirent> dlist;
        struct dirent* pdir;
        while ((pdir = readdir(dir)) != nullptr) {
            dlist.push_back(*pdir);
        }
        closedir(dir);
        auto pdf = std::make_shared<DirectoryFile>
                (std::move(dlist));
        inomap[st.st_ino] = std::make_shared<inode>(st, std::move(pdf));
    } else if (S_ISLNK(st.st_mode)) {
        char buf[MAXPATHLEN];
        ssize_t n = readlink(path, buf, MAXPATHLEN);
        if (n == -1)
            return 0;
        buf[n] = 0;
        inomap[st.st_ino] = std::make_shared<inode>(st,
                std::make_shared<SymbolicLinkFile>(buf));
    } else {
        inomap[st.st_ino] = std::make_shared<inode>(st,
                std::make_shared<UnsupportedFile>());
    }
    return st.st_ino;
}

static ino_t get_root_ino() {
    static bool got;
    static ino_t ino;
    if (!got) {
        ino = cache_path("/");
        got = true;
    }
    return ino;
}

ino_t fs_cache(const char *str) {
    char buff[MAXPATHLEN];
    if (str[0] == '/') {
        strcpy(buff, str);
    } else {
        if (getwd(buff) == nullptr)
            return 0;
        strcat(buff, "/");
        strcat(buff, str);
    }
    auto splitted = split_str(buff);

    ino_t nowino = get_root_ino();
    size_t bufpos = 0;
    size_t lastpos = 0;
    for (const auto &str: splitted) {
        restart:
        if (S_ISDIR(inomap[nowino]->meta.st_mode)) {
            auto dirfile = std::dynamic_pointer_cast<DirectoryFile>(
                    inomap[nowino]->content);

            for (auto &d : *dirfile) {
                if (strcmp(d.d_name, str.c_str()) == 0) {
                    nowino = inomap[nowino]->meta.st_ino;
                    goto ok;
                }
            }
            // failed to find
            goto fail;
            ok:
            buff[bufpos++] = '/';
            buff[bufpos] = 0;
            strcpy(&buff[bufpos], str.c_str());

            nowino = cache_path(buff);
            continue;
        } else if (S_ISLNK(inomap[nowino]->meta.st_mode)) {
            char buffsym[MAXPATHLEN];
            memcpy(buffsym, buff, bufpos);
            ssize_t s = readlink(buff, cwdbuff, MAXPATHLEN);
            memcpy(&buffsym[lastpos], cwdbuff, s);
            buffsym[lastpos + s] = 0;
            nowino = fs_cache(buffsym);
            if (nowino == 0)
                goto fail;
            goto restart;
        }
        fail:
            errno = ENOENT;
            return 0;
    }
    return nowino;
}

void dump_cache() {

}