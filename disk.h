#ifndef DISK_H
#define DISK_H

#include<stdio.h>

#define BOOT_BLOCK_SIZE 512
#define SUPERBLOCK_SIZE 512
#define N_DBLOCKS 10
#define N_IBLOCKS 4

struct superblock {
    int blocksize;
    int inode_offset;
    int data_offset;
    int swap_offset;
    int free_inode;
    int free_block;
};

struct inode {
    int next_inode;
    int protect;
    int nlink;
    int size;
    int uid;
    int gid;
    int ctime;
    int mtime;
    int atime;
    int dblocks[N_DBLOCKS];
    int iblocks[N_IBLOCKS];
    int i2block;
    int i3block;
};

void relocate_indirect_blocks(FILE *input_fp, FILE *output_fp, int *block_pointer, struct superblock *sb, int *next_free_block, int depth);
void defragment(FILE *input_fp, FILE *output_fp);

#endif