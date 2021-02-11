//
// Created by mbrozek on 22.12.20.
//

#ifndef ZOS_SEM_2020_COMMANDS_H
#define ZOS_SEM_2020_COMMANDS_H

#include <string>
#include <utility>
#include <vector>

#define BAD_COMMAND 0
#define COPY 1
#define MOVE 2
#define REMOVE 3
#define MKDIR 4
#define RMDIR 5
#define LIST 6
#define CAT 7
#define CD 8
#define PWD 9
#define INFO 10
#define INCP 11
#define OUTCP 12
#define LOAD 13
#define FORMAT 14
#define HARD_LINK 15

class SingleCommand{
private:

public:
    std::vector<std::string> params;
    int val;
    SingleCommand(int value, std::vector<std::string> key) : val(value), params(std::move(key)){}
};
struct Comm{
    int val;
    std::string key;
};
class Commands{
public:
    inline static const std::vector<Comm> aviCommands{{COPY, "cp"}, {MOVE, "mv"}, {REMOVE, "rm"}, {MKDIR, "mkdir"}, {RMDIR, "rmdir"}, {LIST, "ls"}, {CAT, "cat"}, {CD, "cd"}, {PWD, "pwd"}, {INFO, "info"}, {INCP, "incp"}, {OUTCP, "outcp"}, {LOAD, "load"}, {FORMAT, "format"}, {HARD_LINK, "ln"}};
public:
    static void commandController(const std::string& command);
private:

    static SingleCommand parseCommand(const std::string &basicString);
};
#endif //ZOS_SEM_2020_COMMANDS_H
