#include "filesystem.h"
#include "softwaredisk.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

//uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
uint8_t data_bitmap[SOFTWARE_DISK_BLOCK_SIZE] = {0};
uint8_t inode_dir_bitmap[SOFTWARE_DISK_BLOCK_SIZE] = {0};
Inode_Block inode_blocks[NUM_INODE_BLOCKS]= {0};
Dir_Block directory_blocks[NUM_DIR_BLOCKS] = {0};
DataBlock data_blocks[SOFTWARE_DISK_BLOCK_SIZE] = {0}; // Initialize all elements to 0
FSError fserror;


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

static void mark_inode_or_dir(int idx)
{
    inode_dir_bitmap[idx] = 1;

}

static void mark_block(int idx)  
{
    data_bitmap[idx] = 1;
}

static void free_inode_or_dir(int idx)
{
    inode_dir_bitmap[idx] = 0;
}

static void free_block(int idx)
{
    data_bitmap[idx] = 0;
}

static Inode* fetch_inode(uint16_t idx){
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
        index = idx - 384;
    }else{
        fserror = FS_OUT_OF_SPACE;
        //error error out of range
    }
    Inode *inode = &inode_blocks[whichBlock].IBlock[index];
    return inode;
}

static DataBlock* fetch_data_blocks(int num_blocks, uint16_t *indices){

    DataBlock *blocks = malloc(num_blocks * sizeof(DataBlock));

    for (int i = 0; i < num_blocks; ++i) {
        int idx = find_free_block();
        if(i >= SOFTWARE_DISK_BLOCK_SIZE)
        {
            fserror = FS_OUT_OF_SPACE;
            return blocks;
        }
        mark_block(idx);
        blocks[i] = data_blocks[idx];
        indices[i] = idx;
        // printf("value of index: %d", idx);
    }

    return blocks;
}

static Dir_Entry* fetch_dir_entry(int idx){

    int dir_block_index = idx / (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
    int dir_entry_index = idx % (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
    return &directory_blocks[dir_block_index].dblock[dir_entry_index];
}

static void write_inode(Inode *inode, uint16_t direct_addr[], uint16_t indirect_inode, uint32_t size)
{
    memcpy(inode->direct_addresses, direct_addr, sizeof(uint16_t[NUM_DIRECT_INODE_BLOCKS]));
    inode->indirect = indirect_inode;
    inode->size = size;
    
}

static void write_dir_entry(Dir_Entry *dir_entry, char *name, int id, uint8_t mode)
{
    strcpy(dir_entry->name, name);
    dir_entry->id = (uint16_t)id;
    dir_entry->mode = mode;
}

static unsigned long write_data_blocks(char buf[], uint16_t indicies[], int num_blocks, unsigned long numbytes)
{
    int current_bytes = numbytes;
    int byte_count = 0;
    for(int i = 0; i < num_blocks; i++)
    {
        
        char *buf_start = buf + i * SOFTWARE_DISK_BLOCK_SIZE;
        if(current_bytes >= SOFTWARE_DISK_BLOCK_SIZE){
        memcpy(data_blocks[indicies[i]].block, buf_start, SOFTWARE_DISK_BLOCK_SIZE);
        byte_count += SOFTWARE_DISK_BLOCK_SIZE;
        }
        else if(current_bytes < SOFTWARE_DISK_BLOCK_SIZE)
        {
            memcpy(data_blocks[indicies[i]].block, buf_start, current_bytes);
            byte_count+= current_bytes;
        }
        current_bytes -= SOFTWARE_DISK_BLOCK_SIZE;
    }
    return byte_count;
}

static void init_globals()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = read_sd_block(buf, DATA_BITMAP_BLOCK);
    memcpy(data_bitmap, buf, SOFTWARE_DISK_BLOCK_SIZE);

    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    ret = read_sd_block(buf, INODE_BITMAP_BLOCK);
    memcpy(inode_dir_bitmap, buf, SOFTWARE_DISK_BLOCK_SIZE);

    int z = 0;
    for(int i = FIRST_INODE_BLOCK; i <=LAST_INODE_BLOCK; i++)
    {
        bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
        ret = read_sd_block(buf, i);
        memcpy(inode_blocks[z].IBlock, buf, SOFTWARE_DISK_BLOCK_SIZE);
        z++;
    }

    int j= 0;
    for(int i = FIRST_DIR_ENTRY_BLOCK; i <= LAST_DIR_ENTRY_BLOCK; i++)
    { 
        bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
        ret = read_sd_block(buf, i);
        memcpy(directory_blocks[j].dblock, buf, SOFTWARE_DISK_BLOCK_SIZE);
        j++;
        // if(i == FIRST_DIR_ENTRY_BLOCK){
        //     for(int j = 0; j < SOFTWARE_DISK_BLOCK_SIZE; j++)
        //     printf("%x", buf[j]);
        //}
    }
}

static void write_dir_entry_to_disk(int idx)
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];

    int dir_block_index = idx / (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry));
    int sd_block_index = idx / (SOFTWARE_DISK_BLOCK_SIZE / sizeof(Dir_Entry)) + FIRST_DIR_ENTRY_BLOCK;
    

    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    memcpy(buf, directory_blocks[dir_block_index].dblock, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = write_sd_block(buf, sd_block_index);

    // for(int j = 0; j<SOFTWARE_DISK_BLOCK_SIZE; j++){
    //     printf("%c", directory_blocks[dir_block_index].dblock);
    // }
    
    
    
}

static void write_inode_dir_bitmap_to_disk()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    memcpy(buf, inode_dir_bitmap, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = write_sd_block(buf, INODE_BITMAP_BLOCK);
}

static void write_data_blocks_to_disk(int num_blocks, uint16_t indicies[], unsigned long numbytes)
{
    int ret;
    int current_bytes = numbytes;
    for(int i = 0; i < num_blocks; i++)
    {
        uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
        bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
        ret = read_sd_block(buf, indicies[i]);
        if(current_bytes >= SOFTWARE_DISK_BLOCK_SIZE){
            memcpy(buf, data_blocks[indicies[i]].block, SOFTWARE_DISK_BLOCK_SIZE);
        }
        else if(current_bytes < SOFTWARE_DISK_BLOCK_SIZE){
            memcpy(buf, data_blocks[indicies[i]].block, current_bytes);
        }

        write_sd_block(buf, indicies[i]);
        current_bytes = current_bytes - SOFTWARE_DISK_BLOCK_SIZE;
    }


}

//static void write_file();

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
    init_globals();
    int inode_dir_index = find_free_inode_or_dir();
    
    if(inode_dir_index == INT32_MAX)
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
    write_inode_dir_bitmap_to_disk();
    //fetch_inode(inode_dir_index);
    Dir_Entry *dir_entry = fetch_dir_entry(inode_dir_index);
    Inode *inode = fetch_inode(inode_dir_index);


    file->cursor_position = 0;
    file->inode = inode;
    file->mode = READ_WRITE;
    //uint16_t test[NUM_DIRECT_INODE_BLOCKS] = {3};


    write_dir_entry(dir_entry, name, inode_dir_index, 'b');

    write_dir_entry_to_disk(inode_dir_index);
    //write_inode(inode, test, NULL, 32);

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

    fserror = FS_NONE;
    Inode *inode = file->inode;
    // uint16_t test[NUM_DIRECT_INODE_BLOCKS] = {3};
    int num_blocks = (inode->size + numbytes + SOFTWARE_DISK_BLOCK_SIZE - 1) / SOFTWARE_DISK_BLOCK_SIZE;
    uint16_t indicies[num_blocks];
    DataBlock *blocks = fetch_data_blocks(num_blocks, indicies);
    //printf("value of index: %d", indicies[1]);
    write_inode(file->inode, indicies, 0, inode->size + numbytes);
    // printf("idcontents: %d\n", inode->size);
    unsigned long ret = write_data_blocks(buf, indicies, num_blocks, numbytes);
    write_data_blocks_to_disk(num_blocks, indicies, numbytes);

    return ret;

    //currently doesn't handle indirect inode allocation

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
    //printf("Expecting sizeof(Indirect_Inode_Block) = 4096, actual = %lu\n", sizeof(Indirect_Inode_Block));
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

// int main(int argc, char *argv[]){
//     init_software_disk();
//     check_structure_alignment();
//     File file1 = create_file("hello");
//     // File file2 = create_file("hello");
//     // File file3 = create_file("howdy partner");
//     // File file4 = create_file("howdy partner");
//     char buf[SOFTWARE_DISK_BLOCK_SIZE];
//     for(int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE-1000; i++){
//     memset(buf, "A"+i, SOFTWARE_DISK_BLOCK_SIZE-1000);
//     }
//     write_file(file1, buf, sizeof(buf));

//     read_sd_block(buf, 70);
//     for(int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE-1000; i++)
//     {
//         printf("%c", buf[i]);
//     }

//     return 0;
// }