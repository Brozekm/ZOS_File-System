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

void FileSystem::outcp(const std::string & fsFile,const std::string &outFile) {
    //TODO check if fsFile exists in file system
//    auto fileInFS = std::make_unique<PsInode>();
////    if(!getInodeBasedOnPath(currentInode.get(), fsFile, fileInFS.get()) or fileInFS->isDirectory == true){
////        std::cout << "File is directory" << std::endl;
////        return;
////    }
//    if (!getInodeBasedOnPath(currentInode.get(), fsFile, fileInFS.get())){
//        std::cerr << "ERRORRRR" << std::endl;
//    }

    std::ofstream outputStream;
    outputStream.open(outFile, std::ios::out|std::ios::binary);
    if (outputStream.is_open()){
        auto fileIndex = getFileBasedOnPath(fsFile);
        auto fileInFS = std::make_unique<PsInode>();
        fileSystem.seekg(superBlock->inode_start_address + fileIndex * sizeof(PsInode), std::ios::beg);
        fileSystem.read(reinterpret_cast<char *>(fileInFS.get()), sizeof(PsInode));

        std::string content = getFileContent(*fileInFS);

        outputStream.write(content.data(), content.size());
        outputStream.flush();
        outputStream.close();
    std::cout << "File '"<< fsFile <<"' extracted from FS successfully" << std::endl;
    }else{
        std::cout << "Cannot open file '"<< outFile << "' outside of FS" << std::endl;
    }
}



void FileSystem::incp(const std::string & extFile,const std::string &inpFile) {
    std::ifstream inputStream;
    inputStream.open(extFile, std::ios::in|std::ios::binary);
    if (inputStream.is_open()){
        auto parent = std::make_unique<PsInode>();
        std::string dir, file;
        file = getFileFromPath(inpFile);
        dir = getDirectoriesFromPath(inpFile);

        //TODO Check if file name is already taken in file or directories

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
        reloadCurrentInode();

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




/** Functions working with file system */
void FileSystem::createFile(PsInode *parent, std::string &fileName, const std::string& content, unsigned long size) {
    unsigned long clustersNeeded = (size/(BLOCK_SIZE))+1;
    unsigned int indirectNeeded = 0;
    unsigned long numberOfAddInIndirect = superBlock->cluster_size / sizeof(unsigned int);
    std::cout << "Number of add in indirect " << numberOfAddInIndirect << std::endl;
    if (clustersNeeded > 5 && clustersNeeded < numberOfAddInIndirect+5){
        indirectNeeded++;
        clustersNeeded++;
    }else if ((clustersNeeded > numberOfAddInIndirect+5) && (clustersNeeded < numberOfAddInIndirect * 2 + 5)){
        indirectNeeded += 2;
        clustersNeeded += 2;
    } else{
        std::cout << "Item size is to big for FS" << std::endl;
        return;
    }


    auto freePsInodeAddr = getFreeInode();
    std::vector<unsigned int> freeClusterAddr = getFreeClusters(clustersNeeded);
    std::cout << "Free Inode address " << freePsInodeAddr << std::endl;
    std::cout << "Clusters needed " << clustersNeeded << std::endl;

    std::vector<std::string> contentForClusters;
    for (size_t index = 0; index < content.length(); index += superBlock->cluster_size) {
        contentForClusters.push_back(content.substr(index, superBlock->cluster_size));
    }

    for(unsigned long i = 0; i<freeClusterAddr.size()-indirectNeeded; i++){
        fileSystem.seekg(superBlock->data_start_address + freeClusterAddr.at(i) * superBlock->cluster_size, std::ios::beg);
        fileSystem.write(contentForClusters.at(i).data(), superBlock->cluster_size);
        fileSystem.flush();
    }

    PsInode newInode{};
    newInode.nodeid = freePsInodeAddr;
    newInode.isDirectory = false;
    newInode.references = 1;
    newInode.file_size = size;

    if(indirectNeeded == 0){
        for(int i = 0 ; i< clustersNeeded; i++){
            newInode.direct[i] = freeClusterAddr.at(i);
        }
    } else if (indirectNeeded == 1){
        newInode.inDirect[0] = freeClusterAddr.at(freeClusterAddr.size()-1);
        for(int i = 0 ; i< 5; i++){
            newInode.direct[i] = freeClusterAddr.at(i);
        }
        fileSystem.seekg(superBlock->data_start_address + newInode.inDirect[0] * superBlock->cluster_size, std::ios::beg);
        for (unsigned int clusterIndex = 5 ; clusterIndex < freeClusterAddr.size() - indirectNeeded ; clusterIndex++) {
            fileSystem.write(reinterpret_cast<const char *>(&freeClusterAddr.at(clusterIndex)), sizeof(freeClusterAddr.at(clusterIndex)));
            fileSystem.flush();
        }
    }else if (indirectNeeded == 2 ){
        newInode.inDirect[0] = freeClusterAddr.at(freeClusterAddr.size()-2);
        newInode.inDirect[1] = freeClusterAddr.at(freeClusterAddr.size()-1);
        for(int i = 0 ; i< 5; i++){
            newInode.direct[i] = freeClusterAddr.at(i);
        }
        fileSystem.seekg(superBlock->data_start_address + newInode.inDirect[0] * superBlock->cluster_size, std::ios::beg);
        for (unsigned int clusterIndex = 5 ; clusterIndex < numberOfAddInIndirect + 5 ; clusterIndex++) {
            fileSystem.write(reinterpret_cast<const char *>(&freeClusterAddr.at(clusterIndex)), sizeof(freeClusterAddr.at(clusterIndex)));
            fileSystem.flush();
        }
        fileSystem.seekg(superBlock->data_start_address + newInode.inDirect[1] * superBlock->cluster_size, std::ios::beg);
        for (unsigned int clusterIndex = numberOfAddInIndirect + 5 ; clusterIndex < freeClusterAddr.size()-indirectNeeded ; clusterIndex++) {
            fileSystem.write(reinterpret_cast<const char *>(&freeClusterAddr.at(clusterIndex)), sizeof(freeClusterAddr.at(clusterIndex)));
            fileSystem.flush();
        }
    } else{
        std::cerr << "Error occurred while filling file with data" << std::endl;
        return;
    }

    fileSystem.seekg(superBlock->inode_start_address + newInode.nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.write((char * ) &newInode, sizeof(newInode));
    fileSystem.flush();
    DirectoryItem newDir{};
    newDir.inode = newInode.nodeid;
    strcpy(newDir.item_name, fileName.c_str());
    fileSystem.seekg(superBlock->data_start_address + parent->direct[0] * BLOCK_SIZE + parent->references * sizeof(DirectoryItem), std::ios::beg);
    fileSystem.write((char *) &newDir, sizeof(newDir));
    fileSystem.flush();
    parent->references += 1;
    fileSystem.seekg(superBlock->inode_start_address + parent->nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.write(reinterpret_cast<const char *>(parent), sizeof(PsInode));
    fileSystem.flush();

    std::cout << "File created successfully" << std::endl;
}

unsigned int FileSystem::getFreeInode() {
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

std::vector<unsigned int>FileSystem::getFreeClusters(unsigned long clustersNeeded) {
    unsigned char currentByte;
    std::vector<unsigned int> addresses;
    fileSystem.seekg(superBlock->bitmap_start_address, std::ios::beg);
    for (unsigned long ixByte = 0; ixByte < superBlock->cluster_count/8; ixByte++) {
        currentByte = fileSystem.get();
        for (unsigned long ixBit = 7; ixBit >= 0 ; ixBit--) {
            if (!((currentByte >> ixBit)& 1U)){
                fileSystem.seekg(-1, std::ios::cur);
                currentByte |= 1UL << ixBit;
                fileSystem.put(currentByte);
                fileSystem.flush();
                addresses.push_back((ixByte * 8) + (7-ixBit));
                if (addresses.size() == clustersNeeded){
                    goto endOfLoops;
                }
            }
        }
    }
    endOfLoops:
    return addresses;
}

bool FileSystem::getInodeBasedOnPath(PsInode *startInode, std::string path, PsInode *destination) {
    auto parentInode = std::make_unique<PsInode>();
    if (path.empty()){
        memcpy(reinterpret_cast<char*>(destination), reinterpret_cast<char*>(startInode), sizeof(PsInode));
        return true;
    }else if (path[0] == '/'){
        //path starts from root
        fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
        fileSystem.read(reinterpret_cast<char*>(parentInode.get()), sizeof(PsInode));
        path = path.erase(0, 1);
    }else if (path.find("..") == 0){
        //parent reference
        getInodeParent(startInode, parentInode.get());
        path = path.erase(0, 3);
    }else if (path[0] == '.'){
        //self reference
        memcpy(reinterpret_cast<char*>(parentInode.get()), reinterpret_cast<char*>(startInode), sizeof(PsInode));
        path = path.erase(0, 2);
    }else{
        memcpy(reinterpret_cast<char*>(parentInode.get()), reinterpret_cast<char*>(startInode), sizeof(PsInode));
    }
    if (path.empty()){
        memcpy(reinterpret_cast<char*>(destination), reinterpret_cast<char*>(parentInode.get()), sizeof(PsInode));
        return true;
    }
//    auto index = path.find_first_of('/');
//    std::string directoryName = path.substr(0, index);
//    if(!getInodeBasedOnDir(parentInode.get(), directoryName, destination)){
//        std::cout << "Directory does not exists " << path << std::endl;
//        return false;
//    }
//    memcpy(reinterpret_cast<char*>(parentInode.get()), reinterpret_cast<char*>(destination), sizeof(PsInode));
//    path.erase(0, index);
//    return getInodeOnPath(parentInode.get(), path, destination);
}

void FileSystem::getInodeParent(PsInode *startInode, PsInode *destination) {
    auto item = std::make_unique<DirectoryItem>();
    fileSystem.seekg(superBlock->data_start_address + startInode->direct[0] * superBlock->cluster_size, std::ios::beg);
    fileSystem.read(reinterpret_cast<char*>(item.get()), sizeof(DirectoryItem));

    //directory parent reference
    fileSystem.seekg(superBlock->inode_start_address + item->inode * sizeof(PsInode), std::ios::beg);
    fileSystem.read(reinterpret_cast<char*>(destination), sizeof(PsInode));
}

void FileSystem::reloadCurrentInode() {
    fileSystem.seekg(superBlock->inode_start_address + currentInode->nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &currentInode, sizeof(PsInode));
}

std::string FileSystem::getFileContent(const PsInode &fileInFS){
    std::string content;
    std::array<char, BLOCK_SIZE> buffer {0};

    for (unsigned int i : fileInFS.direct) {
        if (i == 0){
            return content;
        }
        fileSystem.seekg(superBlock->data_start_address + i * superBlock->cluster_size, std::ios::beg);
        fileSystem.read(buffer.data(),buffer.size());
        for (const auto& c : buffer) {
            if (content.size() == fileInFS.file_size){
                return content;
            }
            content += c;
        }
    }
    unsigned int indirInnerIndex = 0;
    for (const auto& inDirectLinks : fileInFS.inDirect){
        if (inDirectLinks == 0 ){
            return content;
        }
        for (int i = 0; i < BLOCK_SIZE; i += sizeof(unsigned int)) {
            fileSystem.seekg(superBlock->data_start_address + (inDirectLinks * BLOCK_SIZE)+ i, std::ios::beg);
            fileSystem.read((char *) &indirInnerIndex, sizeof(indirInnerIndex));
            if (indirInnerIndex == 0){
                return content;
            }
            fileSystem.seekg(superBlock->data_start_address + (indirInnerIndex * BLOCK_SIZE), std::ios::beg);
            fileSystem.read(buffer.data(), buffer.size());
            for (const auto& c : buffer) {
                if (content.size() == fileInFS.file_size){
                    return content;
                }
                content += c;
            }
        }
    }
    return content;
}

unsigned int FileSystem::getFileBasedOnPath(const std::string &filePath) {
    //TODO THIS METHOD CURRENTLY LOOKS FOR FILE ONLY IN ROOT -> change to find it on path
    std::string fileName = getFileFromPath(filePath);
    std::string path = getDirectoriesFromPath(filePath);
    auto root = PsInode{};
    fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
    fileSystem.read((char * )&root, sizeof(PsInode));
    for (int i = 0; i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
        auto tmpDir = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + i * sizeof(DirectoryItem), std::ios::beg);
        fileSystem.read((char *)&tmpDir, sizeof(DirectoryItem));
        if (tmpDir.item_name == fileName){
            std::cout << tmpDir.item_name << " ,index: " << tmpDir.inode << std::endl;
            return tmpDir.inode;
        }
    }
    std::cout<< "File was not found in root" << std::endl;
    return 0;
}



//bool FileSystem::getInodeBasedOnDir(PsInode *directory, const std::string& inodeName, PsInode *destination) {
//    auto item = std::make_unique<DirectoryItem>();
//    unsigned int maxBlock = directory->references / MAX_DIRECTORY_ITEMS_FOR_DATA_BLOCK;
//    unsigned int itemsPassed = 2;
//
//    unsigned int startingItemId = 2;
//    unsigned long blockAddress;
//    if (inodeName == ".."){
//        //is parent
//        fileSystem.seekg(superBlock->data_start_address + directory->direct[0] * superBlock->cluster_size);
//        fileSystem.read((char*)(item.get()), sizeof(DirectoryItem));
//        getInode(item->inode, destination);
//        return true;
//    }else if (inodeName == "."){
//        //is self reference
//        memcpy(reinterpret_cast<char*>(destination), reinterpret_cast<char*>(directory), sizeof(Inode));
//        return true;
//    }
//
//    for(int blockId = 0; blockId < maxBlock + 1; blockId++){
//        blockAddress = getXthInodeBlock(directory, blockId);
//        fileSystem.seekg(system->data_start_address + blockAddress * DEFAULT_BLOCK_SIZE + startingItemId * sizeof(DirectoryItem));
//        for(unsigned int i = startingItemId; i < MAX_DIRECTORY_ITEMS_FOR_DATA_BLOCK and
//                                             itemsPassed <= directory->references; i++, itemsPassed++){
//            //iterate though all directory items
//            fileSystem.read((char*)(item.get()), sizeof(DirectoryItem));
//            if(strcmp(item->item_name, inodeName.c_str()) == 0){
//                getInode(item->inode, destination);
//                return true;
//            }
//        }
//        startingItemId = 0;
//    }
//    std::cout << "Inode " << inodeName << " was not found" << std::endl;
//    return false;
//}

