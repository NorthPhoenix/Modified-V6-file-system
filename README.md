<div align="center">
  <h1>
    University of Texas at Dallas--Computer Science Department<br>
    CS 4348.005 Operating Systems Concepts Spring 2022<br>
    Project 2 Part 2
  </h1>
</div>

## Description

A modification to V6 file system has been done: Block size is 1024 Bytes, `i-node` size is
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

## Goals

You need to implement the following additional commands and add to the project 2-part 1:
**Keep in mind that all files in v6 file system that you need to deal with in this project part are in the root directory and there will be no subdirectories.**

1. **`cpin externalfile v6-file`**
   - Creat a new file called `v6-file` in the v6 file system and fill the contents of the newly created file with the contents of the `externalfile`.
2. **`cpout v6-file externalfile`**
   - If the v6-file exists, create `externalfile` and make the externalfile's contents equal to `v6-file`.
3. **`rm v6-file`**
   - Delete the file v6_file from the v6 file system. Remove all the data blocks of the file, free the i-node and remove the directory entry.
4. **`open file_system`**
   - Open a native Unix machine file `file_system` that has been formated like a modified V6 file system.
5. **`q`**
   - Save all changes and quit. 

## User Commands

1. **`initfs file_name n1 n2`**
   - Here, `file_name` is the name of the file in the native Unix machine (where you are running your program) that represents the disk drive, where `n1` is the file system size in number of blocks and `n2` is the number of blocks devoted to the i-nodes.
2. **`open file_system`**
   - Open a native Unix machine file `file_system` that represents the disk drive.
3. **`cpin externalfile v6-file`**
   - Copy a native Unix machine file `externalfile` into the file named `v6-file` in the root of the opened V6 file system.
4. **`cpout v6-file externalfile`**
   - Copy a file `v6-file` from the root directory of the oppened V6 file system into a native Unix machine file `externalfile`.
5. **`rm v6-file`**
   - Remove a file `v6-file` from the root directkory of the oppened V6 file system.
6. **`q`**
   - Save and quit the program


## Due Date
April 26, 2022 11:55pm

