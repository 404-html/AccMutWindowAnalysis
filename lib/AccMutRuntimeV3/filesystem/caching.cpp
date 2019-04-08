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
#include <set>
#include <queue>
#include <stack>
#include <deque>
#include <dirent.h>

static char cwdbuff[MAXPATHLEN];
static std::map<ino_t, std::shared_ptr<inode>> inomap;

// use cwdbuff
static std::deque<std::string> split_str(const char *str) {
    char buf[MAXPATHLEN];
    strcpy(buf, str);
    char *b = buf;
    char *e = b;
    std::deque<std::string> ret;
    while (*e != 0) {
        if (*e == '/') {
            if (b == e) {
                b = ++e;
                continue;
            }
            *e = 0;
            ret.push_back(b);
            b = ++e;
        } else {
            ++e;
        }
    }
    if (b != e)
        ret.push_back(b);
    return ret;
}

static ino_t cache_path(const char *path) {
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
        struct dirent *pdir;
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

static int link_depth = 0;

ino_t fs_cache_helper(const char *path) {
    char buff[MAXPATHLEN];
    if (path[0] == '/') {
        strcpy(buff, path);
    } else {
        if (getwd(buff) == nullptr)
            return 0;
        strcat(buff, "/");
        strcat(buff, path);
    }
    std::vector<size_t> lastbasestack;
    lastbasestack.push_back(get_root_ino());
    std::vector<size_t> lastposstack;
    lastposstack.push_back(0);
    std::deque<std::string> strdeque = split_str(buff);
    int follownum = 0;
    while (!strdeque.empty()) {
        std::string str = strdeque.front();
        strdeque.pop_front();
        size_t base = lastbasestack.back();
        if (S_ISDIR(inomap[base]->meta.st_mode)) {
            auto dirfile = std::static_pointer_cast<DirectoryFile>(
                    inomap[base]->content);
            for (auto &d : *dirfile) {
                if (strcmp(d.d_name, str.c_str()) == 0) {
                    base = inomap[base]->meta.st_ino;
                    goto ok;
                }
            }
            // failed to find
            goto fail;
            ok:
            lastposstack.push_back(lastposstack.back());
            auto &bufpos = lastposstack.back();
            buff[bufpos++] = '/';
            strcpy(&buff[bufpos], str.c_str());
            bufpos += str.length();
            buff[bufpos] = 0;

            base = cache_path(buff);
            lastbasestack.push_back(base);
            link_depth = 0;
            continue;
        } else if (S_ISLNK(inomap[base]->meta.st_mode)) {
            if (++follownum == 100) {
                errno = ELOOP;
                return 0;
            }
            /*char buffsym[MAXPATHLEN];
            memcpy(buffsym, buff, bufpos);
            ssize_t s = readlink(buff, cwdbuff, MAXPATHLEN);
            memcpy(&buffsym[lastpos + 1], cwdbuff, s);
            buffsym[lastpos + s + 1] = 0;*/
            char buff1[MAXPATHLEN];
            ssize_t s = readlink(buff, buff1, MAXPATHLEN);
            buff1[s] = 0;
            bool relative = buff1[0] != '/';

            auto d1 = split_str(buff1);
            strdeque.push_front(str);
            for (auto rb = d1.rbegin(); rb != d1.rend(); ++rb) {
                strdeque.push_front(*rb);
            }
            if (relative) {
                lastbasestack.pop_back();
                lastposstack.pop_back();
            } else {
                lastbasestack.clear();
                lastposstack.clear();
                lastbasestack.push_back(get_root_ino());
                lastposstack.push_back(0);
            }
            continue;
        }
        fail:
        errno = ENOENT;
        return 0;
    }
    return lastbasestack.back();
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
    return fs_cache_helper(str);
}

static std::set<ino_t> dumped;

void dump_helper(FILE *f, ino_t ino) {
    fprintf(f, "Start dump: %lu\n", (unsigned long) ino);
    dumped.insert(ino);
    auto ptr = inomap[ino];
    ptr->dump(f);
    if (S_ISDIR(ptr->meta.st_mode)) {
        for (const auto &t : *std::static_pointer_cast<DirectoryFile>(ptr->content)) {
            if (dumped.find(t.d_ino) == dumped.end())
                if (inomap.find(t.d_ino) != inomap.end())
                    dump_helper(f, t.d_ino);
        }
    }
    fprintf(f, "End dump: %lu\n", (unsigned long) ino);
}

void dump_cache(FILE *f) {
    dumped.clear();
    dump_helper(f, get_root_ino());
}