//
// Created by mbrozek on 27.12.20.
//

#ifndef ZOS_SEM_2020_FILESYSTEM_H
#define ZOS_SEM_2020_FILESYSTEM_H

#include <string>

class FileSystem{
public:
    bool formatSystem(const std::string& systemName, const std::string& systemSize);
    bool loadSystem(const std::string& systemName);

private:
    int parseSize(const std::string &systemSize);
};
#endif //ZOS_SEM_2020_FILESYSTEM_H
