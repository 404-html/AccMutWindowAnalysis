//
// Created by Sirui Lu on 2019-04-08.
//

#ifndef LLVM_DIRECTORYFILE_H
#define LLVM_DIRECTORYFILE_H

#include "BaseFile.h"
#include <list>
#include <dirent.h>

class DirectoryFile: public BaseFile {
    std::list<dirent> dirents;
    typedef std::list<dirent>::iterator iterator;
    typedef std::list<dirent>::const_iterator const_iterator;
public:
    inline DirectoryFile(std::list<dirent> dirents)
        : dirents(std::move(dirents)) {}
    inline iterator begin() { return dirents.begin(); };
    inline iterator end() { return dirents.end(); };
    inline const_iterator begin() const { return dirents.begin(); };
    inline const_iterator end() const { return dirents.end(); };
    void dump(FILE *f) override;
};

#endif //LLVM_DIRECTORYFILE_H
