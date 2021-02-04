//
// Created by mbrozek on 27.12.20.
//

#include <strings.h>
#include <cstring>
#include <vector>
#include "FileSystem.h"

std::string FileSystem::systemName;
std::fstream FileSystem::fileSystem;
std::unique_ptr<SuperBlock> FileSystem::superBlock;
std::unique_ptr<PsInode> FileSystem::currentInode;

bool FileSystem::formatSystem(const std::string &systemSize) {
    unsigned long system_size = parseSize(systemSize);

    if(system_size == 0){
        return false;
    }

    if (fileSystem.is_open()){
        fileSystem.close();
        remove(systemName.c_str());
    }

    std::ofstream outputStream(systemName.c_str(), std::ios::out|std::ios::binary);

    if(!outputStream.is_open()){
        std::cout << "Error occurred while creating output stream" << std::endl;
        return false;
    }

    unsigned long remain = system_size;
    remain -= sizeof(SuperBlock);
    unsigned long clusterCount = 0, inodeCount = 0;
    unsigned long bytesForInode = remain*0.002;
    unsigned long inodeData = 0;
    while (inodeData < bytesForInode){
        inodeData +=1 + (8 * sizeof(inodeCount));
        inodeCount += 8;
    }
    std::cout<< "InodeCount: " << inodeCount << std::endl;
    remain -= inodeData;

    unsigned long clusterData = 0;
    int addByte=0;
    while (clusterData < remain){
        clusterData +=+ BLOCK_SIZE;
        clusterCount++;
        addByte++;
        if (addByte%8==0){
            clusterData+=1;
            addByte=0;
        };
    }
    clusterCount--;
    clusterData -= BLOCK_SIZE;
    int modDiff = clusterCount % 8;
    clusterCount -= modDiff;
    clusterData -= (modDiff * BLOCK_SIZE);
    remain -= clusterData;
    std::cout<< "BlockCount: " << clusterCount << std::endl;
//    std::cout<< "Remain: " << remain << std::endl;


    if(!initSuperBlock(system_size-remain,inodeCount, clusterCount)){
        std::cout << "Error occurred while initializing Super block" << std::endl;
        return false;
    }

    outputStream.write(reinterpret_cast<char*>(superBlock.get()), sizeof(SuperBlock));
//
//    std::cout << "After superblock: " << outputStream.tellp() << std::endl;

    //BITMAPA INIT
    outputStream.put(0x80);
    for (int i = 1; i < inodeCount/8; i++) {
        outputStream.put(0x00);
    }

    outputStream.put(0x80);
    for (int i = 0; i < clusterCount/8; ++i) {
        outputStream.put(0x00);
    }
//    std::cout << "After bitmap: "<<outputStream.tellp() << std::endl;


    currentInode = std::make_unique<PsInode>();
    currentInode->isDirectory = true;
    currentInode->references = 0;
    currentInode->nodeid = 0;
    currentInode->file_size = sizeof(DirectoryItem) * 2;
    outputStream.write((char*)currentInode.get(), sizeof(PsInode));
    currentInode->isDirectory = false;
    currentInode->file_size = 0;
    for (int i = 1; i < inodeCount; i++) {
        currentInode->nodeid = i;
        outputStream.write((char*)currentInode.get(), sizeof(PsInode));
    }
//    std::cout << "After inode: "<<outputStream.tellp() << std::endl;

    auto* tmpDir = new DirectoryItem {0, '/'};
    outputStream.write((char*) tmpDir, sizeof(DirectoryItem));
    outputStream.write((char*) tmpDir, sizeof(DirectoryItem));
    for (int i = 0; i < (BLOCK_SIZE - sizeof(DirectoryItem)*2); ++i) {
        outputStream.put(0x00);
    }
    delete tmpDir;

    for (int i = 0; i < (clusterCount-1)*BLOCK_SIZE; i++) {
        outputStream.put(0x00);
    }
//    std::cout << "After data blocks: "<<outputStream.tellp() << std::endl;

    outputStream.flush();
    outputStream.close();
    fileSystem.open(systemName, std::ios::in|std::ios::out|std::ios::binary);
    std::cout << "FS formatted correctly" << std::endl;
    return true;
}

bool FileSystem::loadSystem() {
    fileSystem.open(systemName, std::ios::in|std::ios::out|std::ios::binary);
    if (fileSystem.is_open()){
        currentInode = std::make_unique<PsInode>();
        superBlock = std::make_unique<SuperBlock>();

        fileSystem.read(reinterpret_cast<char *>(superBlock.get()), sizeof(SuperBlock));
        fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
        fileSystem.read(reinterpret_cast<char *>(currentInode.get()), sizeof(PsInode));
        return true;
    }
    std::cout << "FS can not be opened" << std::endl;
    return false;
}


void FileSystem::incp(const std::string & extFile,const std::string &inpFile) {
    std::ifstream inputStream;
    inputStream.open(extFile, std::ios::in|std::ios::binary);
    if (inputStream.is_open()){
        auto parent = std::make_unique<PsInode>();
        std::string dir, file;
        file = getFileFromPath(inpFile);
        dir = getDirectoriesFromPath(inpFile);

        /** Check if file name is already taken in file or directories */

        inputStream.seekg(0, std::ios::end);
        unsigned long fileSize = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);


        /** */
        char content[fileSize];
        inputStream.read((char*)content, fileSize);
        inputStream.close();
        std::string strContent;
        for(auto& byte : content){
            strContent += byte;
        }
        createFile(parent.get(),file, strContent,fileSize);

    }else{
        std::cout<< "File not found or it cannot be opened" << std::endl;
    }
}

/** HELPER FUNCTIONS */
unsigned long FileSystem::parseSize(const std::string &systemSize) {
    std::string unit = systemSize.substr(systemSize.length()-2, systemSize.length());
    unsigned long multiply;
    if (unit == "KB"){
        multiply = KILOBYTE;
    } else if (unit == "MB"){
        multiply = MEGABYTE;
    }else if (unit == "GB"){
        multiply = GIGABYTE;
    } else{
        std::cout << "Unknown memory type (GB, MB, KB expected)" << std::endl;
        std::cout << "e.g.: format 600MB" << std::endl;
        return 0;
    }

    unsigned long size = stol(systemSize.substr(0,systemSize.length()-2));
    if ((2 * GIGABYTE) < (size * multiply)) {
        std::cout << "FS maximum size is 2GB" << std::endl;
        std::cout << "Please enter size smaller then 2GB" << std::endl;
        return 0;
    }else if((size*multiply)< (KILOBYTE*41)) {
        std::cout << "FS minimum size is 41 KB" << std::endl;
        return 0;
    }

    std::cout << "FS size: " << size<< unit << std::endl;
    size = size * multiply * 1024;
    return size;
}

bool FileSystem::initSuperBlock(unsigned long size, unsigned long inodeCount, unsigned long clusterCount) {
    superBlock = std::make_unique<SuperBlock>();
    bzero(superBlock->signature, 9);
    strcpy(superBlock->signature, "Brozek\0");
    bzero(superBlock->volume_descriptor, 251);
    strcpy(superBlock->volume_descriptor, "ZOS 2020\0");
    superBlock->disk_size = size;
    superBlock->cluster_size = BLOCK_SIZE;
    superBlock->cluster_count = clusterCount;
    superBlock->inode_count = inodeCount;
    superBlock->bitmapi_start_address = sizeof(SuperBlock);
    superBlock->bitmap_start_address = sizeof(SuperBlock) + inodeCount/8;
    superBlock->inode_start_address = sizeof(SuperBlock) + inodeCount/8 + clusterCount/8;
    superBlock->data_start_address = superBlock->inode_start_address + inodeCount * sizeof(PsInode);
    return true;
}

bool FileSystem::isSystemRunning() {
    return fileSystem.is_open();
}

std::string FileSystem::getFileFromPath(const std::string &path) {
    size_t index = path.find_last_of('/');
    if (index == std::string::npos){
        return path;
    }
    return path.substr(index+1);
}

std::string FileSystem::getDirectoriesFromPath(const std::string &path) {
    size_t index = path.find_last_of('/');
    if (index == std::string::npos){
        return "";
    }
    return path.substr(0,index+1);
}




/** Functions working with file system from outside */
void FileSystem::createFile(PsInode *parent, std::string &fileName, const std::string& content, unsigned long size) {
    auto freePsInodeAddr = getFreeInode();
    unsigned long clustersNeeded = (size/(BLOCK_SIZE))+1;
    std::vector<unsigned long> freeClusterAddr = getFreeClusters(clustersNeeded);
    std::cout << "Free Inode address " << freePsInodeAddr << std::endl;
    std::cout << "Clusters needed " << clustersNeeded << std::endl;

    std::vector<std::string> contentForClusters;
    for (size_t index = 0; index < content.length(); index += superBlock->cluster_size) {
        contentForClusters.push_back(content.substr(index, superBlock->cluster_size));
    }

    for(unsigned long i = 0; i<freeClusterAddr.size(); i++){
        fileSystem.seekg(freeClusterAddr.at(i));
        fileSystem.write(contentForClusters.at(i).data(), superBlock->cluster_size);
        fileSystem.flush();
    }
    //TODO create new inode and fill

}

unsigned long FileSystem::getFreeInode() {
    unsigned char currentByte;
    fileSystem.seekg(superBlock->bitmapi_start_address, std::ios::beg);
    for (unsigned long ixByte= 0; ixByte < superBlock->inode_count/8 ; ixByte++) {
        currentByte = fileSystem.get();
        for (unsigned long ixBit = 7; ixBit>=0 ; ixBit--) {
            if(!((currentByte >> ixBit) & 1U)){
                fileSystem.seekg(-1, std::ios::cur);
                currentByte |= 1UL << ixBit;
                fileSystem.put(currentByte);
                fileSystem.flush();
                return (ixByte * 8) + (7-ixBit);
            }
        }
    }
    return 0;
}

std::vector<unsigned long>FileSystem::getFreeClusters(unsigned long clustersNeeded) {
    unsigned char currentByte;
    std::vector<unsigned long> addresses;
    unsigned long indexAddress= 0;
    fileSystem.seekg(superBlock->bitmap_start_address, std::ios::beg);
    for (unsigned long ixByte = 0; ixByte < superBlock->cluster_count/8; ixByte++) {
        currentByte = fileSystem.get();
        for (unsigned long ixBit = 7; ixBit >= 0 ; ixBit--) {
            if (!((currentByte >> ixBit)& 1U)){
                fileSystem.seekg(-1, std::ios::cur);
                currentByte |= 1UL << ixBit;
                fileSystem.put(currentByte);
                fileSystem.flush();
                addresses.push_back((((ixByte * 8) + (7-ixBit))* superBlock->cluster_size)+superBlock->data_start_address);
                if (addresses.size() == clustersNeeded){
                    goto endOfLoops;
                }
            }
        }
    }
    endOfLoops:
    return addresses;
}



