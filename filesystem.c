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
    for(int i = FIRST_DATA_BLOCK; i < LAST_DATA_BLOCK; i++)
    {
        if (data_bitmap[i] == 0)
        {
            return i ;
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
        index = idx;
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
    
    //WILL CODE
    inode->indirect = 0;
    inode->size = 0;


    return inode;
}

static DataBlock* fetch_data_blocks(int num_blocks, uint16_t *indices){

    DataBlock *blocks = malloc(num_blocks * sizeof(DataBlock));

    for (int i = 0; i < num_blocks; i++) {

        int idx = find_free_block();//find_free_block() returns INT32_MAX if there is no space left
        
        if(idx == INT32_MAX)
        {
            fserror = FS_OUT_OF_SPACE;
            free(blocks);
            return NULL;
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

static unsigned long write_inode(Inode *inode, uint16_t addresses[], int num_blocks)
{   
    int num_New_Addresses;
    int num_Addresses_Already_In_Inode;
    int num_Addresses_To_Add_In_Direct;
    int num_Addresses_To_Add_In_Indirect;
    int num_Of_Indirect_Addresses_In_Inode;

    num_New_Addresses = num_blocks;

    num_Addresses_Already_In_Inode = inode->size;
    printf("-------------------------------------------------");
    printf("\n\nINODE SIZE : %u\n\n",inode->size);
    printf("-------------------------------------------------\n");

    //if inode already uses indirect block then set numToAdd to be 0
    
    num_Addresses_To_Add_In_Direct = 13 - num_Addresses_Already_In_Inode; if( num_Addresses_To_Add_In_Direct < 0){num_Addresses_To_Add_In_Direct = 0;}
    //if inode does not need indirect block then set numToAdd to be 0
    num_Addresses_To_Add_In_Indirect = num_New_Addresses - num_Addresses_To_Add_In_Direct; if(num_Addresses_To_Add_In_Indirect < 0){num_Addresses_To_Add_In_Indirect = 0;}
    //if you cannot add all addresses without breaking the bounds of the block then return zero and set fserror
    num_Of_Indirect_Addresses_In_Inode = num_Addresses_Already_In_Inode - 13;if(num_Of_Indirect_Addresses_In_Inode + num_New_Addresses > 2048){fserror = FS_EXCEEDS_MAX_FILE_SIZE;  return 0;}


    printf("Number of New Addresses : %d\nNumber of Addresses Already In : %d\n Number of Indirect Addresses Already In : %d\nNumber to Add in Direct : %d\nNumber to Add in Indirect : %d\n",
    num_New_Addresses,num_Addresses_Already_In_Inode,num_Of_Indirect_Addresses_In_Inode,num_Addresses_To_Add_In_Direct,num_Addresses_To_Add_In_Indirect);
    

    // if(num_Addresses_Already_In_Inode == 1 && num_New_Addresses <= 12)
    // {
    //     printf("\nCASE 1\n");
    //     //copy the addresses into the Inode with no extra logic
    //     memcpy(inode->direct_addresses + 2, addresses, sizeof(uint16_t[NUM_DIRECT_INODE_BLOCKS]));
    //     uint32_t number = num_New_Addresses;
    //     inode->size = number;
    //     return 1;
    
    // }else{ 

        if(num_Addresses_To_Add_In_Direct > num_blocks){
            num_Addresses_To_Add_In_Direct = num_blocks;
        }

        printf("NEW NUMTOADD: %d\n",num_Addresses_To_Add_In_Direct);
        
        if(num_Addresses_To_Add_In_Direct > 0){

            printf("\nCASE 2\n");
            //get sam to look over this pretty please
            memcpy(inode->direct_addresses+(num_Addresses_Already_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Direct*2));
            uint32_t number = num_Addresses_To_Add_In_Direct;
            inode->size = inode->size + number;
            printf("-------------------------------------------------");
    printf("\n\nINODE SIZE : %u\n\n",inode->size);
    printf("-------------------------------------------------\n");
            if(num_Addresses_To_Add_In_Indirect == 0){
                return 1;
            }
        }

        if(num_Addresses_To_Add_In_Indirect > 0){
        
            //give inode an index for its indirect block
            if(inode->indirect == 0){
                inode->indirect = find_free_block();
                mark_block(inode->indirect);
                
            }

            //Are the direct addresses filled up and therefore do we need to worry about spacing?
            if(num_Addresses_To_Add_In_Direct == 0){

                //Is the indirect block empty and therefore do we need to worry about spacing?
                if(num_Of_Indirect_Addresses_In_Inode == 0){
                    printf("\nCASE 3\n");
                    memcpy(data_blocks[inode->indirect].block, addresses, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    printf("-------------------------------------------------");
                    printf("\n\nINODE SIZE : %u\n\n",inode->size);
                    printf("-------------------------------------------------\n");
                    return 1;

                }else{
                    printf("\nCASE 4\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    printf("-------------------------------------------------");
                    printf("\n\nINODE SIZE : %u\n\n",inode->size);
                    printf("-------------------------------------------------\n");
                    return 1;
                }
            }else{

                if(num_Of_Indirect_Addresses_In_Inode == 0){
                    printf("\nCASE 5\n");
                    memcpy(data_blocks[inode->indirect].block, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    printf("-------------------------------------------------");
                    printf("\n\nINODE SIZE : %u\n\n",inode->size);
                    printf("-------------------------------------------------\n");
                    return 1;

                }else{
                    printf("\nCASE 6\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    printf("-------------------------------------------------");
                    printf("\n\nINODE SIZE : %u\n\n",inode->size);
                    printf("-------------------------------------------------\n");
                    return 1;
                }
            }
            
            
        }
    //}
    return 2;
}

static void write_dir_entry(Dir_Entry *dir_entry, char *name, int id, uint8_t mode)
{
    strcpy(dir_entry->name, name);
    dir_entry->id = (uint16_t)id;
    dir_entry->mode = mode;
}

static unsigned long write_data_blocks(char buf[], uint16_t indicies[], int num_blocks, unsigned long numbytes, int startingPoint)
{   
    uint16_t blockNum = indicies[0];
    printf("Block Num : %u\n",blockNum);
    printf("Starting Point : %u\n",startingPoint);
    
    //how many bytes we have left to write
    int current_bytes = numbytes;
    //how many we have written
    int byte_count = startingPoint;

    for(int i = 0; i < num_blocks; i++)
    {
        
        char *buf_start = buf + startingPoint + (i * SOFTWARE_DISK_BLOCK_SIZE);

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
    return byte_count - SOFTWARE_DISK_BLOCK_SIZE;
}

static void init_globals()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    //
    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = read_sd_block(buf, DATA_BITMAP_BLOCK);
    memcpy(data_bitmap, buf, SOFTWARE_DISK_BLOCK_SIZE);
    // for(int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    // {
    //     printf("%d", data_bitmap[i]);
    // }
    //
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

    int r = 0;
    for(int i = FIRST_DATA_BLOCK; i <=LAST_DATA_BLOCK; i++)
    {
        bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
        ret = read_sd_block(buf, i);
        memcpy(data_blocks[i].block, buf, SOFTWARE_DISK_BLOCK_SIZE);
        r++;
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

static void write_data_block_bitmap_to_disk()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    memcpy(buf, data_bitmap, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = write_sd_block(buf, DATA_BITMAP_BLOCK);
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
    
    // for(int i = 0; i < num_blocks; i++){

    //     write_sd_block(data_blocks[indicies[i]].block,indicies[i]);

    // }
    
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

static int write_inode_seek_file(Inode *inode, uint16_t addresses[], int num_blocks){//I want this version to return the value of the last block address to use in seek file

    int num_New_Addresses;
    int num_Addresses_Already_In_Inode;
    int num_Addresses_To_Add_In_Direct;
    int num_Addresses_To_Add_In_Indirect;
    int num_Of_Indirect_Addresses_In_Inode;
    int last_block;

    num_New_Addresses = num_blocks;

    num_Addresses_Already_In_Inode = inode->size;

    //if inode already uses indirect block then set numToAdd to be 0
    num_Addresses_To_Add_In_Direct = 13 - num_Addresses_Already_In_Inode; if( num_Addresses_To_Add_In_Direct < 0){num_Addresses_To_Add_In_Direct = 0;}
    //if inode does not need indirect block then set numToAdd to be 0
    num_Addresses_To_Add_In_Indirect = num_New_Addresses - num_Addresses_To_Add_In_Direct; if(num_Addresses_To_Add_In_Indirect < 0){num_Addresses_To_Add_In_Indirect = 0;}
    //if you cannot add all addresses without breaking the bounds of the block then return zero and set fserror
    num_Of_Indirect_Addresses_In_Inode = num_Addresses_Already_In_Inode - 13;if(num_Of_Indirect_Addresses_In_Inode + num_New_Addresses > 2048){fserror = FS_EXCEEDS_MAX_FILE_SIZE;  return 0;}


    printf("Number of New Addresses : %d\nNumber of Addresses Already In : %d\n Number of Indirect Addresses Already In : %d\nNumber to Add in Direct : %d\nNumber to Add in Indirect : %d\n",
    num_New_Addresses,num_Addresses_Already_In_Inode,num_Of_Indirect_Addresses_In_Inode,num_Addresses_To_Add_In_Direct,num_Addresses_To_Add_In_Indirect);

    int last_block;
    

    // if(num_Addresses_Already_In_Inode == 1 && num_New_Addresses <= 12)
    // {
    //     printf("\nCASE 1\n");
    //     //copy the addresses into the Inode with no extra logic
    //     memcpy(inode->direct_addresses + 2, addresses, sizeof(uint16_t[NUM_DIRECT_INODE_BLOCKS]));
    //     uint32_t number = num_New_Addresses;
    //     inode->size = number;
    //     return 1;
    
    // }else{ 

        if(num_Addresses_To_Add_In_Direct > num_blocks){
            num_Addresses_To_Add_In_Direct = num_blocks;
        }

        printf("NEW NUMTOADD: %d\n",num_Addresses_To_Add_In_Direct);
        
        if(num_Addresses_To_Add_In_Direct > 0){

            printf("\nCASE 2\n");
            //get sam to look over this pretty please
            memcpy(inode->direct_addresses+(num_Addresses_Already_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Direct*2));
            uint32_t number = num_Addresses_To_Add_In_Direct;
            inode->size = inode->size + number;
            if(num_Addresses_To_Add_In_Indirect == 0){
                last_block = addresses[num_Addresses_Already_In_Inode];//set value of last block to be last direct address
                return last_block;
            }
        }

        if(num_Addresses_To_Add_In_Indirect > 0){
        
            //give inode an index for its indirect block
            if(inode->indirect == 0){
                inode->indirect = find_free_block();
                mark_block(inode->indirect);
                
            }

            //Are the direct addresses filled up and therefore do we need to worry about spacing?
            if(num_Addresses_To_Add_In_Direct == 0){

                //Is the indirect block empty and therefore do we need to worry about spacing?
                if(num_Of_Indirect_Addresses_In_Inode == 0){
                    printf("\nCASE 3\n");
                    memcpy(data_blocks[inode->indirect].block, addresses, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    last_block = addresses[num_Addresses_To_Add_In_Indirect];//assign last block value
                    return last_block;

                }else{
                    printf("\nCASE 4\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    last_block = addresses[num_Addresses_To_Add_In_Indirect];
                    return last_block;
                }
            }else{

                if(num_Of_Indirect_Addresses_In_Inode == 0){
                    printf("\nCASE 5\n");
                    memcpy(data_blocks[inode->indirect].block, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    last_block = addresses[num_Addresses_To_Add_In_Indirect+num_Addresses_To_Add_In_Direct];
                    return last_block;

                }else{
                    printf("\nCASE 6\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    last_block = addresses[num_Addresses_To_Add_In_Indirect+num_Addresses_To_Add_In_Direct];
                    return last_block;
                }
            }
            
            
        }
    //}
    return 2;
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
    file->size = 0;
    //strcpy(file->name, name );
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

   
    //can't read more than numbytes
    //starts at data_blocks[file->inode->direct_addresses[i]].block[pos_in_block]
    //stops if end of file is reached
    // for(int i = starting_block_index; i <)
    // memcpy(buf, data_blocks[file->inode->direct_addresses[i]].block[pos_in_block], )

}

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes){
    //printf("CURRENT FILE NAME : %c",file->name);
    init_globals();
    printf("Start of write_file :\n\n");
    printf("file size = %u\n",file->size);
    printf("inode size = %u\n", file->inode->size);
    unsigned long ret = 0;
    int how_Many_Blocks = file->inode->size;
    int how_Much_Data = file->size;
    int how_Much_In_Last_Block = how_Much_Data % 4096;
    int amount_To_Add_In_Last_Block = 4096 - how_Much_In_Last_Block;
    file->cursor_position = seek_file(file, file->size);
    DataBlock last_Block;
    uint16_t addressOfLastBlock;
    

    if(file->inode->size == 0){
            
            int idx = find_free_block();
            printf("\n\nFREED BLOCK : %d\n\n",idx);
            mark_block(idx);
            uint16_t num = idx;
            file->inode->direct_addresses[0] = num;
            file->inode->size++;
            printf("inode size = %u\n", file->inode->size);
            how_Many_Blocks++;

        }


    if(how_Many_Blocks > 13){

        int amount_Of_Addresses_In_Indirect_Block = how_Many_Blocks - 13;
        printf("\nAmount Of Addresses In Indirect : %d\n",amount_Of_Addresses_In_Indirect_Block);
        last_Block = data_blocks[addressOfLastBlock];//THIS WILL NOT WORK BECAUSE ADDRESS ISN"T INITIALIZED YET HERE
        printf("LAST BLOCK INDEX : ", how_Many_Blocks-1);

    }else{


        last_Block = data_blocks[file->inode->direct_addresses[how_Many_Blocks-1]];
        printf("LAST BLOCK INDEX : %d", how_Many_Blocks-1);
        addressOfLastBlock = file->inode->direct_addresses[how_Many_Blocks-1];
        

    }
    printf("\nAmount To Add In Last Block : %d\n", amount_To_Add_In_Last_Block);
    if(numbytes > amount_To_Add_In_Last_Block){

        memcpy(last_Block.block + how_Much_In_Last_Block, buf, amount_To_Add_In_Last_Block);
        write_sd_block(last_Block.block, addressOfLastBlock);
        printf("A\n");
    }else{
        printf("HOW MUCH IN LAST BLOCK : %d\n", how_Much_In_Last_Block);
        printf("NUMBYTES : %lu\n",numbytes);
        memcpy(last_Block.block + how_Much_In_Last_Block, buf, numbytes);
        write_sd_block(last_Block.block, addressOfLastBlock);


        write_data_block_bitmap_to_disk();
        write_inode_dir_bitmap_to_disk();


        printf("address : %d\n", addressOfLastBlock);
        printf("B\n");
        file->size += numbytes;
        return numbytes;

    }

    
    fserror = FS_NONE;
    Inode *inode = file->inode;
    // uint16_t test[NUM_DIRECT_INODE_BLOCKS] = {3};
    //int num_blocks = (file->size + numbytes + SOFTWARE_DISK_BLOCK_SIZE - 1) / SOFTWARE_DISK_BLOCK_SIZE;
    int num_blocks = ((numbytes - amount_To_Add_In_Last_Block + SOFTWARE_DISK_BLOCK_SIZE - 1)/SOFTWARE_DISK_BLOCK_SIZE);
    printf("Num Blocks : %d\n",num_blocks);
    uint16_t indicies[num_blocks];

    DataBlock *blocks = fetch_data_blocks(num_blocks, indicies);
    
    
    unsigned long enoughSpace = write_inode(file->inode, indicies, num_blocks);
    printf("Enough Space : %d\n", enoughSpace);
    if(enoughSpace == 1){
    ret = write_data_blocks(buf, indicies, num_blocks, numbytes - amount_To_Add_In_Last_Block, amount_To_Add_In_Last_Block) + amount_To_Add_In_Last_Block;
    
    write_data_blocks_to_disk(num_blocks, indicies, numbytes);
    printf("ret = %lu\n",ret);


    write_data_block_bitmap_to_disk();
    write_inode_dir_bitmap_to_disk();



    }
    file->cursor_position = file->size;
    return ret;

    //currently doesn't handle indirect inode allocation

}

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos){//sets file->cursor_position to a pointer to data_blocks[last_block].block[position_in_last_block]

    init_globals();
    fserror = FS_NONE;
    printf("file size = %u\n",file->size);
    printf("inode size = %u\n", file->inode->size);
    unsigned long ret = 0;
    int how_Many_Blocks = file->inode->size;
    int how_Much_Data = file->size;
    int how_Much_In_Last_Block = how_Much_Data % 4096;
    int amount_To_Add_In_Last_Block = 4096 - how_Much_In_Last_Block;
    DataBlock last_Block;
    uint16_t addressOfLastBlock;

    int how_many_blocks_needed = (bytepos / SOFTWARE_DISK_BLOCK_SIZE+1) - (how_Many_Blocks) + 1; // this could be so wrong
    int position_in_last_block;
    int last_block_index;



    //at this point we should know how many new addresses we need
    //debating calling fetch data blocks or write inode first
    if(how_many_blocks_needed > 0){
    uint16_t indicies[how_many_blocks_needed];
    DataBlock *blocks = fetch_data_blocks(how_many_blocks_needed, indicies);
    last_block_index = write_inode_seek_file(file->inode, indicies, how_many_blocks_needed);
    //last_Block = data_blocks[last_block_index];
    position_in_last_block = (bytepos - file->size) % SOFTWARE_DISK_BLOCK_SIZE;
    file->cursor_position = data_blocks[last_block_index].block[position_in_last_block];
    }else{
        //how do I get the index of the last block without calling write_inode
        //don't know if it will be in direct or indirect
        
        position_in_last_block = bytepos % SOFTWARE_DISK_BLOCK_SIZE;
        file->cursor_position = data_blocks;
    }

    //bytepos is the position relative to the new file size decided
    //will probably use how_many_blocks_needed * SOFTWARE_DISK_BLOCK_SIZE
    //assume start at 0 then we want to go to file->size
    //probably need conditionals to determine how to calculate it


    //now have the location of the last block allocated for the seek_file operation
    //how do I actually put the cursor there.

    //i want to put it at data_blocks[last_block_index].blocks[position_in_last_block]
    

    

    
    fserror = FS_NONE;
    // printf("%u", file->cursor_position);
    return 1;

}

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file){
    return file->size;
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
//     File file2 = create_file("howdy partner");
//     // File file4 = create_file("howdy partner");
//     File file3 = create_file("hiya");
    
//     char buf[2000];
//     memset(buf, 'A', 2000);
//     write_file(file1, buf, sizeof(buf));
//     char buf2[SOFTWARE_DISK_BLOCK_SIZE*20];
//     memset(buf2, 'D', SOFTWARE_DISK_BLOCK_SIZE*20);
//     write_file(file1, buf2, sizeof(buf2));
    

//     return 0;
// }