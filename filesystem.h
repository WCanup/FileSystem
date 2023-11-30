//
// Simple filesystem API for LSU 4103 filesystem assignment.
// (@nolaforensix), 11/2017.  Minor updates 11/2019.  Updated 6/2022.
// Modified by AAG, 11/2022

#include <stdint.h>
#include "softwaredisk.h"

#if ! defined(__FILESYSTEM_4103_H__)
#define __FILESYSTEM_4103_H__

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



// file type used by user code


// access mode for open_file() 
typedef enum {
	READ_ONLY, READ_WRITE
} FileMode;

typedef enum {
	OPEN, CLOSED
}AccessMode;



//uint8_t dir_entry_bitmap[SOFTWARE_DISK_BLOCK_SIZE] = {0};

// typedef struct DataBlock{
//     char data[1];
// }DataBlock;

// typedef struct Data_Blocks{
//     char block[STARTING_SPACE];
// }Data_Blocks;

// Data_Blocks filesystem;
//File filesystem[SOFTWARE_DISK_BLOCK_SIZE] = {0};


// typedef struct Indirect_Inode{
//   uint16_t data_block_idx;
// }Indirect_Inode;

// typedef struct Indirect_Inode_Block{
//   uint8_t i_iblock[SOFTWARE_DISK_BLOCK_SIZE];
// }Indirect_Inode_Block;

typedef struct Inode{
    uint16_t direct_addresses[NUM_DIRECT_INODE_BLOCKS]; //26 bytes
    uint16_t indirect;       //2 bytes
    uint32_t size;                 //4 bytes
                                   //26+2+4 = 32 bytes
}Inode;

typedef struct Inode_Block{

    Inode IBlock[INODES_PER_BLOCK]; //32 * 128 = 4096

}Inode_Block;

typedef struct Dir_Entry{

    char name[MAX_FILENAME_SIZE];  //507 bytes
    uint16_t id;        //2 bytes
    uint8_t mode;       //1 byte
    //uint16_t NT;      //2 bytes
                        //507+2+1+2 = 512
}Dir_Entry;

typedef struct Dir_Block{

  Dir_Entry dblock[DIR_ENTRIES_PER_BLOCK]; //512 * 8 = 4096

}Dir_Block;

typedef struct DataBlock{
    uint8_t block[SOFTWARE_DISK_BLOCK_SIZE];
}DataBlock;


typedef struct FileInternals{
    //char fname[MAX_FILENAME_SIZE];
    //uint32_t size;
    AccessMode mode;
    uint32_t cursor_position;
    Inode *inode;
}FileInternals;

typedef struct FileInternals* File;
// error codes set in global 'fserror' by filesystem functions
typedef enum  {
  FS_NONE, 
  FS_OUT_OF_SPACE,         // the operation caused the software disk to fill up
  FS_FILE_NOT_OPEN,  	   // attempted read/write/close/etc. on file that isn't open
  FS_FILE_OPEN,      	   // file is already open. Concurrent opens are not
                           // supported and neither is deleting a file that is open.
  FS_FILE_NOT_FOUND, 	   // attempted open or delete of file that doesn’t exist
  FS_FILE_READ_ONLY, 	   // attempted write to file opened for READ_ONLY
  FS_FILE_ALREADY_EXISTS,  // attempted creation of file with existing name
  FS_EXCEEDS_MAX_FILE_SIZE,// seek or write would exceed max file size
  FS_ILLEGAL_FILENAME,     // filename begins with a null character
  FS_IO_ERROR              // something really bad happened
} FSError;



// function prototypes for filesystem API

// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode);

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name);

// close 'file'.  Always sets 'fserror' global.
void close_file(File file);

// read at most 'numbytes' of data from 'file' into 'buf', starting at the 
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes);

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes);

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos);

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file);

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name); 

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name);

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void);

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void);

// filesystem error code set (set by each filesystem function)
extern FSError fserror;


#endif
