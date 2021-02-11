//
// Created by mbrozek on 22.12.20.
//

#include <iostream>
#include <sstream>
#include <algorithm>
#include "Commands.h"
#include "FileSystem.h"



void Commands::commandController(const std::string& command) {
    SingleCommand comm = parseCommand(command);
    if ((comm.val != 0) && (comm.val != 14)){
        if (!FileSystem::isSystemRunning()){
            std::cout << "FS is not running" << std::endl;
            std::cout << "Use: format <format-size>" << std::endl;
            return;
        }
    }
    switch(comm.val){
        case COPY:
            if(comm.params.size() != 2){
                std::cout << "Usage: cp <source-file> <target-file>" << std::endl;
            }else{
                if(FileSystem::cp(comm.params.at(0), comm.params.at(1))){
                    std::cout << "OK" << std::endl;
                }
            }
            break;
        case MOVE:
            if(comm.params.size() != 2){
                std::cout << "Usage: mv <source-file> <target-file>" << std::endl;
            }else{
                FileSystem::mv(comm.params.at(0), comm.params.at(1));
            }
            break;
        case REMOVE:
            if(comm.params.size() != 1){
                std::cout << "Usage: rm <file-name>" << std::endl;
            }else{
                FileSystem::rm(comm.params.at(0));
            }
            break;
        case MKDIR:
            if(comm.params.size() != 1){
                std::cout << "Usage: mkdir <directory-name>" << std::endl;
            }else{
                FileSystem::mkdir(comm.params.at(0));
            }
            break;
        case RMDIR:
            if(comm.params.size() != 1){
                std::cout << "Usage: rmdir <directory-name>" << std::endl;
            }else{
                FileSystem::rmdir(comm.params.at(0));
            }
            break;
        case LIST:
            if (comm.params.size() == 0){
                FileSystem::ls(".");
            }else if(comm.params.size() == 1){
                FileSystem::ls(comm.params.at(0));
            }else{
                std::cout << "Usage: ls <directory-name>" << std::endl;
            }
            break;
        case CAT:
            if(comm.params.size() != 1){
                std::cout << "Usage: cat <file-name>" << std::endl;
            }else{
                FileSystem::cat(comm.params.at(0));
            }
            break;
        case CD:
            if(comm.params.size() != 1){
                std::cout << "Usage: cd <directory-name>" << std::endl;
            }else{

                FileSystem::cd(comm.params.at(0));
            }
            break;
        case PWD:
            if(comm.params.size() != 0){
                std::cout << "Usage: pwd" << std::endl;
            }else{
               FileSystem::pwd();
            }
            break;
        case INFO:
            if(comm.params.size() != 1){
                std::cout << "Usage: info <directory-name/file-name>" << std::endl;
            }else{
                FileSystem::info(comm.params.at(0));
            }
            break;
        case INCP:
            if(comm.params.size() != 2){
                std::cout << "Usage: incp <source-file> <target-file>" << std::endl;
            }else{
                FileSystem::incp(comm.params.at(0), comm.params.at(1));
            }
            break;
        case OUTCP:
            if(comm.params.size() != 2){
                std::cout << "Usage: outcp <source-file> <target-file>" << std::endl;
            }else{
                FileSystem::outcp(comm.params.at(0), comm.params.at(1));
            }
            break;
        case LOAD:
            if(comm.params.size() != 1){
                std::cout << "Usage: load <file-with-commands>" << std::endl;
            }else{
                FileSystem::load(comm.params.at(0));
            }
            break;
        case FORMAT:
            if(comm.params.size() != 1){
                std::cout << "Usage: format <format-size>" << std::endl;
            }else{

                if(comm.params.at(0).length() < 2){
                    std::cout << "Size was not entered correctly" << std::endl;
                } else{
                    FileSystem::formatSystem(comm.params.at(0));
                }
            }
            break;
        case HARD_LINK:
            if(comm.params.size() != 2){
                std::cout << "Usage: ln <source-file> <hard-link>" << std::endl;
            }else{
                FileSystem::ln(comm.params.at(0), comm.params.at(1));
            }
            break;
        case BAD_COMMAND:
        default:
            std::cout << "UNKNOWN COMMAND" << std::endl;
            break;

    }
}

SingleCommand Commands::parseCommand(const std::string &command) {
    int value = BAD_COMMAND;
    std::vector<std::string> vecParams;

    std::stringstream streamStr(command);
    std::string tmpStr;
    std::getline(streamStr, tmpStr, ' ');
    for(int i = 0; i< Commands::aviCommands.size(); i++){
        if(tmpStr.compare(Commands::aviCommands[i].key)==0){
            value = Commands::aviCommands[i].val;
        }
    }

    while (std::getline(streamStr,tmpStr, ' ')){
        vecParams.push_back(tmpStr);
    }
    SingleCommand comm(value, vecParams);
    return comm;
}



