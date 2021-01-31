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

#define BLOCK_SIZE 1024

#define DATA_BLOCK_COUNT_DEF 5

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
    unsigned long direct[5] = {0,0,0,0,0};                // 1. přímý odkaz na datové bloky
    unsigned long inDirect[2] = {0,0};
} PsInode;

typedef struct DIRECTORY_ITEM {
    unsigned int inode;                   // inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
} DirectoryItem;


class FileSystem{
public:
    static std::unique_ptr<PsInode> currentInode;
    static std::string systemName;

    static bool formatSystem(const std::string& systemSize);
    static bool loadSystem();
    static bool isSystemRunning();

    static void incp(const std::string &file, const std::string &basicString);
private:
    static std::string getDirectoriesFromPath(const std::string & path);
    static std::string getFileFromPath(const std::string & path);
    static std::unique_ptr<SuperBlock> superBlock;
    static std::fstream fileSystem;
    static unsigned long parseSize(const std::string &systemSize);
    static std::vector<unsigned long> getFreeClusters(unsigned long clustersNeeded);
    static bool initSuperBlock(unsigned long i, unsigned long count, unsigned long count1);


    static void createFile(PsInode *parent, std::string &fileName, const std::string& content, unsigned long size);

    static unsigned long getFreeInode();
};
#endif //ZOS_SEM_2020_FILESYSTEM_H
