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
int zeroArr[256];

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
void fill_an_inode_and_write(int inum)
{
    inode_type root;
    int i;

    root.flags |= 1 << 15; // Root is allocated
    root.flags |= 1 << 14; // It is a directory
    root.actime = time(NULL);
    root.modtime = time(NULL);

    root.size0 = 0;
    root.size1 = 2 * sizeof(dir_type);
    root.addr[0] = 100; // assuming that blocks 2 to 99 are for i-nodes; 100 is the first data block that can hold root's directory contents
    for (i = 1; i < 9; i++)
        root.addr[i] = -1; // all other addr elements are null so setto -1
    inode_writer(inum, root);
}

//takes the address of a block to be added to the free list and returns 1 if all went well and -1 if something went wrong.
int add_free_block(int address) {
    lseek(fd,BLOCK_SIZE * (address),SEEK_SET);
    write(fd,&zeroArr,sizeof(zeroArr));
    if (superBlock.nfree < 200) {
        superBlock.free[superBlock.nfree] = address;
        superBlock.nfree++;
        return 1;
    } else {
        return -1;
    }
}

//returns the address of a free data block (from the free list) if everything is good and returns -1 should there be an error somewhere (say no free blocks left)
int get_free_block() {
    superBlock.nfree--;
    if (superBlock.free[superBlock.nfree] == 0) {
        return -1;
    } else {
        return superBlock.free[superBlock.nfree];
    }
}

void initfs(int fileBlocks,int inodeBlocks) {

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
    inode_type inode1;
    open_fs("Test_fs.txt");
    fill_an_inode_and_write(1);
    inode1 = inode_reader(1, inode1);
    printf("Value of inode1's addr[0] is %d\n", inode1.addr[0]);
    printf("Value of inode1's addr[1] is %d\n", inode1.addr[1]);
    while(1){
        printf("Enter one of the two commands\n");
        printf("initfs file_name n1 n2\n");
        printf("q\n");
        char command[128];
        scanf(" %[^\n]s",command);       
        char * token = strtok(command," ");
        if(strcmp(token,"initfs") == 0){
            char * n1 = strtok(NULL," ");
            char * n2 = strtok(NULL," ");
            //initfs(atoi(n1),atoi(n2));
        }else if(strcmp(token,"q") == 0){
            quit();
        }else{
            printf("No command found, Try again\n");
        }
    }
}