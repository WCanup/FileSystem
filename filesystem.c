#include "filesystem.h"
#include "softwaredisk.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


static int find_free_inode_or_dir(void)
{
    for(int i = 0; i < MAX_FILES; i++)
    {
        if(inode_dir_bitmap[i] == 0)
        {
            return i;
        }
    }
    return INT16_MAX;
}

static int find_free_block(void)
{
    for(int i = FIRST_DATA_BLOCK; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        if (data_bitmap[i] == 0)
        {
            return i;
        }
    }
    return INT32_MAX;
}

// static int find_free_dir_entry(void)
// {
//     for(int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
//     {
//         if (inode_dir_bitmap[i] == 0)
//         {
//             return i;
//         }
//     }
//     return INT32_MAX;
// }

static void mark_inode_or_dir(int idx)
{
    inode_dir_bitmap[idx] = 1;
}

static void mark_block(int idx)  
{
    data_bitmap[idx] = 1;
}

// static void mark_dir_entry(int idx)
// {
//     dir_entry_bitmap[idx] = 1;
// }

static void free_inode_or_dir(int idx)
{
    inode_dir_bitmap[idx] = 0;
}

static void free_block(int idx)
{
    data_bitmap[idx] = 0;
}

// static void free_dir_entry(int idx)
// {
//     dir_entry_bitmap[idx] = 0;
// }

static Inode fetch_Inode(uint16_t idx){
    int index = idx;
    int whichBlock;
    //0-127 is 128 indexes
    if(index >= 0 && index <= 127 ){
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

static void fetch_data_block(int idx){

}

static Dir_Entry fetch_dir_entry(int idx){
    int first_dir_per_block = 0;
    Dir_Entry result;
    for(int i = 0; i < NUM_DIR_BLOCKS; i++)
    {
        for(int j = 0; j < DIR_ENTRIES_PER_BLOCK; j++){
            if(j + first_dir_per_block == idx)
            {
                result = directory_blocks[i].dblock[j];
                return result;
            }
        }
        first_dir_per_block = first_dir_per_block + DIR_ENTRIES_PER_BLOCK;
    }
    return result;
}

static void write_inode(uint16_t idx, uint16_t direct_addr[], uint16_t indirect_inode, uint32_t i_size)
{
    Inode inode = fetch_Inode(idx);
    memcpy(inode.direct_addresses, direct_addr, sizeof(inode.direct_addresses));


}

static void write_dir_entry(Dir_Entry *dir_entry, char *name[], int id, char mode)
{
    strcpy(dir_entry->name, name);
    dir_entry->id = id;
    dir_entry->mode = mode;

}

static void write_file();

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
    file = malloc(sizeof(FileInternals));
    int inode_dir_index = find_free_inode_or_dir();
    int data_index = find_free_block();
    
    if(inode_dir_index == INT32_MAX)
    {
        fserror = FS_OUT_OF_SPACE;
        fs_print_error();
        return NULL;
    }
    if(data_index == INT32_MAX)
    {
        fserror = FS_OUT_OF_SPACE;
        fs_print_error();
        return NULL;
    }
    if(file_exists(name))
    {
        fserror = FS_FILE_ALREADY_EXISTS;
        fs_print_error();
        return NULL;
    }

    mark_inode_or_dir(inode_dir_index);
    fetch_Inode(inode_dir_index);
    mark_block(data_index);

    file->cursor_position = 0;
    file->inode = fetch_Inode(inode_dir_index);
    strcpy(file->fname, name);
    file->mode = READ_WRITE;
    file->size = 0;

    int dir_block_index = inode_dir_index / (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
    int dir_entry_block_index = inode_dir_index % (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));

    write_dir_entry(&directory_blocks[dir_block_index].dblock[dir_entry_block_index], name, inode_dir_index, 'b');

    printf("name contents: %s\n", file->fname);
    free(file);

    return file;

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

    for(int i = 0; i < NUM_DIR_BLOCKS * DIR_ENTRIES_PER_BLOCK; i++)
    {
        //printf("value of i: %d\n", i);
        if(inode_dir_bitmap[i] == 1)
        {
            int dir_block_index = i / (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
            int dir_entry_index = i % (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
            if(!strcmp(directory_blocks[dir_block_index].dblock[dir_entry_index].name, name))
            {
                return 1;
            }
        }
    }
    return 0;

    // int first_dir_per_block = 0;
    // for(int i = 0; i < NUM_DIR_BLOCKS; i++){
    //     for(int j = 0; j < DIR_ENTRIES_PER_BLOCK; i++){
    //         if(strcmp(!directory_blocks[i].dblock[j].name, name)){
    //             printf("new file name:%s\nold file name: %s\n", name, directory_blocks[i].dblock[j].name);
    //             fserror = FS_FILE_ALREADY_EXISTS;
    //             return 1;
    //         }
    //     }
    //     first_dir_per_block = first_dir_per_block + DIR_ENTRIES_PER_BLOCK;
    // }
    // return 0;

}

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void){
    switch (fserror) {
  case FS_EXCEEDS_MAX_FILE_SIZE:
    printf("FS: File is too big.\n");
    break;
  case FS_FILE_ALREADY_EXISTS:
    printf("FS: File name already exists.\n");
    break;
  case FS_FILE_NOT_FOUND:
    printf("FS: File not found.\n");
    break;
  case FS_FILE_NOT_OPEN:
    printf("FS: File is not open.\n");
    break;
  case FS_FILE_OPEN:
    printf("FS: File is already open.\n");
    break;
  case FS_FILE_READ_ONLY:
    printf("FS: File is in read only mode.\n");
    break;
  case FS_ILLEGAL_FILENAME:
    printf("FS: Illegal file name.\n");
    break;
  case FS_OUT_OF_SPACE:
    printf("FS: Out of space.\n");
    break;
  case FS_NONE:
    printf("FS: No error.\n");
    break;
  default:
    printf("FS: Unknown error code %d.\n", sderror);
  }

}

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void){

    printf("Expecting sizeof(Inode) = 32, actual = %llu\n", sizeof(Inode));
    printf("Expecting sizeof(Indirect_Inode_Block) = 4096, actual = %llu\n", sizeof(Indirect_Inode_Block));
    printf("Expecting sizeof(Inode_Block) = %d, actual = %llu\n", SOFTWARE_DISK_BLOCK_SIZE,sizeof(Inode_Block));
    printf("Expecting sizeof(Dir_Entry) = 512, actual = %llu\n", sizeof(Dir_Entry));
    printf("Expecting sizeof(Dir_Block) = %d, actual = %llu\n", SOFTWARE_DISK_BLOCK_SIZE, sizeof(Dir_Block));
    printf("Expecting sizeof(data_bitmap) = %d, actual = %llu\n", SOFTWARE_DISK_BLOCK_SIZE,sizeof(data_bitmap));
    
    if(sizeof(Inode) != 32|| 
    sizeof(Inode_Block) != SOFTWARE_DISK_BLOCK_SIZE || 
    sizeof(Dir_Entry) != 512 || 
    sizeof(Dir_Block) != SOFTWARE_DISK_BLOCK_SIZE || 
    sizeof(data_bitmap) != SOFTWARE_DISK_BLOCK_SIZE){
    return 0;
    }
    return 1;
}

// filesystem error code set (set by each filesystem function)
extern FSError fserror;

int main(int argc, char *argv[]){
    File file1 = create_file("hello");
    File file2 = create_file("hello");
    File file3 = create_file("howdy partner");
    File file4 = create_file("howdy partner");



    return 0;
}