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
    switch(comm.val){
        case COPY:
            if(comm.params.size() != 2){
                std::cerr << "Usage: cp <source-file> <target-file>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case MOVE:
            if(comm.params.size() != 2){
                std::cerr << "Usage: mv <source-file> <target-file>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case REMOVE:
            if(comm.params.size() != 1){
                std::cerr << "Usage: rm <file-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case MKDIR:
            if(comm.params.size() != 1){
                std::cerr << "Usage: mkdir <directory-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case RMDIR:
            if(comm.params.size() != 1){
                std::cerr << "Usage: rmdir <directory-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case LIST:
            if(comm.params.size() != 1){
                std::cerr << "Usage: ls <directory-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case CAT:
            if(comm.params.size() != 1){
                std::cerr << "Usage: cat <file-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case CD:
            if(comm.params.size() != 1){
                std::cerr << "Usage: cd <directory-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case PWD:
            if(comm.params.size() != 0){
                std::cerr << "Usage: pwd" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case INFO:
            if(comm.params.size() != 1){
                std::cerr << "Usage: info <directory-name/file-name>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case INCP:
            if(comm.params.size() != 2){
                std::cerr << "Usage: incp <source-file> <target-file>" << std::endl;
            }else{
                FileSystem::incp(comm.params.at(0), comm.params.at(1));
            }
            break;
        case OUTCP:
            if(comm.params.size() != 2){
                std::cerr << "Usage: outcp <source-file> <target-file>" << std::endl;
            }else{
                FileSystem::outcp(comm.params.at(0), comm.params.at(1));
            }
            break;
        case LOAD:
            if(comm.params.size() != 1){
                std::cerr << "Usage: load <file-with-commands>" << std::endl;
            }else{
                //TODO Call method
            }
            break;
        case FORMAT:
            if(comm.params.size() != 1){
                std::cerr << "Usage: format <format-size>" << std::endl;
            }else{

                if(comm.params.at(0).length() < 2){
                    std::cerr << "Size was not entered correctly" << std::endl;
                } else{
                    FileSystem::formatSystem(comm.params.at(0));
                }
            }
            break;
        default:
            std::cerr << "Unknown command" << std::endl;
            break;

    }
}

SingleCommand Commands::parseCommand(const std::string &command) {
    int value = 0;
    std::vector<std::string> vecParams;

    std::stringstream streamStr(command);
    std::string tmpStr;
    std::getline(streamStr, tmpStr, ' ');
    std::cout << tmpStr << std::endl;
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



