#include <iostream>
#include <langinfo.h>
#include <sstream>
#include "Commands.h"
#include "FileSystem.h"

bool loadFileSystem(char *fSysName);

void formatSystem();

int main(int argc, char *argv[]){
    if (argc < 2){
        std::cout << "Not enough parameter to run file system" << std::endl;
        std::cout << "Name of the file system is missing" << std::endl;
        exit(1);
    }
    if(loadFileSystem(argv[1])){
        std::cout << "File system "<< FileSystem::systemName <<" loaded successfully" << std::endl;
        std::string input;
        while (true){
            getline(std::cin,input);
            if (input =="exit"){
                break;
            }else{
             Commands::commandController(input);
            }
        }
    }else{
        std::cout << "FS was neither loaded or formatted correctly" << std::endl;
        return 0;
    }
    std::cout << "Exiting FS" << std::endl;
    return 0;
}

bool loadFileSystem(char *fSysName) {
    FileSystem::systemName = fSysName;
    std::cout << "Loading file system: " << fSysName << std::endl;
    if(!FileSystem::loadSystem()){
        formatSystem();
    }

    return FileSystem::isSystemRunning();
}

void formatSystem() {
    std::cout << "File system under name: " << FileSystem::systemName << " does not exist!" << std::endl;
    std::cout << "For initializing this File system please enter system size." << std::endl;
    std::string systemSize;
    getline(std::cin, systemSize);
    Commands::commandController(systemSize);
}


