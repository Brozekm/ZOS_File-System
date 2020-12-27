#include <iostream>
#include <langinfo.h>
#include <sstream>
#include "Commands.h"

bool loadFileSystem(char *string);

int main(int argc, char *argv[]){
    if (argc < 2){
        std::cout << "Not enough parameter to run file system" << std::endl;
        std::cout << "Name of the file system is missing" << std::endl;
        exit(1);
    }
    if(loadFileSystem(argv[1])){
        std::cout << "File system loaded successfully" << std::endl;
    }else{

    }

    return 0;
}

bool loadFileSystem(char *fSysName) {
    std::cout << "Loading file system: " << fSysName << std::endl;
//    if(loadSystem(fSysName)){
    if(true){
        std::cout << "File system under name: " << fSysName << "does not exist!" << std::endl;
        std::cout << "For initializing this File system please enter system size." << std::endl;
        std::string systemSize;
        getline(std::cin, systemSize);
        std::cout << "System size: "<< systemSize << std::endl;

        //TODO DELE TEXT BELOW - just for test
        std::string tmpS;
        getline(std::cin, tmpS);
        Commands::commandController(tmpS);
    }
    return true;
}


