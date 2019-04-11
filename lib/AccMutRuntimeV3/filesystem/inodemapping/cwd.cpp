//
// Created by Sirui Lu on 2019-04-10.
//

#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/cwd.h>

static bool relative = true;
// relative to real working directory only
static bool current_rela_avail = false;
static char current_rela_buf[MAXPATHLEN];
std::deque<std::string> current_rela_splitted;
// static char current_rela_len;

static std::deque<std::string> relative_path = {};
static ino_t current_ino;
static bool inited = false;

static struct _PWD_INIT {
public:
    inline _PWD_INIT() {
        printf("INIT\n");
        current_ino = cache_path(".");
        if (current_ino == 0)
            panic("Failed to init");
        current_rela_avail = getwd(current_rela_buf) != nullptr;
        if (current_rela_avail) {
            current_rela_splitted = split_path(current_rela_buf);
        }
    }
} _init_obj;

int chdir_internal(const char *path) {
    if (path[0] == '/') {
        current_ino = cache_tree(path);
        relative_path = split_path(path);
        relative = false;
    } else {
        current_ino = cache_tree(path);
        auto sp = split_path(path);
        auto sp1 = remove_redundant(sp.begin(), sp.end());
        auto b1 = relative_path.begin(), e1 = relative_path.end(),
                b2 = sp1.begin(), e2 = sp1.end();
        while (b1 != e1 && b2 != e2) {
            if (*b2 == "..") {
                if (*(e1 - 1) == "..") {
                    break;
                } else {
                    --e1;
                    ++b2;
                }
            } else {
                break;
            }
        }
        relative_path.erase(e1, relative_path.end());
        while (b2 != e2)
            relative_path.push_back(*b2++);
    }
}

ino_t getwdino_internal() {
    return current_ino;
}

int getcwd_internal(char *buf, size_t size) {
    if (current_ino == 0) {
        errno = EACCES;
        return -1;
    }
    if (size == 0) {
        errno = EINVAL;
        return -1;
    }
    if (!relative) {
        std::string str = concat_path_nodup(
                relative_path.begin(),
                relative_path.end(), relative
        );
        if (str.size() < size) {
            strcpy(buf, str.c_str());
            return 0;
        } else {
            errno = ERANGE;
            return -1;
        }
    } else {
        if (current_rela_avail) {
            std::string str = concat_path(current_rela_splitted.begin(),
                                          current_rela_splitted.end(),
                                          relative_path.begin(),
                                          relative_path.end(), false);
            if (str.size() < size) {
                strcpy(buf, str.c_str());
                return 0;
            } else {
                errno = ERANGE;
                return -1;
            }
        }
        errno = EACCES;
        return -1;
    }
}

void getrelawd_internal(char *buf) {
    std::string str = concat_path_nodup(
            relative_path.begin(),
            relative_path.end(), relative);
    strcpy(buf, str.c_str());
}

int getwd_internal(char *buf) {
    if (current_ino == 0) {
        errno = EACCES;
        return -1;
    }
    if (!relative) {
        std::string str = concat_path_nodup(
                relative_path.begin(),
                relative_path.end(), relative);
        strcpy(buf, str.c_str());
        return 0;
    } else {
        if (current_rela_avail) {
            std::string str = concat_path(current_rela_splitted.begin(),
                                          current_rela_splitted.end(),
                                          relative_path.begin(),
                                          relative_path.end(), false);
            strcpy(buf, str.c_str());
            /*
            std::string str = concat_path_nodup(
                    relative_path.begin(),
                    relative_path.end(), false
            );
            strcpy(buf, current_rela_buf);
            int pos = current_rela_len;
            if (current_rela_buf[pos - 1] == '/') {
                strcpy(buf + pos, str.c_str());
            } else {
                buf[pos] = '/';
                strcpy(buf + pos + 1, str.c_str());
            }*/
            return 0;
        }
        errno = EACCES;
        return -1;
    }
}


