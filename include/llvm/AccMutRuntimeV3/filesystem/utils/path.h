//
// Created by Sirui Lu on 2019-04-10.
//

#ifndef LLVM_STRINGUTIL_H
#define LLVM_STRINGUTIL_H

#include <deque>
#include <string>
#include <vector>
#include <sys/param.h>

inline
bool is_relapath(const char *path) {
    return path[0] != '/';
}

inline
std::deque<std::string> split_path(const char *str) {
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

template<typename iter>
std::deque<std::string> remove_redundant(iter b, iter e) {
    std::deque<std::string> ret;
    int ddnum = 0;
    while (b != e) {
        std::string str = *b++;
        if (str == ".")
            continue;
        if (str == "..") {
            if (ret.empty()) {
                ++ddnum;
            } else {
                ret.pop_back();
            }
            continue;
        }
        ret.push_back(std::move(str));
    }
    for (int i = 0; i < ddnum; ++i)
        ret.push_front("..");
    return ret;
}

template<typename iter>
std::string concat_path_nodup(iter b, iter e, bool relative = false) {
    std::string ret = relative ? "." : "";
    while (b != e) {
        ret += "/";
        ret += *b;
        ++b;
    }
    return ret;
}

template<typename iter>
std::string concat_path(iter b, iter e, bool relative = false) {
    auto dq = remove_redundant(b, e);
    return concat_path_nodup(dq.begin(), dq.end());
}

template<typename iter1, typename iter2>
std::string concat_path_nodup(iter1 b1, iter1 e1, iter2 b2, iter2 e2, bool relative = false) {
    std::string ret = relative ? "" : "/";
    bool add_one = false;
    while (b1 != e1) {
        add_one = true;
        ret += *b1;
        ret += "/";
        ++b1;
    }
    while (b2 != e2) {
        add_one = true;
        ret += *b2;
        ret += "/";
        ++b2;
    }
    if (add_one) {
        ret.pop_back();
    } else {
        if (relative) {
            ret = "";
        } else {
            ret = "/";
        }
    }
    return ret;
}

template<typename iter1, typename iter2>
std::string concat_path(iter1 b1, iter1 e1, iter2 b2, iter2 e2, bool relative = false) {
    auto dq1 = remove_redundant(b1, e1);
    auto dq2 = remove_redundant(b2, e2);
    auto bb1 = dq1.begin(), bb2 = dq2.begin(), ee1 = dq1.end(), ee2 = dq2.end();
    while (bb1 != ee1 && bb2 != ee2) {
        if (*bb2 == "..") {
            if (*(ee1 - 1) == "..") {
                break;
            } else {
                --ee1;
                ++bb2;
            }
        } else {
            break;
        }
    }
    return concat_path_nodup(bb1, ee1, bb2, ee2, relative);
}

#endif //LLVM_STRINGUTIL_H
