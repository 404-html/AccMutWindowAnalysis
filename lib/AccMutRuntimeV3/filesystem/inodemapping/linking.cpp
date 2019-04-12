//
// Created by Sirui Lu on 2019-04-08.
//

#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/linking.h>
#include <llvm/AccMutRuntimeV3/filesystem/inodemapping/caching.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/DirectoryFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/RegularFile.h>
#include <llvm/AccMutRuntimeV3/filesystem/datastructure/SymbolicLinkFile.h>
#include <errno.h>
#include <unistd.h>
#include <llvm/AccMutRuntimeV3/checking/panic.h>

blksize_t blksize;

struct _BLKSIZE_INIT {
    _BLKSIZE_INIT() {
        struct stat st;
        if (lstat(".", &st) < 0)
            panic("Failed to init blksize");
        blksize = st.st_blksize;
    }
} _init;

int path_resolve(const char *from, bool follow_symlink, std::string *to) {
    auto t = query_tree(from, 0, follow_symlink, to);
    if (!t)
        return -1;
    return 0;
}


int delete_file(const char *name, bool follow_symlink) {
    if (name[strlen(name) - 1] == '/') {
        panic("Should not end with '/'");
    }
    std::string path;
    if (!query_tree(name, 0, follow_symlink, &path)) {
        errno = EACCES;
        return 0;
    }
    size_t pos = path.rfind('/');
    std::string father;
    std::string file_name;
    if (pos == std::string::npos) {
        father = ".";
        file_name = path;
    } else {
        father = path.substr(0, pos + 1);
        file_name = path.substr(pos + 1);
    }

    auto father_inode = query_tree(father.c_str(), CHECK_W, true);
    if (!father_inode) {
        // errno set by query_tree
        return 0;
    }
    if (!father_inode->isDir()) {
        errno = ENOTDIR;
        return 0;
    }

    auto this_inode = query_tree(path.c_str(), 0, follow_symlink);
    if (!this_inode)
        panic("Must be found");
    if (this_inode->isDir()) {
        errno = EPERM;
        return -1;
    }

    std::shared_ptr<DirectoryFile> pdf =
            std::static_pointer_cast<DirectoryFile>(father_inode->getUnderlyingFile());
    for (auto b = pdf->begin(); b != pdf->end(); ++b) {
        if (strcmp(b->d_name, file_name.c_str()) == 0) {
            // found
            if (this_inode->getIno() != b->d_ino)
                panic("Should be the same inode number");
            this_inode->decRef();
            // no need to expire

            pdf->erase(b);
            break;
        }
    }
    return 0;
}


ino_t get_fresh_ino() {
    static ino_t ori = 0x6000000000000000;
    return --ori;
}

#ifdef __APPLE__

#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec

#endif

ino_t new_regular(const char *name, mode_t mode, const ino_t *ino) {
    if (name[strlen(name) - 1] == '/') {
        panic("Should not end with '/'");
    }
    std::string path;
    if (query_tree(name, 0, false)) {
        errno = EEXIST;
        return 0;
    }
    path = name;
    size_t pos = path.rfind('/');
    std::string father;
    std::string file_name;
    if (pos == std::string::npos) {
        father = ".";
        file_name = path;
    } else {
        father = path.substr(0, pos + 1);
        file_name = path.substr(pos + 1);
    }

    auto father_inode = query_tree(father.c_str(), CHECK_W, true);
    if (!father_inode) {
        // errno set by query_tree
        return 0;
    }
    if (!father_inode->isDir()) {
        errno = ENOTDIR;
        return 0;
    }

    struct stat st;
    st.st_mode = (mode_t) S_IFREG | mode;
    if (ino)
        st.st_ino = *ino;
    else
        st.st_ino = get_fresh_ino();
    st.st_nlink = 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    st.st_blksize = blksize;
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;

    std::shared_ptr<RegularFile> rf = std::make_shared<RegularFile>(std::vector<char>(1), 0);
    std::shared_ptr<inode> pi = std::make_shared<inode>(st, rf);
    register_inode(st.st_ino, pi);

    dirent d{
            .d_ino = st.st_ino,
#ifdef __APPLE__
            .d_namlen = (uint16_t) strlen(name),
#endif
            .d_type = DT_REG
    };
    strcpy(d.d_name, file_name.c_str());
    auto father_file = std::static_pointer_cast<DirectoryFile>(father_inode->getUnderlyingFile());

    father_file->push_back(d);

    return st.st_ino;
}


ino_t new_dir(const char *name, mode_t mode, ino_t *ino) {
    std::string path;
    if (query_tree(name, 0, false)) {
        errno = EEXIST;
        return 0;
    }
    path = name;
    if (path.back() == '/')
        path.pop_back();
    size_t pos = path.rfind('/');
    std::string father;
    std::string file_name;
    if (pos == std::string::npos) {
        father = ".";
        file_name = path;
    } else {
        father = path.substr(0, pos + 1);
        file_name = path.substr(pos + 1);
    }

    auto father_inode = query_tree(father.c_str(), CHECK_W, true);
    if (!father_inode) {
        // errno set by query_tree
        return 0;
    }
    if (!father_inode->isDir()) {
        errno = ENOTDIR;
        return 0;
    }

    struct stat st;
    st.st_mode = (mode_t) S_IFDIR | mode;
    if (ino)
        st.st_ino = *ino;
    else
        st.st_ino = get_fresh_ino();
    st.st_nlink = 2;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0; // not work here
    st.st_blksize = blksize;
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;

    std::list<dirent> dirents;
    dirent dot{
            .d_ino = st.st_ino,
#ifdef __APPLE__
            .d_namlen = 1,
#endif
            .d_type = DT_DIR
    };
    strcpy(dot.d_name, ".");
    dirents.push_back(dot);

    dirent ddot{
            .d_ino = father_inode->getIno(),
#ifdef __APPLE__
            .d_namlen = 2,
#endif
            .d_type = DT_DIR
    };
    strcpy(ddot.d_name, "..");
    dirents.push_back(ddot);

    std::shared_ptr<DirectoryFile> df = std::make_shared<DirectoryFile>(std::move(dirents));
    std::shared_ptr<inode> pi = std::make_shared<inode>(st, df);
    register_inode(st.st_ino, pi);

    dirent d{
            .d_ino = st.st_ino,
#ifdef __APPLE__
            .d_namlen = (uint16_t) strlen(name),
#endif
            .d_type = DT_DIR
    };
    strcpy(d.d_name, file_name.c_str());
    auto father_file = std::static_pointer_cast<DirectoryFile>(father_inode->getUnderlyingFile());

    father_file->push_back(d);

    father_inode->incRef();

    return st.st_ino;

}


ino_t new_symlink(const char *name, const char *linkto, ino_t *ino) {
    if (name[strlen(name) - 1] == '/') {
        panic("Should not end with '/'");
    }
    std::string path;
    if (query_tree(name, 0, false)) {
        errno = EEXIST;
        return 0;
    }
    path = name;
    size_t pos = path.rfind('/');
    std::string father;
    std::string file_name;
    if (pos == std::string::npos) {
        father = ".";
        file_name = path;
    } else {
        father = path.substr(0, pos + 1);
        file_name = path.substr(pos + 1);
    }

    auto father_inode = query_tree(father.c_str(), CHECK_W, true);
    if (!father_inode) {
        // errno set by query_tree
        return 0;
    }
    if (!father_inode->isDir()) {
        errno = ENOTDIR;
        return 0;
    }

    struct stat st;
    st.st_mode = (mode_t) S_IFLNK | 0755;
    if (ino)
        st.st_ino = *ino;
    else
        st.st_ino = get_fresh_ino();
    st.st_nlink = 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;

    std::shared_ptr<SymbolicLinkFile> sf = std::make_shared<SymbolicLinkFile>(linkto);
    std::shared_ptr<inode> pi = std::make_shared<inode>(st, sf);
    register_inode(st.st_ino, pi);

    dirent d{
            .d_ino = st.st_ino,
#ifdef __APPLE__
            .d_namlen = (uint16_t) strlen(name),
#endif
            .d_type = DT_LNK
    };
    strcpy(d.d_name, name);

    auto father_file = std::static_pointer_cast<DirectoryFile>(father_inode->getUnderlyingFile());

    father_file->push_back(d);

    return st.st_ino;
}