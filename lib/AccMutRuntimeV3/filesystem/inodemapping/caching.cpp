//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/caching.h>
#include <llvm/AccMutRuntimeV3/checking/panic.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/DirectoryFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/RegularFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/SymbolicLinkFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/UnsupportedFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/cwd.h>

#include <set>
#include <stack>

static char cwdbuff[MAXPATHLEN];
static std::map<ino_t, std::shared_ptr<inode>> inomap;
static std::set<ino_t> cached;

void register_inode(ino_t ino, std::shared_ptr<inode> ptr) {
    if (inomap.find(ino) != inomap.end())
        panic("Should not have ino in map");
    inomap[ino] = ptr;
}

// remove and restore the path
ino_t cache_path(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0)
        return 0;
    auto iter = inomap.find(st.st_ino);
    if (iter != inomap.end()) {
        if (iter->second->cached())
            return st.st_ino;
    }
    if (cached.find(st.st_ino) != cached.end())
        return st.st_ino;
    // fprintf(stderr, "%s\n", path);
    if (!check_read_perm(st)) {
        // no read permission
        if (S_ISLNK(st.st_mode)) {
            panic("No read perm for link");
        }
        inomap[st.st_ino] = std::make_shared<inode>(st, nullptr);
        return st.st_ino;
    }
    // assume no write only files...
    if (S_ISREG(st.st_mode)) {
        int fd = open(path, O_RDONLY);
        if (!fd)
            panic("Failed to open");
        auto buf = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (!buf) {
            close(fd);
            panic("Failed to mmap");
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

ino_t get_root_ino() {
    static bool got;
    static ino_t ino;
    if (!got) {
        ino = cache_path("/");
        got = true;
    }
    return ino;
}

ino_t get_current_ino() {
    return getwdino_internal();
}

static int link_depth = 0;

ino_t cache_tree_recur(const char *path, int depth) {
    if (depth >= 200)
        return 0;
    char buff[2 * MAXPATHLEN];
    strcpy(buff, path);
    size_t len = strlen(buff);
    size_t lstlen = len;
    for (; lstlen != 0; --lstlen) {
        if (buff[lstlen] == '/') {
            buff[lstlen] = 0;
            cache_tree_recur(buff, depth + 1);
            break;
        }
    }
    // fprintf(stderr, "%s\t%d\n", path, depth);
    ino_t outer = cache_path(path);
    if (outer != 0) {
        auto outer_inode = inomap[outer];
        if (outer_inode->isLnk()) {
            buff[lstlen] = '/';
            auto t = readlink(path, buff + lstlen + 1, MAXPATHLEN);
            buff[lstlen + 1 + t] = 0;
            if (t < 0)
                return 0;
            cache_tree_recur(buff, depth + 1);
        }
    }
    return outer;
}

ino_t cache_tree(const char *path) {
    char buff[MAXPATHLEN];
    if (is_relapath(path)) {
        getrelawd_internal(buff);
        strcat(buff, "/");
        strcat(buff, path);
    } else {
        strcpy(buff, path);
    }
    return cache_tree_recur(buff, 0);
}

std::shared_ptr<inode> query_tree(
        const char *path,
        int checkmode,
        bool follow_symlink,
        std::string *real_relapath) {
    char buff_base[MAXPATHLEN];
    char *buff = buff_base + 2;
    strcpy(buff, path);
    size_t len = strlen(path);
    if (buff[len - 1] == '/') {
        buff[len] = '.';
        buff[len + 1] = 0;
    }
    cache_tree(path);
    // if (cache_tree(path) == 0)
    //    return nullptr;
    bool is_relative;
    if (path[0] == '/') {
        // strcpy(buff, path);
        is_relative = false;
    } else {
        *(--buff) = '/';
        *(--buff) = '.';
        is_relative = true;
    }
    // printf("%s\n", buff);
    std::vector<size_t> lastbasestack;
    std::vector<size_t> lastposstack;
    if (!is_relative) {
        lastbasestack.push_back(get_root_ino());
        lastposstack.push_back(0);
    } else {
        lastbasestack.push_back(get_current_ino());
        lastposstack.push_back(1);
    }

    std::deque<std::string> strdeque = split_path(buff);
    std::deque<std::string> realpath_deque;
    if (follow_symlink)
        strdeque.push_back(".");
    int follownum = 0;
    while (!strdeque.empty()) {
        // printf("%s\n", buff);
        std::string str = strdeque.front();
        strdeque.pop_front();
        size_t base = lastbasestack.back();
        auto iter = inomap.find(base);
        if (iter == inomap.end()) {
            errno = EACCES;
            return nullptr;
        }
        auto inodeptr = iter->second;
        if (!inodeptr->cached()) {
            errno = ENOENT;
            return nullptr;
        }
        if (inodeptr->isDir()) {
            if (!inomap[base]->canExecute()) {
                errno = EACCES;
                return nullptr;
            }
            auto dirfile = std::static_pointer_cast<DirectoryFile>(inodeptr->getUnderlyingFile());
            for (auto &d : *dirfile) {
                if (strcmp(d.d_name, str.c_str()) == 0) {
                    base = d.d_ino;
                    // base = inodeptr->getIno();
                    goto ok;
                }
            }
            // failed to find
            errno = ENOENT;
            return nullptr;
            ok:
            lastposstack.push_back(lastposstack.back());
            auto &bufpos = lastposstack.back();
            buff[bufpos++] = '/';
            strcpy(&buff[bufpos], str.c_str());
            bufpos += str.length();
            buff[bufpos] = 0;
            cache_tree(buff);

            // base = cache_path(buff);
            lastbasestack.push_back(base);
            if (real_relapath)
                realpath_deque.push_back(std::move(str));
            // Cannot retrive
            if (lastbasestack.back() == 0) {
                errno = ENOENT;
                return nullptr;
            }
            link_depth = 0;
            continue;
        } else if (inodeptr->isLnk()) {
            if (++follownum == 100) {
                errno = ELOOP;
                return nullptr;
            }
            auto linkto = std::static_pointer_cast<SymbolicLinkFile>(
                    inodeptr->getUnderlyingFile())->getTarget();
            bool relative = linkto[0] != '/';

            auto d1 = split_path(linkto.c_str());
            strdeque.push_front(str);
            for (auto rb = d1.rbegin(); rb != d1.rend(); ++rb) {
                strdeque.push_front(*rb);
            }
            if (relative) {
                if (real_relapath)
                    realpath_deque.pop_back();
                lastbasestack.pop_back();
                lastposstack.pop_back();
            } else {
                if (real_relapath)
                    realpath_deque.clear();
                lastbasestack.clear();
                lastposstack.clear();
                lastbasestack.push_back(get_root_ino());
                lastposstack.push_back(0);
            }
            continue;
        } else if (strdeque.empty() && follow_symlink) {
            // actually found
            break;
        }
        errno = ENOENT;
        return nullptr;
    }
    ino_t ino = lastbasestack.back();
    auto iter = inomap.find(ino);
    if (iter == inomap.end()) {
        errno = EACCES;
        return nullptr;
    }
    if ((checkmode & CHECK_R) && !iter->second->canRead())
        goto noaccess;
    if ((checkmode & CHECK_W) && !iter->second->canWrite())
        goto noaccess;
    if ((checkmode & CHECK_X) && !iter->second->canExecute())
        goto noaccess;
    if (real_relapath) {
        if (is_relative) {
            getwd_internal(buff);
            auto d = split_path(buff);
            *real_relapath = concat_path(
                    d.begin(), d.end(),
                    realpath_deque.begin(),
                    realpath_deque.end(),
                    false);
        } else {
            *real_relapath = concat_path(
                    realpath_deque.begin(),
                    realpath_deque.end(),
                    false);
        }
    }
    return iter->second;
    noaccess:
    errno = EACCES;
    return nullptr;
    // return lastbasestack.back();
}

std::shared_ptr<inode> ino2inode(ino_t ino) {
    auto iter = inomap.find(ino);
    if (iter == inomap.end())
        return nullptr;
    return iter->second;
}
