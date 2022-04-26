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

#define BLOCK_SIZE 1024
#define INODE_SIZE 64

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
int findFileInSmallDir(char* filename, int block)
{
    dir_type dir;
    int dirCount;
    for(dirCount = 0; dirCount < 32; ++dirCount){ //32 directory entries in a block
        //read in the directory entry
        lseek(fd,(block * BLOCK_SIZE) + (dirCount * sizeof(dir_type)),SEEK_SET);
        read(fd,dir,sizeof(dir_type));

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
 * @brief atempts to reads all 256 integer block adresses from the block into the int array buf. Returns the true amount of integers read.
 * 
 * @param block 
 * @param buf 
 * @return int 
 */
int readIndirectBlock(int block, int* buf)
{
    return 0; //TODO
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
    //read in first inode(it points to the root directory)
    lseek(fd, 2*BLOCK_SIZE, SEEK_SET);
    read(fd,&root,sizeof(inode_type));

    //check how big is the root (small/medium/long/super_long)
    if((root.flags & (1 << 12)) == 0 && (root.flags & (1 << 11)) == 0) //if small (00)
    {
        int i;
        for(i = 0; i < 9; ++i){ //loop through addr[]
            if(root.addr[i] <= 0){//block number is invalid
                break;
            }
            int fileInode = findFileInSmallDir(filename, root.addr[i]); //look for filename in addr[i]
            if(fileInode != -1){
                return fileInode;
            }
        }
    }
    else if ((root.flags & (1 << 12)) == 0 && (root.flags & (1 << 11)) == (1 << 11)) //if medium (01)
    {
        int* singleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int i;
        for(i = 0; i < 9; ++i){ //loop through addr[]
            if(root.addr[i] <= 0){//block number is invalid
                break;
            }
            int dataBlocks = readIndirectBlock(root.addr[i], &singleIndirectBuf);
            int j;
            for(j = 0; j < dataBlocks; ++j){//loop over the single indirect block
                int fileInode = findFileInSmallDir(filename, singleIndirectBuf[j]); //look for filename in block at j
                if(fileInode != -1){
                    return fileInode;
                }
            }
        }
    }
    else if ((root.flags & (1 << 12)) == (1 << 12) && (root.flags & (1 << 11)) == 0) //if long (10)
    {
        int* singleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int* doubleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int i;
        for(i = 0; i < 9; ++i){ //loop through addr[]
            if(root.addr[i] <= 0){//block number is invalid
                break;
            }
            int singleIndirectBlocks = readIndirectBlock(root.addr[i], &doubleIndirectBuf);
            int j;
            for(j = 0; j < singleIndirectBlocks; ++j){//loop over the double indirect block
                int dataBlocks = readIndirectBlock(doubleIndirectBuf[j], &singleIndirectBuf);
                int k;
                for(k = 0; k < dataBlocks; ++k){//loop over the single indirect block
                    int fileInode = findFileInSmallDir(filename, singleIndirectBuf[k]); //look for filename in block at j
                    if(fileInode != -1){
                        return fileInode;
                    }
                }
            }
        }
    }
    else //if super long (11)
    {
        int* singleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int* doubleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int* tripleIndirectBuf[256]; // 256 block addresses(int) per an inderect block 
        int i;
        for(i = 0; i < 9; ++i){ //loop through addr[]
            if(root.addr[i] <= 0){//block number is invalid
                break;
            }
            int doubleIndirectBlocks = readIndirectBlock(root.addr[i], &tripleIndirectBuf);
            int j;
            for(j = 0; j < doubleIndirectBlocks; ++j){//loop over the double indirect block
                int singleIndirectBlocks = readIndirectBlock(tripleIndirectBuf[j], &doubleIndirectBuf);
                int k;
                for(k = 0; k < singleIndirectBlocks; ++k){//loop over the double indirect block
                    int dataBlocks = readIndirectBlock(doubleIndirectBuf[k], &singleIndirectBuf);
                    int l;
                    for(l = 0; l < dataBlocks; ++l){//loop over the single indirect block
                        int fileInode = findFileInSmallDir(filename, singleIndirectBuf[l]); //look for filename in block at j
                        if(fileInode != -1){
                            return fileInode;
                        }
                    }
                }
            }
        }
    }

    return -1;
}


int cpin(char* externalFile, char* v6File) {
    // open an external file
    int externalFileFD;
    if ((externalFileFD = open(externalFile, O_RDONLY)) == -1) {
        printf("Error opening file.\n");
        return 0;
    } 

    // look for the v6File in the root directory
    // if it exists -> overite it
    // else -> add a new entry to the root directory (expand root if needed)
    bool override = false;
    if(findFileInRoot(v6File)){
        override = true;
    }

    

    // create an inode
    unsigned int inodeNum = 2; //container for the inode number (=2 because that's the first potential free inode for a new file)

    // setup the inode

    // get free blocks as needed and copy the contents of the external file into the new v6 file

    
}

void rm(char * fileName) {

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
        printf("cpin external_file_name file_system_name\n");
        printf("cpout file_system_name external_file_name\n");
        printf("rm file_name\n");
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
        } else if (strcmp(token,"cpin") == 0) {
            if (setup != 1) {
                printf("File System is not yet initialized. Use initfs command first.\n");
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
                printf("File System is not yet initialized. Use initfs command first.\n");
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
                printf("File System is not yet initialized. Use initfs command first.\n");
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