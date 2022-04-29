/*
University of Texas at Dallas--Computer Science Department
CS 4348.005 Operating Systems Concepts Spring 2022
Project 2 Part 1

Members:
Haniya Zafar
Nikita Istomin
Zubair Shaik

Contributions: 
Zubair: worked on the add_free_block(), and get_free_block(), and main() functions.
Nikita: worked on add_free_block() and initfs() functions + debugging.
Haniya: worked on add_free_block() and initfs() functions. 

*/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#define BLOCK_SIZE 1024
#define INODE_SIZE 64
#define INDIRECT_SIZE 256 // 256 integers in the indirect block

typedef struct
{
    int isize;
    int fsize;
    int nfree;
    unsigned int free[200];
    char flock;
    char ilock;
    char fmod;
    unsigned int time;
} superblock_type;

typedef struct
{
    unsigned short flags;
    unsigned short nlinks;
    unsigned int uid;
    unsigned int gid;
    unsigned int size0;
    unsigned int size1;
    unsigned int addr[9];
    unsigned int actime;
    unsigned int modtime;
} inode_type;

typedef struct
{
    unsigned int inode;
    char filename[28];
} dir_type; // 32 Bytes long

superblock_type superBlock;
inode_type root;
int fd;
int iNode;
int zeroArr[256];
int chainArr[256];


//function declarations
int open_fs(char *file_name);
void inode_writer(int inum, inode_type inode);
inode_type inode_reader(int inum, inode_type inode);

void fill_root_and_write(int inodeBlocks);
int add_free_block(int address);
int get_free_block();
int get_free_block();
int initfs(char* fileName, int blocks, int inodeBlocks);
int openFileSystem(char* system);

int cpin(char* externalFile, char* v6File);
void cpout(char * sourcePath, char* destinationPath);
void rm(char * fileName);

int findFileInDirBlock(char* filename, int block);
int getNextFileBlock(inode_type inode, int* save);
int findFileInRoot(char* filename);
int getLastFileBlock(int inodeNum);
off_t fileSize(const char *filename);
void quit();

int open_fs(char *file_name)
{
    fd = open(file_name, O_RDWR | O_CREAT, 0600);

    if (fd == -1)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

// Function to write inode
void inode_writer(int inum, inode_type inode)
{

    lseek(fd, 2 * BLOCK_SIZE + (inum - 1) * INODE_SIZE, SEEK_SET);
    write(fd, &inode, sizeof(inode));
}

// Function to read inodes
inode_type inode_reader(int inum, inode_type inode)
{
    lseek(fd, 2 * BLOCK_SIZE + (inum - 1) * INODE_SIZE, SEEK_SET);
    read(fd, &inode, sizeof(inode));
    return inode;
}

// Function to write inode number after filling some fileds
void fill_root_and_write(int inodeBlocks)
{
    inode_type root;
    int i;

    root.flags |= 1 << 15; // Root is allocated
    root.flags |= 1 << 14; // It is a directory
    root.actime = time(NULL);
    root.modtime = time(NULL);

    root.size0 = 0;
    root.size1 = 2 * sizeof(dir_type);
    root.addr[0] = 2 + inodeBlocks; // first data block
    for (i = 1; i < 9; i++)
        root.addr[i] = -1; // all other addr elements are null so set to -1
    inode_writer(1, root);
}

//takes the address of a block to be added to the free list and returns 1 if all went well and -1 if something went wrong.
int add_free_block(int address) {
    if(superBlock.nfree == 200){
        lseek(fd, address * BLOCK_SIZE, SEEK_SET);
        write(fd, &superBlock.nfree, sizeof(int));
        write(fd, superBlock.free, sizeof(int) * 200);
        superBlock.nfree = 0;
        lseek(fd, BLOCK_SIZE, SEEK_SET);
        write(fd, &superBlock, BLOCK_SIZE);
    }
    if (superBlock.nfree < 200) {
        superBlock.free[superBlock.nfree] = address;
        superBlock.nfree++;
        lseek(fd, BLOCK_SIZE, SEEK_SET);
        write(fd, &superBlock, BLOCK_SIZE);
        return 1;
    } else {
        return -1;
    }
}

//returns the address of a free data block (from the free list) if everything is good and returns -1 should there be an error somewhere (say no free blocks left)
int get_free_block() {
    superBlock.nfree--;
    if (superBlock.nfree > 0) {
        if (superBlock.free[superBlock.nfree] == 0) {
            return -1;
        } else {
            return superBlock.free[superBlock.nfree];
        }
    } else {
        //nfree is 0 here
        int s = superBlock.free[0];
        lseek(fd, BLOCK_SIZE * s, SEEK_SET);
        read(fd, &chainArr, sizeof(chainArr));
        superBlock.nfree = chainArr[0] - 1;
        int i;
        for(i = 0; i < 250; i++) {
            superBlock.free[i] = chainArr[i];
        }
        superBlock.free[superBlock.nfree] = s;
        return superBlock.free[superBlock.nfree];
    }
}

/*
initfs file_name n1 n2 Here, file_name is the name of the file in the native Unix machine (where you are running your program) that represents the disk drive.
where n1 is the file system size in number of blocks and n2 is the number of blocks devoted to the i-nodes.
In this case, set all data blocks free,except for one data block for storing the contents of i-node number 1,
representing the root, which has the two entries . and ..
All i-nodes except i-node number 1 are (unallocated) set to free.
Make sure that all free blocks are accessible from free[] array of the super block.
One of the data blocks contains the root directory's contents (two entries . and ..)
*/
int initfs(char* fileName, int blocks, int inodeBlocks) {

    // Open/create file that will be our file system
    if(open_fs(fileName) == -1){
        printf("Failed to open file %s\n", fileName);
    }
    else{
        printf("Successfuly opened file %s\n", fileName);
    }

    // Set all file blocks to 0
    char buf[BLOCK_SIZE];
    memset(buf,0,BLOCK_SIZE);
    lseek(fd, 0, SEEK_SET);
    int i;
    for(i = 0; i < blocks; i++){
        write(fd, buf, BLOCK_SIZE);
    }

    // Initialize the super block
    superBlock.isize = inodeBlocks;
    superBlock.fsize = blocks;
    superBlock.nfree = 1;
    superBlock.free[0] = 0;
    // superBlock.flock;
    // superBlock.ilock;
    // superBlock.fmod;
    superBlock.time = time(NULL);
    lseek(fd, BLOCK_SIZE, SEEK_SET);
    write(fd, &superBlock, BLOCK_SIZE);

    //Initialize free blocks
    int freeBlock; // bootBlock + superBlock + inodeBlocks + rootDirectoryBlock = first free block
    for (freeBlock = 2 + inodeBlocks + 1; freeBlock < blocks; freeBlock++) {
        add_free_block(freeBlock);
    }

    // Initialize i-node for the root directory
    fill_root_and_write(inodeBlocks);

    // Initialize root directory
    dir_type directory[2];
    directory[0].inode = 1;
    strcpy(directory[0].filename,".");

    directory[1].inode = 1;
    strcpy(directory[1].filename,"..");

    lseek(fd, (2 + inodeBlocks) * BLOCK_SIZE, SEEK_SET);
    write(fd, directory, 2*sizeof(dir_type));
    iNode = 0;
    // Print initialization message
    printf("File system initialized in \"%s\"\n\n", fileName);
    return 1;
}

void cpout(char * sourcePath, char* destinationPath) {
    char buf[BLOCK_SIZE] = {0};
    dir_type direct[100];
    inode_type currInode;
    int fS, y;
    if ((fS = open(sourcePath, O_RDWR | O_CREAT, 0600)) == -1) {
        printf("Error opening file ");
        return;
    } 
    int blockNum = (iNode * INODE_SIZE) / BLOCK_SIZE;
    int off = (iNode * INODE_SIZE) % BLOCK_SIZE;
    lseek(fd,(BLOCK_SIZE * blockNum) + off, SEEK_SET);
    read(fd,&currInode,INODE_SIZE);
    int currBlockNum = currInode.addr[0];
    lseek(fd,(BLOCK_SIZE * currBlockNum), SEEK_SET);
    read(fd,direct,currInode.size0);
    for (int i = 0; i < currInode.size0 / sizeof(dir_type); i++) {
        if (strcmp(destinationPath, direct[i].filename) == 0) {
            inode_type currFile;
            int blockNum = (direct[i].inode * INODE_SIZE) / BLOCK_SIZE;
            int off = (direct[i].inode * INODE_SIZE) % BLOCK_SIZE;
            lseek(fd,(BLOCK_SIZE * blockNum) + off, SEEK_SET);
            read(fd,&iNode,INODE_SIZE);
            unsigned int * s = currFile.addr;
            if (currFile.flags == (1 << 15)) {
                for (y = 0; y < currFile.size0/BLOCK_SIZE; y++) {
                    blockNum = s[y];
                    lseek(fd,(BLOCK_SIZE * blockNum), SEEK_SET);
                    read(fd,buf,BLOCK_SIZE);
                    write(fS, buf,BLOCK_SIZE);
                }
                blockNum = s[y];
                lseek(fd,(BLOCK_SIZE * blockNum), SEEK_SET);
                read(fd,buf,currFile.size0 % BLOCK_SIZE);
                write(fS, buf, currFile.size0 % BLOCK_SIZE);
            } else {
                printf("no file \n");
            }
            return;
        }
    }
}


/**
 * @brief Returns an inode number of the file filename in the active v6 file system if found in the small directory block.
 *        Else returns -1.
 * 
 * @param filename 
 * @param block 
 * @return int 
 */
int findFileInDirBlock(char* filename, int block)
{
    dir_type dir;
    int dirCount;
    for(dirCount = 0; dirCount < 32; ++dirCount){ //32 directory entries in a block
        //read in the directory entry
        lseek(fd, (block * BLOCK_SIZE) + (dirCount * sizeof(dir_type)), SEEK_SET);
        read(fd, &dir, sizeof(dir_type));

        if(dir.inode < 1){//reached the last writen directory entry
            return -1;
        }

        //compare the filename to the file in the directory entry
        if(strcmp(dir.filename, filename) == 0){
            return dir.inode;
        }
    }
}


/**
 * @brief Returns next block number of given directory. If there's no next block, returns -1.
 *          save = [setup = {1 - ready}, size = {0 - small, 1 - medium, 2 - long, 3 - superlong}, blockNumber]
 * 
 * @param inode 
 * @param save 
 * @return int 
 */
int getNextFileBlock(inode_type inode, int* save){
    // printf("########## Starting getNextFileBlock() execution ##########\n");
    if(save[0] != 1){ //not setup
        // printf("\tSave array is not setup. Setting up...\n");
        save[0] = 1;
        save[2] = 0;
        //find size of the directory
        if((inode.flags & (1 << 12)) == 0 && (inode.flags & (1 << 11)) == 0) //if small (00)
        {
            save[1] = 0;
        }
        else if ((inode.flags & (1 << 12)) == 0 && (inode.flags & (1 << 11)) == (1 << 11)) //if medium (01)
        {
            save[1] = 1;
        }
        else if ((inode.flags & (1 << 12)) == (1 << 12) && (inode.flags & (1 << 11)) == 0) //if long (10)
        {
            save[1] = 2;
        }
        else //if super long (11)
        {
            save[1] = 3;
        }
        // printf("\tSave array is setup.\n");
    }


    if(save[1] == 0) { //if small (00)
        if(save[2] >= 9){
            save[0] = -1;
            // printf("\tLast block reached.\n");
            // printf("########## Ending getNextFileBlock() execution ##########\n");
            return -1;
        }
        int blockNum = inode.addr[save[2]];
        save[2]++;
        // printf("\tReturning next block.\n");
        // printf("########## Ending getNextFileBlock() execution ##########\n");
        return blockNum;
    }
    else if(save[1] == 0) { //if medium (01)
        if(save[2] >= 9 * INDIRECT_SIZE){
            save[0] = -1;
            // printf("\tLast block reached.\n");
            // printf("########## Ending getNextFileBlock() execution ##########\n");
            return -1;
        }
        int firstIndirect = save[2] / INDIRECT_SIZE; //which index in addr[]
        int offset = save[2] % INDIRECT_SIZE; //which int at a single indirect block
        int blockNum;

        lseek(fd, (inode.addr[firstIndirect] * BLOCK_SIZE) + (offset * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        save[2]++;
        // printf("\tReturning next block.\n");
        // printf("########## Ending getNextFileBlock() execution ##########\n");
        return blockNum;
    }
    else if(save[1] == 0) { //if long (10)
        if(save[2] >= 9 * INDIRECT_SIZE * INDIRECT_SIZE){
            save[0] = -1;
            // printf("\tLast block reached.\n");
            // printf("########## Ending getNextFileBlock() execution ##########\n");
            return -1;
        }
        int firstIndirect = (save[2] / INDIRECT_SIZE) / INDIRECT_SIZE; //which index in addr[]
        int secondIndirect = (save[2] / INDIRECT_SIZE) % INDIRECT_SIZE; //offset at first indirect
        int offset = save[2] % INDIRECT_SIZE; //which int at a single indirect block
        int blockNum;

        lseek(fd, (inode.addr[firstIndirect] * BLOCK_SIZE) + (secondIndirect * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        lseek(fd, (blockNum * BLOCK_SIZE) + (offset * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        save[2]++;
        // printf("\tReturning next block.\n");
        // printf("########## Ending getNextFileBlock() execution ##########\n");
        return blockNum;
    }
    else { //if super long (11)
        if(save[2] >= 9 * INDIRECT_SIZE * INDIRECT_SIZE * INDIRECT_SIZE){
            save[0] = -1;
            // printf("\tLast block reached.\n");
            // printf("########## Ending getNextFileBlock() execution ##########\n");
            return -1;
        }
        int firstIndirect = ((save[2] / INDIRECT_SIZE) / INDIRECT_SIZE) / INDIRECT_SIZE; //which index in addr[]
        int secondIndirect = ((save[2] / INDIRECT_SIZE) / INDIRECT_SIZE) % INDIRECT_SIZE; //offset at first indirect
        int thirdIndirect = (save[2] / INDIRECT_SIZE) % INDIRECT_SIZE; //offset at second indirect
        int offset = save[2] % INDIRECT_SIZE; //which int at a single indirect block
        int blockNum;

        lseek(fd, (inode.addr[firstIndirect] * BLOCK_SIZE) + (secondIndirect * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        lseek(fd, (blockNum * BLOCK_SIZE) + (thirdIndirect * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        lseek(fd, (blockNum * BLOCK_SIZE) + (offset * sizeof(int)), SEEK_SET);
        read(fd, &blockNum, sizeof(int));

        save[2]++;
        // printf("\tReturning next block.\n");
        // printf("########## Ending getNextFileBlock() execution ##########\n");
        return blockNum;
    }
}


/**
 * @brief Returns an inode number of the file filename in the active v6 file system if found in the root.
 *        Else returns -1.
 * 
 * @param filename 
 * @return inode number 
 */
int findFileInRoot(char* filename)
{
    // printf("########## Starting findFileInRoot() execution ##########\n\n");
    //read in first inode(it points to the root directory)
    root = inode_reader(1,root);
    int save[] = {0,0,0};
    int blockNum;
    // printf("\tSetup complete.\n");

    while((blockNum = getNextFileBlock(root, save)) != -1)
    {
        // printf("\t\tLoking through a file block %i\n", blockNum);
        int fileInode = findFileInDirBlock(filename, blockNum); //look for filename in block
        if(fileInode != -1){
            // printf("\tFile found.\n");
            // printf("\n########## Ending findFileInRoot() execution ##########\n");
            return fileInode;
        }
    }

    // printf("\tNo file found.\n");
    // printf("\n########## Ending findFileInRoot() execution ##########\n");
    return -1;
}


/**
 * @brief Get the last file block of a given inode file 
 * 
 * @param inodeNum 
 * @return int 
 */
int getLastFileBlock(int inodeNum)
{
    inode_type inode;
    inode = inode_reader(inodeNum, inode);
    int save[] = {0,0,0};
    int lastBlockNum = -1;
    int currentBlockNum = -1;

    do
    {
        lastBlockNum = currentBlockNum;
    } while((currentBlockNum = getNextFileBlock(inode, save)) != -1);

    return lastBlockNum;
}

/**
 * @brief returns the size of external file filename
 * 
 * @param filename 
 * @return off_t 
 */
off_t fileSize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    printf("Cannot determine size of %s \n", filename);

    return -1;
}


int cpin(char* externalFile, char* v6File)
{
    printf("########## Starting cpin execution ##########\n\n");
    //check if file name fits in the directory entry
    if(sizeof(v6File) / sizeof(char) > 28){
        printf("ERROR: v6 filename is too long, choose a shorter name.\n");
        printf("########## Ending cpin execution ##########\n\n");
        return EXIT_FAILURE;
    }
    printf("\tChecked that given new filename is acceptable.\n");

    // open an external file
    int externalFileFD;
    if ((externalFileFD = open(externalFile, O_RDONLY)) == -1) {
        printf("ERROR: Error opening file.\n");
        printf("\n########## Ending cpin execution ##########\n\n");
        return EXIT_FAILURE;
    } 
    lseek(externalFileFD, 0, SEEK_SET);
    printf("\tSuccesfully oppened the external file.\n\n");

    // look for the v6File in the root directory
    // if it exists -> remove it
    printf("\tLooking for internal file matching the new filename...\n");
    unsigned int inodeNum; //container for the inode number 
    // int override = 0;
    if((inodeNum = findFileInRoot(v6File)) != -1){
        printf("\t\tFound a file. Removing it...\n");
        rm(v6File); //remove the existing file
        printf("\t\tFile removed.\n\n");
    }
    else{
        printf("\t\tNo file found.\n\n");
    }

    // create an inode

    printf("\tCreating a new i-node...\n");
    lseek(fd, BLOCK_SIZE, SEEK_SET);
    read(fd, &superBlock, BLOCK_SIZE);

    printf("\t\tLooking for a free i-node number...\n");
    inode_type inode;
    for(inodeNum = 2; inodeNum < superBlock.isize * (BLOCK_SIZE / INODE_SIZE); ++inodeNum) //loop over all inode numbers until we find an unallocated inode
    {
        printf("\t\tChecking i-node #%i\n", inodeNum);
        inode = inode_reader(inodeNum, inode);
        // printf("\t\t\tinode.flags = %i\n",inode.flags);
        if((inode.flags & (1 << 15)) == 0){
            printf("\t\tFound a free i-node.\n\n");
            break; //found unallocated inode
        }
    }
    if(inodeNum >= superBlock.isize * (BLOCK_SIZE / INODE_SIZE)){
        printf("ERROR: Error copping the file. No more free i-nodes. \n");
        printf("\n########## Ending cpin execution ##########\n\n");
        return EXIT_FAILURE;
    }

    // setup the inode
    printf("\tSetting up a new i-node...\n");
    inode.flags |= 1 << 15; //inode allocated
    printf("\t\ti-node is allocated.\n");

    //figure out how big the file is...
    off_t fSize = fileSize(externalFile);
    inode.size1 = fSize;
    printf("\t\tNumerical file size set.\n");
    if(fSize > (sizeof(root.addr) / sizeof(int)) * (BLOCK_SIZE / sizeof(int)) ){ //long / super long 
        inode.flags |= 1 << 12;
        if(fSize > (sizeof(root.addr) / sizeof(int)) * (BLOCK_SIZE / sizeof(int)) * (BLOCK_SIZE / sizeof(int))){ //super long
            inode.flags |= 1 << 11;
            printf("\t\tFIle size is set to be SUPER LONG.\n");
        }
        else{
            printf("\t\tFIle size is set to be LONG.\n");
        }
    }
    else{ // small / medium
        if(fSize > (sizeof(root.addr) / sizeof(int))){ // medium
            inode.flags |= 1 << 11;
            printf("\t\tFIle size is set to be MEDIUM.\n");
        }
        else{
            printf("\t\tFIle size is set to be SMALL.\n");
        }
    }

    int i;
    for (i = 0; i < 9; i++)
        inode.addr[i] = -1; // all addr elements are null so set to -1
    printf("\t\tContents of addr array set to -1\n");
    inode_writer(inodeNum, inode);
    printf("\ti-node setup done.\n\n");


    // get free blocks as needed and copy the contents of the external file into the new v6 file
    printf("\tCopying contents of the external file into the internal file...\n");
    char* blockBuf[BLOCK_SIZE];
    int blockCount;
    int blockCountMax = (inode.size1 % BLOCK_SIZE == 0) ? (inode.size1 / BLOCK_SIZE) : ((inode.size1 / BLOCK_SIZE) + 1);
    if((inode.flags & (1 << 12)) == 0 && (inode.flags & (1 << 11)) == 0) //if small (00)
    {
        for(blockCount = 0; blockCount < blockCountMax; ++blockCount)
        {
            int freeBlock = get_free_block(); //get a free block

            read(externalFileFD, &blockBuf, BLOCK_SIZE); //read a block from external file
            lseek(fd, freeBlock * BLOCK_SIZE, SEEK_SET);
            write(fd, &blockBuf, BLOCK_SIZE); //write a block internaly

            inode.addr[blockCount] = freeBlock;
        }
    }
    else if ((inode.flags & (1 << 12)) == 0 && (inode.flags & (1 << 11)) == (1 << 11)) //if medium (01)
    {
        for(blockCount = 0; blockCount < blockCountMax; ++blockCount)
        {
            int freeBlock = get_free_block(); //get a free block

            read(externalFileFD, &blockBuf, BLOCK_SIZE); //read a block from external file
            lseek(fd, freeBlock * BLOCK_SIZE, SEEK_SET);
            write(fd, &blockBuf, BLOCK_SIZE); //write a block internaly

            int addrIndex = blockCount / INDIRECT_SIZE;
            if(inode.addr[addrIndex] == -1){
                inode.addr[addrIndex] = get_free_block();
            }
            int singleOffset = blockCount % INDIRECT_SIZE;

            lseek(fd,(inode.addr[addrIndex] * BLOCK_SIZE) + (singleOffset * sizeof(int)), SEEK_SET);
            write(fd, &freeBlock, sizeof(int));
        }
    }
    else if ((inode.flags & (1 << 12)) == (1 << 12) && (inode.flags & (1 << 11)) == 0) //if long (10)
    {
        for(blockCount = 0; blockCount < blockCountMax; ++blockCount)
        {
            int freeBlock = get_free_block(); //get a free block

            read(externalFileFD, &blockBuf, BLOCK_SIZE); //read a block from external file
            lseek(fd, freeBlock * BLOCK_SIZE, SEEK_SET);
            write(fd, &blockBuf, BLOCK_SIZE); //write a block internaly

            int addrIndex = (blockCount / INDIRECT_SIZE) / INDIRECT_SIZE;
            if(inode.addr[addrIndex] == -1){
                inode.addr[addrIndex] = get_free_block();
            }
            int doubleOffset = (blockCount / INDIRECT_SIZE) % INDIRECT_SIZE;
            int singleOffset = blockCount % INDIRECT_SIZE;

            //read the block # in -> check if't a valid block # -> possibly get a free block
            lseek(fd, (inode.addr[addrIndex] * BLOCK_SIZE) + (doubleOffset * sizeof(int)), SEEK_SET);
            int blockNum;
            read(fd, &blockNum, sizeof(int));

            if(blockNum < 2 + superBlock.isize){ //if invalid block number
                blockNum = get_free_block();
                lseek(fd, (inode.addr[addrIndex] * BLOCK_SIZE) + (doubleOffset * sizeof(int)), SEEK_SET);
                write(fd, &blockNum, sizeof(int));
            }

            lseek(fd, (blockNum * BLOCK_SIZE) + (singleOffset * sizeof(int)), SEEK_SET);
            write(fd, &freeBlock, sizeof(int));
        }
    }
    else //if super long (11)
    {
        for(blockCount = 0; blockCount < blockCountMax; ++blockCount)
        {
            int freeBlock = get_free_block(); //get a free block

            read(externalFileFD, &blockBuf, BLOCK_SIZE); //read a block from external file
            lseek(fd, freeBlock * BLOCK_SIZE, SEEK_SET);
            write(fd, &blockBuf, BLOCK_SIZE); //write a block internaly

            int addrIndex = ((blockCount / INDIRECT_SIZE) / INDIRECT_SIZE) / INDIRECT_SIZE;
            if(inode.addr[addrIndex] == -1){
                inode.addr[addrIndex] = get_free_block();
            }
            int tripleOffset = ((blockCount / INDIRECT_SIZE) / INDIRECT_SIZE) % INDIRECT_SIZE;
            int doubleOffset = (blockCount / INDIRECT_SIZE) % INDIRECT_SIZE;
            int singleOffset = blockCount % INDIRECT_SIZE;

            //read the block # in -> check if't a valid block # -> possibly get a free block
            lseek(fd, (inode.addr[addrIndex] * BLOCK_SIZE) + (tripleOffset * sizeof(int)), SEEK_SET);
            int blockNum1;
            read(fd, &blockNum1, sizeof(int));

            if(blockNum1 < 2 + superBlock.isize){ //if invalid block number
                blockNum1 = get_free_block();
                lseek(fd, (inode.addr[addrIndex] * BLOCK_SIZE) + (tripleOffset * sizeof(int)), SEEK_SET);
                write(fd, &blockNum1, sizeof(int));
            }

            //read the block # in -> check if't a valid block # -> possibly get a free block
            lseek(fd, (blockNum1 * BLOCK_SIZE) + (doubleOffset * sizeof(int)), SEEK_SET);
            int blockNum2;
            read(fd, &blockNum2, sizeof(int));

            if(blockNum2 < 2 + superBlock.isize){ //if invalid block number
                blockNum2 = get_free_block();
                lseek(fd, (blockNum1 * BLOCK_SIZE) + (doubleOffset * sizeof(int)), SEEK_SET);
                write(fd, &blockNum2, sizeof(int));
            }

            lseek(fd, (blockNum2 * BLOCK_SIZE) + (singleOffset * sizeof(int)), SEEK_SET);
            write(fd, &freeBlock, sizeof(int));
        }
    }
    printf("\tCopying contents done.\n\n");


    // add a new entry to the root directory (expand root if needed)
    printf("\tAttempting to add the file into the root directory...\n");
    int dirBlock = getLastFileBlock(1);

    dir_type dir;
    int dirCount;
    for(dirCount = 0; dirCount < 32; ++dirCount){ //32 directory entries in a block
        //read in the directory entry
        lseek(fd, (dirBlock * BLOCK_SIZE) + (dirCount * sizeof(dir_type)), SEEK_SET);
        read(fd, &dir, sizeof(dir_type));

        if(dir.inode < 1){//reached the last writen directory entry
            break;
        }
    }
    if(dir.inode > 1){//reached the last directory entry in the block but they all ar used
            printf("ERROR: Can't add a file to the root directory. Root directory expansion not implemented \n");
            printf("\n########## Ending cpin execution ##########\n\n");
            return EXIT_FAILURE;
        }

    dir.inode = inodeNum;
    strcpy(dir.filename, v6File);

    lseek(fd, (dirBlock * BLOCK_SIZE) + (dirCount * sizeof(dir_type)), SEEK_SET);
    write(fd, &dir, sizeof(dir));
    printf("\tAdded the file into the root directory.\n");

    printf("\n########## Ending cpin execution ##########\n\n");
    return EXIT_SUCCESS;
}

void rm(char * fileName) {

}

/**
 * @brief Opens the passed external file as a current v6 file system.  
 * 
 * @param system 
 * @return 1 on success, -1 on failure.
 */
int openFileSystem(char* system)
{
    if ((fd = open(system, O_RDWR)) == -1) {
        printf("ERROR: Error opening file.\n");
        return -1;
    } 
    return 1;
}


//quit program
void quit(){
    close(fd);
    printf("Quitting Program \n");
    exit(0);
}

// The main function
int main()
{
    // inode_type inode1;
    // open_fs("Test_fs.txt");
    // fill_root_and_write();
    // inode1 = inode_reader(1, inode1);
    // printf("Value of inode1's addr[0] is %d\n", inode1.addr[0]);
    // printf("Value of inode1's addr[1] is %d\n", inode1.addr[1]);
    int setup = 0;
    while(1){
        printf("Enter one of the five commands\n");
        printf("initfs file_name fsize isize\n");
        printf("open file_system\n");
        printf("cpin external_file_name internal_file_name\n");
        printf("cpout internal_file_name external_file_name\n");
        printf("rm internal_file_name\n");
        printf("q\n");
        printf("> ");
        char command[128];
        scanf(" %[^\n]s",command);       
        char* token = strtok(command," ");
        if(strcmp(token,"initfs") == 0){
            char* file_name = strtok(NULL," ");
            char* n1 = strtok(NULL," ");
            char* n2 = strtok(NULL," ");
            if (initfs(file_name,atoi(n1),atoi(n2)) == 1) {
                setup = 1;
            }
        } else if(strcmp(token,"open") == 0){
            char* system = strtok(NULL," ");
            if (openFileSystem(system) == 1) {
                setup = 1;
            }
        }else if (strcmp(token,"cpin") == 0) {
            if (setup != 1) {
                printf("File System is not yet initialized. Use initfs or open command first.\n");
                return 0;
            }
            char * sourceFile = strtok(NULL," ");
            char * destinationFile = strtok(NULL," ");;
            if (!sourceFile || !destinationFile) {
                printf("Enter a source and destination path. \n");
            } else {
                cpin(sourceFile, destinationFile);
            }
        } else if (strcmp(token,"cpout") == 0) {
            if (setup != 1) {
                printf("File System is not yet initialized. Use initfs or open command first.\n");
                return 0;
            }
            char * sourceFile = strtok(NULL," ");
            char * destinationFile = strtok(NULL," ");;
            if (!sourceFile || !destinationFile) {
                printf("Enter a source and destination path. \n");
            } else {
                cpout(sourceFile, destinationFile);
            }
        } else if (strcmp(token,"rm") == 0) {
            if (setup != 1) {
                printf("File System is not yet initialized. Use initfs or open command first.\n");
                return 0;
            }
            char * fileName = strtok(NULL," ");
            if (!fileName) {
                printf("Enter a filename\n");
            } else {
                rm(fileName);
            }
        } else if(strcmp(token,"q") == 0){
            quit();
        }else{
            printf("No command found, Try again\n");
        }
    }
}