//
// Created by mbrozek on 27.12.20.
//

#ifndef ZOS_SEM_2020_FILESYSTEM_H
#define ZOS_SEM_2020_FILESYSTEM_H

#include <string>
#include <iostream>
#include <fstream>
#include <memory>

#define GIGABYTE 1000000
#define MEGABYTE 1000
#define KILOBYTE 1

#define BLOCK_SIZE 4096


typedef struct SUPERBLOCK {
    char signature[9];              //login autora FS
    char volume_descriptor[251];    //popis vygenerovaného FS
    unsigned long disk_size;              //celkova velikost VFS
    unsigned long cluster_size;           //velikost clusteru
    unsigned long cluster_count;          //pocet clusteru
    unsigned long inode_count;  //pocet inode uzlu
    unsigned long bitmapi_start_address;  //adresa pocatku bitmapy i-uzlů
    unsigned long bitmap_start_address;   //adresa pocatku bitmapy datových bloků
    unsigned long inode_start_address;    //adresa pocatku  i-uzlů
    unsigned long data_start_address;     //adresa pocatku datovych bloku
} SuperBlock;

typedef struct PSEUDO_INODE {
    unsigned int nodeid;                 //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
    bool isDirectory;               //soubor, nebo adresar
    unsigned int references;              //počet odkazů na i-uzel, používá se pro hardlinky
    unsigned long file_size;              //velikost souboru v bytech
    unsigned int direct[5] = {0,0,0,0,0};                // 1. přímý odkaz na datové bloky
    unsigned int inDirect[2] = {0,0};
} PsInode;

typedef struct DIRECTORY_ITEM {
    unsigned int inode;                   // inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
} DirectoryItem;


class FileSystem{
public:
    static PsInode currentInode;
    static std::string systemName;

    static bool formatSystem(const std::string& systemSize);

    static bool loadSystem();

    static bool isSystemRunning();

    static void incp(const std::string &file, const std::string &basicString);

    static void outcp(const std::string &fsFile, const std::string &outFile);

    static void mkdir(const std::string &path);


    static bool cd(std::string path);

    static void pwd();

    static void cat(const std::string &path);

    static void ls(const std::string &path);

    static bool cp(const std::string &currentPath, const std::string &newFilePath);

    static void rm(const std::string &path);

    static void mv(const std::string &oldPath, const std::string &newPath);

    static void info(const std::string &path);

    static void rmdir(const std::string &path);

    static void load(const std::string &path);

    static void ln(const std::string &fileInFileSystem, const std::string &hardLinkFile);

private:
    static std::string pwdPath;
    static std::string getDirectoriesFromPath(const std::string & path);
    static std::string getFileFromPath(const std::string & path);
    static std::unique_ptr<SuperBlock> superBlock;
    static std::fstream fileSystem;

    static unsigned long parseSize(const std::string &systemSize);
    static std::vector<unsigned int> getFreeClusters(unsigned long clustersNeeded);
    static bool initSuperBlock(unsigned long i, unsigned long count, unsigned long count1);


    static void createFile(std::string &fileName, const std::string& content, unsigned long size);

    static unsigned int getFreeInode();

    static void reloadCurrentInode();

    static std::string getFileContent(const PsInode &inode);

    static void freeInode(unsigned long inodeIndex);

    static void freeCluster(unsigned long clusterIndex);

    static bool changeDirectory(std::string path);


};
#endif //ZOS_SEM_2020_FILESYSTEM_H
