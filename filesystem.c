#include "filesystem.h"
#include "softwaredisk.h"
#include "softwaredisk.c"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

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