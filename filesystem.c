#include "filesystem.h"
#include "softwaredisk.h"
#include "softwaredisk.c"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define NUM_BITMAP_BLOCKS 2
#define NUM_INODE_BLOCKS 4
#define NUM_DIR_BLOCKS 64

#define MAX_FILES 512
#define DATA_BITMAP_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define FIRST_INODE_BLOCK 2
#define LAST_INODE_BLOCK 5

#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 6
#define LAST_DIR_ENTRY_BLOCK 69
#define DIR_ENTRIES_PER_BLOCK 8

#define FIRST_DATA_BLOCK 70
#define LAST_DATA_BLOCK 4095
#define MAX_FILENAME_SIZE 507
#define NUM_DIRECT_INODE_BLOCKS 13
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t))

#define MAX_FILE_SIZE (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE

#define STARTING_SPACE 4026

uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];

// typedef struct DataBlock{
//     char data[1];
// }DataBlock;

// typedef struct Data_Blocks{
//     char block[STARTING_SPACE];
// }Data_Blocks;

// Data_Blocks filesystem;
uint8_t filesystem[SOFTWARE_DISK_BLOCK_SIZE] = {0};
FSError fserror;

typedef struct Inode{
    uint16_t direct_addresses[NUM_DIRECT_INODE_BLOCKS]; //26 bytes
    uint16_t indirect;             //2 bytes
    uint32_t size;                 //4 bytes
                                         //26+2+4 = 32 bytes
}Inode;

typedef struct Inode_Block{

    Inode IBlock[INODES_PER_BLOCK]; //32 * 128 = 4096

}Inode_Block;

Inode_Block inode_blocks[NUM_INODE_BLOCKS];

typedef struct Dir_Entry{

    uint8_t name[MAX_FILENAME_SIZE];  //507 bytes
    uint16_t id;        //2 bytes
    uint8_t mode;       //1 byte
    uint16_t NT;      //2 bytes
                        //507+2+1+2 = 512
}Dir_Entry;

typedef struct Dir_Block{

    Dir_Entry dblock[DIR_ENTRIES_PER_BLOCK]; //512 * 8 = 4096

}Dir_Block;

Dir_Block directory_blocks[NUM_DIR_BLOCKS];

typedef struct FileInternals{
    char *fname[MAX_FILENAME_SIZE];
    uint32_t size;
    FileMode mode;
    unsigned int cursor_position;
    Inode inode;
}FileInternals;

static uint16_t find_free_inode(void)
{
    for(int i = 0; i < MAX_FILES; i++)
    {
        if(inode_bitmap[i] == 0)
        {
            return i;
        }
    }
    return INT16_MAX;
}

static uint32_t find_free_block(void)
{
    for(uint32_t i = FIRST_DATA_BLOCK; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        if (data_bitmap[i] = 0)
        {
            return i;
        }
    }
    return INT32_MAX;
}

static int mark_inode(uint16_t idx)
{
    inode_bitmap[idx] = 1;
    return 1;
}

static int mark_block(uint16_t idx)  
{
    data_bitmap[idx] = 1;
    return 1; 
}

static void free_inode(uint16_t idx)
{
    inode_bitmap[idx] = 0;
}

static void free_block(uint16_t idx)
{
    data_bitmap[idx] = 0;
}

static Inode fetch_Inode(uint16_t idx){
    int index = idx;
    int whichBlock;
    //0-127 is 128 indexes
    if( index >= 0 && index <= 127 ){
        whichBlock = 0;
    //128-255
    }else if(index >= 128 && index <= 255){
        whichBlock = 1;
        index = idx - 128;
    }else if(index >= 256 && index <= 383){
        whichBlock = 2;
        index = idx - 256;
    }else if(index >= 384 && index <= 511){
        whichBlock = 3;
        index = idx - 386;
    }else{
        fserror = FS_OUT_OF_SPACE;
        //error error out of range
    }
    Inode inode = inode_blocks[whichBlock].IBlock[index];
    return inode;
}

static void write_inode(uint16_t idx)
{
    
    Inode inode = fetch_Inode(idx);

}

// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode){

}

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name){
    fserror = FS_NONE;
    File file;
    int index = find_free_inode;
    if(index == INT32_MAX)
    {
        fserror = FS_OUT_OF_SPACE;
        fs_print_error();
        return file;
    }
    mark_inode(index);
    file->cursor_position = 0;
    file->inode = fetch_Inode(index);
    //file->fname = *name;
    strcpy(file->fname, name);

    //file->fname = name;
    
    //find unused Inode
    //find unused Data Block
    //reserve Inode and Data Block

}

// close 'file'.  Always sets 'fserror' global.
void close_file(File file){
    fserror = FS_NONE;
    
}

// read at most 'numbytes' of data from 'file' into 'buf', starting at the 
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes){

}

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes){
    //compare the numbytes + size of our file
    //write opertation exceeds a data blocks capacity
    //
}

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos){

}

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file){

}

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name){

}

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name){

}

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void){

}

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void){

}

// filesystem error code set (set by each filesystem function)
extern FSError fserror;