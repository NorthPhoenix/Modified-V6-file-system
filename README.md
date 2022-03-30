<div align="center">
  <h1>
    University of Texas at Dallas--Computer Science Department<br>
    CS 4348.005 Operating Systems Concepts Spring 2022<br>
    Project 2
  </h1>
</div>

## Description
The purpose of this project is to recreate Unix V6 file system.
This project is done in C/C++ only.

This is a group project and 3-member groups have been formed on elearning.

V6 file system is highly restrictive. A modification has been done: Block size is 1024 Bytes, `i-node` size is
64 Bytes and `i-node`’s structure and directory entry struc have been modified as well and given below:
```C
typedef struct {
int isize;
int fsize;
int nfree;
unsigned int free[200];
char flock;
char ilock;
char fmod;
unsigned int time;
} superblock_type; // Block size is 1024 Bytes; not all bytes of superblock are used.

superblock_type superBlock;

// i-node Structure

typedef struct {
unsigned short flags;
unsigned short nlinks;
unsigned int uid;
unsigned int gid;
unsigned int size0;
unsigned int size1;
unsigned int addr[9];
unsigned int actime;
unsigned int modtime;
} inode_type; //64 Bytes in size


typedef struct {
unsigned int inode;
char filename[28];
} dir_type;//32 Bytes long
```
Flags field has a small change: bits a, b, c are as before. Bits d and e are to represent if the file is small/medium/long/super long file (00 = small file, 01=medium, 10=long and 11 = super long file). Bit f is for set uid on execution and bit g is for set gid on execution. Other bits remain the same.

If file is small `addr[9]` has 9 direct block addresses. If file is medium, `addr[9]` has addresses of 9 single indirect blocks. If file is large, each element of `addr[]` is address of a double indirect block. If file is super long, each element of `addr[]` is address of a triple indirect block.

## User Commands

1. `initfs file_name n1 n2`
Here, `file_name` is the name of the file in the native Unix machine (where you are running your program) that represents the disk drive,
where `n1` is the file system size in number of blocks and `n2` is the number of blocks devoted to the i-nodes.

2. `q`
Quit the program


## Group members
- [Nikita Istomin](https://github.com/NorthPhoenix)
- [Zubair Shaik](https://github.com/ZubairShaik7)
- [Haniya Zafar]()

