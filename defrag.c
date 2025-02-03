// defrag.c
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void relocate_indirect_blocks(FILE *input_fp, FILE *output_fp, int *block_pointer, struct superblock *sb, int *next_free_block, int depth) {
    if(depth<0 || *block_pointer == -1)
    {
        return;
    }

    if(depth == 0)
    {
        unsigned char *data_block = malloc(sb->blocksize);
        if (!data_block) {
            perror("Failed to allocate memory for indirect block");
            exit(EXIT_FAILURE);
        }

        fseek(input_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize) + ((*block_pointer) * sb->blocksize), SEEK_SET);
        fread(data_block, sb->blocksize, 1, input_fp);

        // Write the relocated indirect block
        fseek(output_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize) + ((*next_free_block) * sb->blocksize), SEEK_SET);
        fwrite(data_block, sb->blocksize, 1, output_fp);

        (*next_free_block)++;
        return;
    }

    if(depth>0)
    {
        // structure to store indirect block
        int parent_block = *next_free_block;
        (*next_free_block)++;
        unsigned char *indirect_block = malloc(sb->blocksize);
        if (!indirect_block) {
            perror("Failed to allocate memory for indirect block");
            exit(EXIT_FAILURE);
        }

        //reading the indirect block
        fseek(input_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize) + ((*block_pointer) * sb->blocksize), SEEK_SET);
        fread(indirect_block, sb->blocksize, 1, input_fp);

        //parse the pointers in the indirect block
        int *pointers = (int *)indirect_block;
        int num_pointers = (int)(sb->blocksize / sizeof(int));
        for (int i = 0; i < num_pointers; i++) {
            if (pointers[i] != -1) {
                int temp_block = *next_free_block;
                relocate_indirect_blocks(input_fp, output_fp, &pointers[i], sb, next_free_block, depth-1);
                pointers[i] = temp_block;
            }
        }

        fseek(output_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize) + (parent_block * sb->blocksize), SEEK_SET);
        fwrite(indirect_block, sb->blocksize, 1, output_fp);
        free(indirect_block);
    }
}

void defragment(FILE *input_fp, FILE *output_fp) {

    fseek(input_fp, 0, SEEK_END);
    long size = ftell(input_fp);
    fseek(input_fp, 0, SEEK_SET);

    //reading the disk
    unsigned char *old_buffer, *new_buffer;
    old_buffer = malloc(size);
    new_buffer = malloc(size);
    size_t bytes =  fread(old_buffer,size,1,input_fp);
    if(bytes != 1)
    {
        perror("Failed to read input file");
        free(old_buffer);
        free(new_buffer);
        return;
    }
    // printf("File size is: %ld\n", size);

    // Copy the boot block
    unsigned char boot_block[BOOT_BLOCK_SIZE];
    fseek(input_fp, 0, SEEK_SET);
    fread(boot_block, BOOT_BLOCK_SIZE, 1, input_fp);
    fseek(output_fp, 0, SEEK_SET);
    fwrite(boot_block, BOOT_BLOCK_SIZE, 1, output_fp);

    //reading superblock
    struct superblock *sb;
    sb = (struct superblock*) &(old_buffer[512]);
    // printf("Blocksize is: %d \n", sb->blocksize);
    // printf("Inode offset is: %d \n", sb->inode_offset);
    // printf("Data offset is: %d \n", sb->data_offset);
    // printf("Swap offset is: %d \n", sb->swap_offset);
    // printf("Free inodes is: %d \n", sb->free_inode);
    // printf("Free blocks is: %d \n\n\n", sb->free_block);
    printf("Defragmenting disk... It might take a while!\n");

    // Write the superblock
    unsigned char super_block[SUPERBLOCK_SIZE];
    fseek(input_fp, BOOT_BLOCK_SIZE, SEEK_SET);
    fread(super_block, SUPERBLOCK_SIZE, 1, input_fp);
    fseek(output_fp, BOOT_BLOCK_SIZE, SEEK_SET);
    fwrite(super_block, SUPERBLOCK_SIZE, 1, output_fp);

    // Read and defragment inodes
    int num_inodes = (sb->data_offset - sb->inode_offset) * (int)(sb->blocksize / sizeof(struct inode));
    struct inode *inodes = malloc(num_inodes * sizeof(struct inode));
    if (!inodes) {
        perror("Failed to allocate memory for inodes");
        exit(EXIT_FAILURE);
    }

    // Reading inodes into an array
    fseek(input_fp, 1024+(sb->blocksize*sb->inode_offset), SEEK_SET);
    fread(inodes, sizeof(struct inode), num_inodes, input_fp);
    // // used to check if file api working
    // fseek(output_fp, 1024+(sb->blocksize*sb->inode_offset), SEEK_SET);
    // fwrite(inodes, sizeof(struct inode), num_inodes, output_fp);

    // Buffer to store data blocks
    unsigned char *data_block = malloc(sb->blocksize);
    if (!data_block) {
        perror("Failed to allocate memory for data block");
        free(inodes);
        exit(EXIT_FAILURE);
    }

    // Start writing data blocks sequentially
    int next_free_block = 0;
    for (int i = 0; i < num_inodes; i++) {
        if (inodes[i].nlink == 0) {
            continue; // Skip unused inodes
        }

        // // inode debug print statements
        // for(int j=0; j<N_DBLOCKS;j++)
        // {
        //     if (inodes[i].dblocks[j] != -1) {
        //         printf("Data Block: %d\n", inodes[i].dblocks[j]);
        //         printf("Inode size: %d\n\n", inodes[i].size);
        //     }
        // }

        // Relocate direct blocks
        for (int j = 0; j < N_DBLOCKS; j++) {
            if (inodes[i].dblocks[j] != -1) {
                fseek(input_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize) + (inodes[i].dblocks[j] * sb->blocksize), SEEK_SET);
                fread(data_block, sb->blocksize, 1, input_fp);
                fseek(output_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->data_offset * sb->blocksize)+ (next_free_block * sb->blocksize), SEEK_SET);
                fwrite(data_block, sb->blocksize, 1, output_fp);
                inodes[i].dblocks[j] = next_free_block++;
            }
        }

        // Relocate indirect blocks
        for (int j = 0; j < N_IBLOCKS; j++) {
            if (inodes[i].iblocks[j] != -1) {
                int curr_free_block = next_free_block;
                relocate_indirect_blocks(input_fp, output_fp, &inodes[i].iblocks[j], sb, &next_free_block, 1);
                inodes[i].iblocks[j] = curr_free_block;
            }
        }

        // Relocate doubly indirect block
        if(inodes[i].i2block != -1)
        {
            int curr_free_block = next_free_block;
            relocate_indirect_blocks(input_fp, output_fp, &inodes[i].i2block, sb, &next_free_block, 2);
            inodes[i].i2block = curr_free_block;
        }
            
        // Relocate triply indirect block
        if(inodes[i].i3block != -1)
        {
            int curr_free_block = next_free_block;
            relocate_indirect_blocks(input_fp, output_fp, &inodes[i].i3block, sb, &next_free_block, 3);
            inodes[i].i3block = curr_free_block;
        }

    }

    // Write updated inodes back to the output file
    fseek(output_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->inode_offset * sb->blocksize), SEEK_SET);
    fwrite(inodes, sizeof(struct inode), num_inodes, output_fp);

    //copying the swap space to the output disk
    int swap_space = (size - (sb->blocksize * sb->swap_offset));
    unsigned char *ss_pointer = malloc(swap_space*sizeof(unsigned char));
    if (!ss_pointer) {
        perror("Failed to allocate memory for swap space");
        exit(EXIT_FAILURE);
    }
    fseek(input_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->swap_offset * sb->blocksize), SEEK_SET);
    fread(ss_pointer, swap_space, 1, input_fp);
    fseek(output_fp, (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + (sb->swap_offset * sb->blocksize), SEEK_SET);
    fwrite(ss_pointer, swap_space, 1, output_fp);

    // Rebuild free block list
    sb->free_block = next_free_block;
    // printf("Next free block is: %d", next_free_block);
    int x;
    for (x = next_free_block; x < (sb->swap_offset - sb->data_offset); x++) {
        long offset = (BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE) + ((sb->data_offset + x) * sb->blocksize);
        // Seek to the current free block location
        fseek(output_fp, offset, SEEK_SET);
        if (x + 1 < (sb->swap_offset - sb->data_offset)) {
            // Write the index of the next free block
            next_free_block++;
            fwrite(&next_free_block, sizeof(int), 1, output_fp);
        } else {
            int end_of_list = -1;
            fwrite(&end_of_list, sizeof(int), 1, output_fp);
        }
    }

    //writing superblock to the output disk
    fseek(output_fp, BOOT_BLOCK_SIZE, SEEK_SET);
    fwrite(sb, sizeof(struct superblock), 1, output_fp);

    free(inodes);
    free(data_block);
}
