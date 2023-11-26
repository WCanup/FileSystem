#include "filesystem.h"
#include "softwaredisk.h"
#include <string.h>
#include <stdio.h>

#define SHORT_MAX 65535

//char block[SOFTWARE_DISK_BLOCK_SIZE];
char buf[SOFTWARE_DISK_BLOCK_SIZE];
unsigned char data_bitmap[4096];
unsigned char inode_bitmap[512];

typedef struct Inode{
    unsigned short direct_addresses[13]; //26 bytes
    unsigned short indirect;             //2 bytes
    int size;                   //4 bytes
                                         //26+2+4 = 32 bytes
}Inode;

typedef struct Inode_Block{

    Inode IBlock[128]; //32 * 128 = 4096

}Inode_Block;

typedef struct Dir_Address{

    unsigned char name[507];  //507 bytes
    unsigned short id;        //2 bytes
    unsigned char mode;       //1 byte
    unsigned char NT[2];      //2 bytes
                              //507+2+1+2 = 512
}Dir_Address;

typedef struct Dir_Block{

    Dir_Address dblock[8]; //512 * 8 = 4096

}Dir_Block;

void initialize_bitmap()
{
    
    for(int i = 0; i < 4096; i++){
        data_bitmap[i] = 0;
    }
    for(int i = 0; i < 512; i++){
        inode_bitmap[i] = 0;
    }
    memcpy(buf, data_bitmap, sizeof(data_bitmap));
    write_sd_block(buf, 0);
    memcpy(buf, inode_bitmap, sizeof(inode_bitmap));
    memset(buf+sizeof(data_bitmap), '0', SOFTWARE_DISK_BLOCK_SIZE - sizeof(inode_bitmap));
    write_sd_block(inode_bitmap, 1);
    printf("Contents of block # 3:\n");
  for (int j=0; j < SOFTWARE_DISK_BLOCK_SIZE; j++) {
    printf("%c", buf[j]);
  }
    
}

void initialize_inode_blocks(){
    Inode_Block nodeBlock[4];
    int blockNum = 2;

    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 128; i++){
            for(int r = 0; r < 13; r++){
            nodeBlock[i].IBlock[j].direct_addresses[r] = SHORT_MAX;
            }
        
        nodeBlock[i].IBlock[j].indirect = SHORT_MAX;
        nodeBlock[i].IBlock[j].size = -1;
        }
            write_sd_block(nodeBlock[i].IBlock, blockNum);
            blockNum++;
    }

}

void initialize_dir_blocks(){
    Dir_Block dirBlocks[64];
    int blockNum = 6;
    for(int i = 0; i < 64; i++){
        for(int j = 0; j < 8; j++){
            for(int r = 0; r < 507; r++){
                dirBlocks[i].dblock[j].name[r] = ' ';//define null
            }
            dirBlocks[i].dblock[j].id = SHORT_MAX;
            dirBlocks[i].dblock[j].mode = ' ';
            dirBlocks[i].dblock[j].NT[0] = '\\';//prolly wrong
            dirBlocks[i].dblock[j].NT[1] = '0';
        }
        write_sd_block(dirBlocks[i].dblock, blockNum);
        blockNum++;
    }
}

int main(int argc, char *argv[]){

init_software_disk();
initialize_bitmap();
initialize_inode_blocks();
initialize_dir_blocks();
return 0;




// int num_reserved_blocks = NUM_BITMAP_BLOCKS + NUM_INODE_BLOCKS + NUM_DIR_BLOCKS;

// for (i = )

//return 1;
}