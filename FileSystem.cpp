//
// Created by mbrozek on 27.12.20.
//

#include <strings.h>
#include <cstring>
#include <utility>
#include <vector>
#include "FileSystem.h"
#include "Commands.h"

std::string FileSystem::systemName;
std::fstream FileSystem::fileSystem;
std::unique_ptr<SuperBlock> FileSystem::superBlock;
PSEUDO_INODE FileSystem::currentInode;
std::string FileSystem::pwdPath;

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
        std::cout << "CANNOT CREATE FILE" << std::endl;
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


    if(!initSuperBlock(system_size-remain,inodeCount, clusterCount)){
        std::cout << "CANNOT CREATE FILE" << std::endl;
        return false;
    }

    outputStream.write(reinterpret_cast<char*>(superBlock.get()), sizeof(SuperBlock));


    //BITMAPA INIT
    outputStream.put(0x80);
    for (int i = 1; i < inodeCount/8; i++) {
        outputStream.put(0x00);
    }

    outputStream.put(0x80);
    for (int i = 1; i < clusterCount/8; ++i) {
        outputStream.put(0x00);
    }

    currentInode = PsInode {};
    currentInode.isDirectory = true;
    currentInode.references = 0;
    currentInode.nodeid = 0;
    currentInode.file_size = sizeof(DirectoryItem) * 2;
    outputStream.write((char*) &currentInode, sizeof(PsInode));

    currentInode.isDirectory = false;
    currentInode.file_size = 0;
    for (int i = 1; i < inodeCount; i++) {
        outputStream.write((char *) &currentInode, sizeof(PsInode));
    }


    auto tmpDir = DirectoryItem {};
    tmpDir.inode = 0;
    strcpy(tmpDir.item_name, ".");
    outputStream.write((char*) &tmpDir, sizeof(DirectoryItem));
    auto tmpParent =DirectoryItem {};
    tmpParent.inode = 0;
    std::string parentPath = "..";
    strcpy(tmpParent.item_name, parentPath.c_str());
    outputStream.write((char*) &tmpParent, sizeof(DirectoryItem));
    for (int i = 0; i < (BLOCK_SIZE - sizeof(DirectoryItem)*2); ++i) {
        outputStream.put(0x00);
    }


    for (int i = 0; i < (clusterCount-1)*BLOCK_SIZE; i++) {
        outputStream.put(0x00);
    }

    outputStream.flush();
    outputStream.close();
    fileSystem.open(systemName, std::ios::in|std::ios::out|std::ios::binary);
    fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
    fileSystem.read((char *) &currentInode, sizeof(PsInode));
    pwdPath = "/";
    std::cout << "OK" << std::endl;
    return true;
}

bool FileSystem::loadSystem() {
    fileSystem.open(systemName, std::ios::in|std::ios::out|std::ios::binary);
    if (fileSystem.is_open()){
        currentInode = PsInode {};
        superBlock = std::make_unique<SuperBlock>();

        fileSystem.read(reinterpret_cast<char *>(superBlock.get()), sizeof(SuperBlock));
        fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
        fileSystem.read((char *) &currentInode, sizeof(PsInode));
        pwdPath = "/";
        return true;
    }
    std::cout << "FS can not be opened" << std::endl;
    return false;
}

void FileSystem::outcp(const std::string & fsFile,const std::string &outFile) {
    std::string fileName = getFileFromPath(fsFile);
    std::string filePath = getDirectoriesFromPath(fsFile);

    PsInode currentHolder = currentInode;
    if(!changeDirectory(filePath)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    unsigned int index = 0;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (dirItem.item_name == fileName){
            index = dirItem.inode;
            break;
        }
    }
    if (index == 0 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }

    std::ofstream outputStream;
    outputStream.open(outFile, std::ios::out|std::ios::binary);
    if (outputStream.is_open()){
        auto fileInFs = PsInode {};
        fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
        fileSystem.read((char *) &fileInFs, sizeof(PsInode));

        std::string content = getFileContent(fileInFs);

        outputStream.write(content.data(), content.size());
        outputStream.flush();
        outputStream.close();
        currentInode = currentHolder;
        std::cout << "OK" << std::endl;
    }else{
        currentInode = currentHolder;
        std::cout << "PATH NOT FOUND" << std::endl;
    }
}


void FileSystem::mkdir(const std::string & path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if (file == "." || file == ".."){
        std::cout << "INVALID NAME" << std::endl;
        return;
    }
    if (!changeDirectory(dir)){
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }

    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            std::cout << "EXISTS" << std::endl;
            currentInode = currentHolder;
            return;
        }
    }
    unsigned int inodeIndex = getFreeInode();
    std::vector<unsigned int> clusterIndex = getFreeClusters(1);

    auto newDirectory = PsInode {};
    newDirectory.isDirectory = true;
    newDirectory.direct[0] = clusterIndex.at(0);
    newDirectory.references = 1;
    newDirectory.nodeid = inodeIndex;
    newDirectory.file_size = 2*sizeof(DirectoryItem);
    fileSystem.seekp(superBlock->inode_start_address + inodeIndex * sizeof(PsInode), std::ios::beg);
    fileSystem.write((char *) & newDirectory, sizeof(PsInode));
    fileSystem.flush();

    auto itselfDirItem = DirectoryItem {};
    itselfDirItem.inode = inodeIndex;
    strcpy(itselfDirItem.item_name, ".");
    auto parentDirItem = DirectoryItem {};
    parentDirItem.inode = currentInode.nodeid;
    strcpy(parentDirItem.item_name, "..");
    fileSystem.seekp(superBlock->data_start_address + newDirectory.direct[0] * BLOCK_SIZE, std::ios::beg);
    fileSystem.write((char *) &itselfDirItem, sizeof(DirectoryItem));
    fileSystem.write((char *) &parentDirItem, sizeof(DirectoryItem));
    fileSystem.flush();


    auto dirAsDirItem = DirectoryItem {};
    dirAsDirItem.inode = inodeIndex;
    strcpy(dirAsDirItem.item_name, file.c_str());
    for (int i = 2* sizeof(DirectoryItem); i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
        auto tmpDirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &tmpDirItem, sizeof(DirectoryItem));
        if (tmpDirItem.item_name[0] == 0){
            fileSystem.seekp(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.write((char *) &dirAsDirItem, sizeof(DirectoryItem));
            fileSystem.flush();
            break;
        }
    }

    currentInode.file_size += sizeof(DirectoryItem);
    currentInode = currentHolder;
    reloadCurrentInode();
    std::cout << "OK" << std::endl;

}


bool FileSystem::cd(std::string path) {
   if (changeDirectory(std::move(path))){
       std::cout << "OK" << std::endl;
   } else{
       std::cout << "PATH NOT FOUND" << std::endl;
   }
}

void FileSystem::mv(const std::string &oldPath, const std::string &newPath) {
    if(cp(oldPath, newPath)){
        rm(oldPath);
    }
}

void FileSystem::ls(const std::string &path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if (!changeDirectory(dir)){
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }
    long index = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            index = dirItem.inode;
            break;
        }
    }
    if (index == -1 ){
        std::cout<< "PATH NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }
    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));
    if (fileInFs.isDirectory){
        for (unsigned int i = 2 * sizeof(DirectoryItem); i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
            auto tmpDirItem = DirectoryItem {};
            fileSystem.seekg(superBlock->data_start_address + fileInFs.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.read((char *) & tmpDirItem, sizeof(DirectoryItem));
            if (tmpDirItem.item_name[0] != 0){
                auto tmpInode = PsInode {};
                fileSystem.seekg(superBlock->inode_start_address + tmpDirItem.inode * sizeof(PsInode), std::ios::beg);
                fileSystem.read((char *) & tmpInode, sizeof(PsInode));
                if (tmpInode.isDirectory){
                    std::cout << "+ " << tmpDirItem.item_name << std::endl;
                }else{
                    std::cout << "- " << tmpDirItem.item_name << std::endl;
                }
            }
        }

    }else{
        std::cout << "PATH NOT FOUND" << std::endl;
    }
    currentInode = currentHolder;
}

void FileSystem::pwd() {
    std::cout << pwdPath << std::endl;
}

void FileSystem::cat(const std::string &path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if (!changeDirectory(dir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    unsigned int index = 0;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (dirItem.item_name == file){
            index = dirItem.inode;
            break;
        }
    }
    if (index == 0 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }

    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));

    std::string content = getFileContent(fileInFs);

    std::cout << content << std::endl;
    currentInode = currentHolder;
}

void FileSystem::info(const std::string &path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if(!changeDirectory(dir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    long index = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            index = dirItem.inode;
            break;
        }
    }
    if (index == -1 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }

    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));

    std::cout<< file << " - " << fileInFs.file_size <<  " - " << fileInFs.nodeid;
    if ((file == "." ) && (pwdPath == "/")){
       std::cout << " - " << fileInFs.direct[0];
    }else{
        for (unsigned int direct : fileInFs.direct) {
            if (direct != 0){
                std::cout << " - " << direct;
            }
        }
        for(unsigned int indirect : fileInFs.inDirect){
            if (indirect != 0){
                std::cout << " - " << indirect;
            }
        }
    }
    std::cout << std::endl;
    currentInode = currentHolder;
}


void FileSystem::rmdir(const std::string &path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);
    if ((pwdPath == "/") && ((file == ".") || (file == ".."))){
        std::cout << "CANNOT DELETE ROOT" << std::endl;
        return;
    }
    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if (!changeDirectory(dir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    long index = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            index = dirItem.inode;
            break;
        }
    }
    if (index == -1 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }
    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));
    if (!fileInFs.isDirectory){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }
    for (unsigned int i = 2*sizeof(DirectoryItem); i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
        auto tmpDirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + fileInFs.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &tmpDirItem, sizeof(DirectoryItem));
        if (tmpDirItem.item_name[0] != 0){
            std::cout << "NOT EMPTY" << std::endl;
            currentInode = currentHolder;
            return;
        }
    }

    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto tmpDirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &tmpDirItem, sizeof(DirectoryItem));
        if (fileInFs.nodeid == tmpDirItem.inode){
            auto emptyDir = DirectoryItem {};
            fileSystem.seekp(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.write((char *) &emptyDir, sizeof(DirectoryItem));
            fileSystem.flush();
            break;
        }
    }

    freeCluster(fileInFs.direct[0]);
    freeInode(fileInFs.nodeid);
    std::cout << "OK" << std::endl;
    currentInode = currentHolder;
}

void FileSystem::rm(const std::string &path) {
    std::string file, dir;
    file = getFileFromPath(path);
    dir = getDirectoriesFromPath(path);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if(!changeDirectory(dir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    long index = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            index = dirItem.inode;
            break;
        }
    }
    if (index == -1 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }
    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));

    if (fileInFs.isDirectory){
        std::cout<< "CANNOT REMOVE DIRECTORY" << std::endl;
        currentInode = currentHolder;
        return;
    }

    if(fileInFs.references < 2){
        for (unsigned int direct : fileInFs.direct){
            if (direct != 0){
                fileSystem.seekp(superBlock->data_start_address + direct*BLOCK_SIZE, std::ios::beg);
                for (int j = 0; j < BLOCK_SIZE; ++j) {
                    fileSystem.put('\0');
                }
                freeCluster(direct);
            }
        }
        for (unsigned int indirect : fileInFs.inDirect) {
            if (indirect != 0){
                for (int i = 0; i < BLOCK_SIZE; i+=sizeof(unsigned int)) {
                    unsigned int index;
                    fileSystem.seekg(superBlock->data_start_address + indirect * BLOCK_SIZE + i, std::ios::beg);
                    fileSystem.read(reinterpret_cast<char *>(&index), sizeof(index));
                    if (index == 0){
                        break;
                    }
                    fileSystem.seekp(superBlock->data_start_address + index*BLOCK_SIZE, std::ios::beg);
                    for (int j = 0; j < BLOCK_SIZE; ++j) {
                        fileSystem.put('\0');
                    }
                    freeCluster(index);
                }
            }
        }
        freeInode(fileInFs.nodeid);
    } else{
        fileInFs.references--;
    }


    for (int i = 2*sizeof(DirectoryItem); i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *)& dirItem, sizeof(DirectoryItem));
        if (file == dirItem.item_name){
            auto emptyDir = DirectoryItem {};
            fileSystem.seekp(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.write((char *)&emptyDir, sizeof(DirectoryItem));
            fileSystem.flush();
            break;
        }
    }

    std::cout << "OK" << std::endl;
    currentInode = currentHolder;
}

void FileSystem::ln(const std::string &fileInFileSystem, const std::string &hardLinkFile) {
    std::string fileName, fileDir, hlName, hlDir;
    fileName = getFileFromPath(fileInFileSystem);
    fileDir = getDirectoriesFromPath(fileInFileSystem);
    hlName = getFileFromPath(hardLinkFile);
    hlDir = getDirectoriesFromPath(hardLinkFile);

    if ((hlName == ".") || (hlName == "..")){
        std::cout << "INVALID NAME" << std::endl;
        return;
    }

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if (!changeDirectory(fileDir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    long fileIndex = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (fileName == dirItem.item_name){
            fileIndex = dirItem.inode;
            break;
        }
    }
    if (fileIndex == -1 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return;
    }
    auto fileInFS = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + fileIndex * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) & fileInFS, sizeof(PsInode));

    if (fileInFS.isDirectory){
        std::cout << "DIRECTORIES CANNOT HAVE HARD LINK" << std::endl;
        currentInode = currentHolder;
        return;
    }

    currentInode = currentHolder;
    currentHolder = currentInode;

    if(!changeDirectory(hlDir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (hlName == dirItem.item_name){
            std::cout << "EXISTS" << std::endl;
            currentInode = currentHolder;
            return;
        }
    }

    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (dirItem.item_name[0] == 0){
            auto newHLink = DirectoryItem {};
            newHLink.inode = fileInFS.nodeid;
            strcpy(newHLink.item_name, hlName.c_str());
            fileSystem.seekp(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.write((char *) &newHLink, sizeof(DirectoryItem));
            fileSystem.flush();
            break;
        }
    }

    fileInFS.references++;
    fileSystem.seekp(superBlock->inode_start_address + fileInFS.nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.write((char *) &fileInFS, sizeof(PsInode));
    fileSystem.flush();

    currentInode = currentHolder;
    std::cout << "OK" << std::endl;
}

bool FileSystem::cp(const std::string &currentPath, const std::string &newFilePath) {
    std::string oldFile, oldDir, newFile, newDir;
    oldFile = getFileFromPath(currentPath);
    oldDir = getDirectoriesFromPath(currentPath);
    newFile = getFileFromPath(newFilePath);
    newDir = getDirectoriesFromPath(newFilePath);

    auto currentHolder = PsInode {};
    currentHolder = currentInode;

    if(!changeDirectory(oldDir)){
        std::cout << "FILE NOT FOUND" << std::endl;
        return false;
    }
    long index = -1;
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (oldFile == dirItem.item_name){
            index = dirItem.inode;
            break;
        }
    }
    if (index == -1 ){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return false;
    }
    auto fileInFs = PsInode {};
    fileSystem.seekg(superBlock->inode_start_address + index * sizeof(PsInode), std::ios::beg);
    fileSystem.read((char *) &fileInFs, sizeof(PsInode));
    if (fileInFs.isDirectory){
        std::cout<< "FILE NOT FOUND" << std::endl;
        currentInode = currentHolder;
        return false;
    }
    std::string content = getFileContent(fileInFs);

    currentInode = currentHolder;
    currentHolder = currentInode;

    if(!changeDirectory(newDir)){
        std::cout << "PATH NOT FOUND" << std::endl;
        return false;
    }
    for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
        auto dirItem = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
        if (newFile == dirItem.item_name){
            std::cout << "EXISTS" << std::endl;
            currentInode = currentHolder;
            return false;
        }
    }

    createFile(newFile, content, fileInFs.file_size);
    currentInode = currentHolder;
    return true;
}


void FileSystem::load(const std::string &path) {
    std::ifstream inputStream;
    inputStream.open(path);
    if (inputStream.is_open()){
        std::string line;
        while (std::getline(inputStream, line)){
            Commands::commandController(line);
        }
        inputStream.close();
        std::cout << "OK" <<std::endl;
    }else{
        std::cout << "FILE NOT FOUND" << std::endl;
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

        auto oldInode = PsInode {};
        oldInode = currentInode;
        if(!changeDirectory(dir)){
            std::cout << "PATH NOT FOUND" << std::endl;
            inputStream.close();
            return;
        }
        for (int i = 0; i < BLOCK_SIZE; i+=sizeof(DirectoryItem)) {
            auto dirItem = DirectoryItem {};
            fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE, std::ios::beg);
            fileSystem.read((char *) &dirItem, sizeof(DirectoryItem));
            if (dirItem.item_name == file){
                std::cout << "EXISTS" << std::endl;
                currentInode = oldInode;
                inputStream.close();
                return;
            }
        }

        inputStream.seekg(0, std::ios::end);
        unsigned long fileSize = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);

        char content[fileSize];
        inputStream.read((char*)content, fileSize);
        inputStream.close();
        std::string strContent;
        for(auto& byte : content){
            strContent += byte;
        }
        createFile(file, strContent,fileSize);
        currentInode = oldInode;
        reloadCurrentInode();
        std::cout << "OK" << std::endl;
    }else{
        std::cout<< "FILE NOT FOUND" << std::endl;
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
    }else if((size*multiply)< (KILOBYTE*50)) {
        std::cout << "FS minimum size is 50 KB" << std::endl;
        return 0;
    }

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
void FileSystem::createFile(std::string &fileName, const std::string& content, unsigned long size) {
    unsigned long clustersNeeded = (size/(BLOCK_SIZE));
    if (size % BLOCK_SIZE != 0){
        clustersNeeded ++;
    }
    unsigned int indirectNeeded = 0;
    unsigned long numberOfAddInIndirect = superBlock->cluster_size / sizeof(unsigned int);

    if (clustersNeeded <= 5 ){
    }else if (clustersNeeded > 5 && clustersNeeded < numberOfAddInIndirect+5){
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


    std::vector<std::string> contentForClusters;
    for (size_t index = 0; index < content.length(); index += superBlock->cluster_size) {
        contentForClusters.push_back(content.substr(index, superBlock->cluster_size));
    }

    for(unsigned long i = 0; i<freeClusterAddr.size()-indirectNeeded; i++){
        fileSystem.seekp(superBlock->data_start_address + freeClusterAddr.at(i) * superBlock->cluster_size, std::ios::beg);
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
        fileSystem.seekp(superBlock->data_start_address + newInode.inDirect[0] * superBlock->cluster_size, std::ios::beg);
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
        fileSystem.seekp(superBlock->data_start_address + newInode.inDirect[0] * superBlock->cluster_size, std::ios::beg);
        for (unsigned int clusterIndex = 5 ; clusterIndex < numberOfAddInIndirect + 5 ; clusterIndex++) {
            fileSystem.write(reinterpret_cast<const char *>(&freeClusterAddr.at(clusterIndex)), sizeof(freeClusterAddr.at(clusterIndex)));
            fileSystem.flush();
        }
        fileSystem.seekp(superBlock->data_start_address + newInode.inDirect[1] * superBlock->cluster_size, std::ios::beg);
        for (unsigned int clusterIndex = numberOfAddInIndirect + 5 ; clusterIndex < freeClusterAddr.size()-indirectNeeded ; clusterIndex++) {
            fileSystem.write(reinterpret_cast<const char *>(&freeClusterAddr.at(clusterIndex)), sizeof(freeClusterAddr.at(clusterIndex)));
            fileSystem.flush();
        }

    } else{
        std::cerr << "Error occurred while filling file with data" << std::endl;
        return;
    }

    fileSystem.seekp(superBlock->inode_start_address + newInode.nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.write((char *) &newInode, sizeof(newInode));
    fileSystem.flush();
    DirectoryItem newDir{};
    newDir.inode = newInode.nodeid;
    strcpy(newDir.item_name, fileName.c_str());


    for(long i = 2*sizeof(DirectoryItem); i<BLOCK_SIZE ; i+= sizeof(DirectoryItem) ){
        auto tmpDir = DirectoryItem {};
        fileSystem.seekg(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
        fileSystem.read((char *) & tmpDir, sizeof(DirectoryItem));
        if (tmpDir.item_name[0] == 0){
            fileSystem.seekp(superBlock->data_start_address + currentInode.direct[0] * BLOCK_SIZE + i, std::ios::beg);
            fileSystem.write((char *) &newDir, sizeof(newDir));
            fileSystem.flush();
            break;
        }

    }


    currentInode.file_size += sizeof(DirectoryItem);
    fileSystem.seekp(superBlock->inode_start_address + currentInode.nodeid * sizeof(PsInode), std::ios::beg);
    fileSystem.write((char *) &currentInode, sizeof(PsInode));
    fileSystem.flush();
}

bool FileSystem::changeDirectory(std::string path){
    if (path.empty()){
        return true;
    }
    bool isAbsolute = path.at(0)=='/';

    std::vector<std::string> tokens;
    size_t index;
    while((index = path.find('/')) != std::string::npos){
        tokens.push_back(path.substr(0, index));
        path.erase(0,index+1);
    }
    if (!path.empty()){
        tokens.push_back(path);
    }
    auto inode = PsInode {};
    if (isAbsolute){
        fileSystem.seekg(superBlock->inode_start_address, std::ios::beg);
        fileSystem.read((char *) &inode, sizeof(PsInode));
    }else {
        inode = currentInode;
    }
    bool tokenFound;
    for(auto &token : tokens){
        tokenFound = false;
        for (unsigned int i = 0; i < BLOCK_SIZE; i+= sizeof(DirectoryItem)) {
            auto dirItem = DirectoryItem {};
            fileSystem.seekg(superBlock->data_start_address + inode.direct[0] * BLOCK_SIZE + i);
            fileSystem.read((char *)& dirItem, sizeof(DirectoryItem));
            if (dirItem.item_name == token){
                fileSystem.seekg(superBlock->inode_start_address + dirItem.inode * sizeof(PsInode), std::ios::beg);
                fileSystem.read((char *) &inode, sizeof(PsInode));
                tokenFound = true;
                if (!inode.isDirectory){
                    return false;
                }
                break;
            }
        }
        if (!tokenFound){
            return false;
        }
    }
    if(isAbsolute){
        std::string newPath;
        newPath += "/";
        for (auto & token: tokens) {
            newPath += token;
            newPath += "/";
        }
        pwdPath = newPath;
    }else{
        for (auto & token: tokens) {
            if (token == "."){
                continue;
            }else if (token == ".."){
                if (pwdPath == "/"){

                }else{
                    pwdPath = pwdPath.substr(0, pwdPath.size()-1);
                    size_t tmpIndex = pwdPath.find_last_of('/');
                    pwdPath = pwdPath.substr(0,tmpIndex+1);
                }
            } else{
                pwdPath += token;
                pwdPath += "/";
            }

        }
    }

    currentInode = inode;
    return true;
}
unsigned int FileSystem::getFreeInode() {
    unsigned char currentByte;
    fileSystem.seekg(superBlock->bitmapi_start_address, std::ios::beg);
    for (unsigned int ixByte= 0; ixByte < superBlock->inode_count/8 ; ixByte++) {
        currentByte = fileSystem.get();
        for (int ixBit = 7; ixBit>=0 ; ixBit--) {
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
    for (unsigned int ixByte = 0; ixByte < superBlock->cluster_count/8; ixByte++) {
        currentByte = fileSystem.get();
        for (int ixBit = 7; ixBit >= 0 ; ixBit--) {
            if (!((currentByte >> ixBit) & 1U)){
                fileSystem.seekg(-1, std::ios::cur);
                currentByte |= 1UL << ixBit;
                fileSystem.put(currentByte);
                fileSystem.flush();
                addresses.push_back((ixByte * 8) + (7-ixBit));
                if (addresses.size() == clustersNeeded){
                    return addresses;
                }
            }
        }
    }

    return addresses;
}

void FileSystem::freeInode(unsigned long inodeIndex) {
    unsigned long byteIndex = (inodeIndex / 8);
    // bit orders are reversed
    unsigned int bitIndex = 7 - (inodeIndex - (byteIndex * 8));
    unsigned char bmp;

    fileSystem.seekg(superBlock->bitmapi_start_address + byteIndex, std::ios::beg);
    fileSystem >> bmp;
    // inserts 1 on desired bit
    bmp &= ~(1LU << bitIndex);
    fileSystem.seekg(-1, std::ios::cur);
    fileSystem << bmp;
}

void FileSystem::freeCluster(unsigned long clusterIndex) {
    unsigned long byteIndex = (clusterIndex / 8);
    // bit shifting is reversed
    unsigned int bitIndex = 7 - (clusterIndex - (byteIndex * 8));
    unsigned char bmp;

    fileSystem.seekg(superBlock->bitmap_start_address + byteIndex, std::ios::beg);
    fileSystem >> bmp;
    // inserts 1 on desired bit
    bmp &= ~(1LU << bitIndex);
    fileSystem.seekg(-1, std::ios::cur);
    fileSystem << bmp;
}


void FileSystem::reloadCurrentInode() {
    fileSystem.seekg(superBlock->inode_start_address + currentInode.nodeid * sizeof(PsInode), std::ios::beg);
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
    std::cout << content << std::endl;
    return content;
}





















