#include "filesystem.h"
#include "filesystem.c"
#include "softwaredisk.h"
#include "softwaredisk.c"
#include <string.h>
#include <stdio.h>
#include <stdint.h>



int check_structure_alignment(void){
    printf("Expecting sizeof(Inode) = 32, actual = %lu\n", sizeof(Inode));
    //printf("Expecting sizeof(IndirectBlock) = 32, actual = %lu\n", sizeof(Inode));
    printf("Expecting sizeof(Inode_Block) = %d, actual = %lu\n", SOFTWARE_DISK_BLOCK_SIZE,sizeof(Inode_Block));
    printf("Expecting sizeof(Dir_Address) = 32, actual = %lu\n", sizeof(Dir_Address));
    printf("Expecting sizeof(Dir_Block) = %d, actual = %lu\n", SOFTWARE_DISK_BLOCK_SIZE, sizeof(Dir_Block));
    printf("Expecting sizeof(data_bitmap) = %d, actual = %lu\n", SOFTWARE_DISK_BLOCK_SIZE,sizeof(data_bitmap));
    
    if(sizeof(Inode) != 32|| 
    sizeof(Inode_Block) != SOFTWARE_DISK_BLOCK_SIZE || 
    sizeof(Dir_Address) != 512 || 
    sizeof(Dir_Block) != SOFTWARE_DISK_BLOCK_SIZE || 
    sizeof(data_bitmap) != SOFTWARE_DISK_BLOCK_SIZE){
    return 0;
    }
    return 1;
}

int main(int argc, char *argv[]){

init_software_disk();
if(!check_structure_alignment)
{
    return 0;
}

return 1;

}