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

static void write_data_block_bitmap_to_disk()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
    memcpy(buf, data_bitmap, SOFTWARE_DISK_BLOCK_SIZE);
    int ret = write_sd_block(buf, DATA_BITMAP_BLOCK);
}

static void write_inode_blocks_to_disk()
{
    uint8_t buf[SOFTWARE_DISK_BLOCK_SIZE];
    int z = 0;
    for(int i = FIRST_INODE_BLOCK; i <= LAST_INODE_BLOCK; i++){

        bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
        memcpy(buf, inode_blocks[z].IBlock, SOFTWARE_DISK_BLOCK_SIZE);
        int ret = write_sd_block(buf, i);

        z++;
    }

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

       
        
        if(num_Addresses_To_Add_In_Direct > 0){

            printf("\nCASE 2\n");
            //get sam to look over this pretty please
            memcpy(inode->direct_addresses+(num_Addresses_Already_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Direct*2));
            uint32_t number = num_Addresses_To_Add_In_Direct;
            inode->size = inode->size + number;
            
            if(num_Addresses_To_Add_In_Indirect == 0){
                write_inode_blocks_to_disk();
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
                    write_inode_blocks_to_disk();
                    return 1;

                }else{
                    printf("\nCASE 4\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    write_inode_blocks_to_disk();
                    return 1;
                }
            }else{

                if(num_Of_Indirect_Addresses_In_Inode == 0){
                    printf("\nCASE 5\n");
                    memcpy(data_blocks[inode->indirect].block, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    write_inode_blocks_to_disk();

                    return 1;

                }else{
                    printf("\nCASE 6\n");
                    memcpy(data_blocks[inode->indirect].block+(num_Of_Indirect_Addresses_In_Inode*2)+1, addresses + (num_Addresses_To_Add_In_Direct*2)+1, (num_Addresses_To_Add_In_Indirect*2));
                    uint32_t number = num_Addresses_To_Add_In_Indirect;
                    inode->size = inode->size + number;
                    write_inode_blocks_to_disk();
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

    //int last_block;
    

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


//returns 1 on success and 0 on failure
unsigned long seek_past_file_length(File file, int howManyBlocksToAdd){
    //printf("CURRENT FILE NAME : %c",file->name);

    unsigned long ret;
    int how_Many_Blocks = file->inode->size;
    int how_Much_Data = file->size;
    int how_Much_In_Last_Block = how_Much_Data % 4096;
    int amount_To_Add_In_Last_Block = 4096 - how_Much_In_Last_Block;
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

            write_inode_blocks_to_disk();

        }
    
    fserror = FS_NONE;
    Inode *inode = file->inode;
    int num_blocks = howManyBlocksToAdd;
    uint16_t indicies[num_blocks];

    DataBlock *blocks = fetch_data_blocks(num_blocks, indicies);
    unsigned long enoughSpace = write_inode(file->inode, indicies, num_blocks);
    
    if(enoughSpace == 1){
    write_data_block_bitmap_to_disk();
    write_inode_dir_bitmap_to_disk();
    return 1;
    }
    fserror = FS_OUT_OF_SPACE;
    return 0;

}

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes){
    

    fserror = FS_NONE;
    //FILL THIS SHIT OUT PLEEEEEEEEEEEEEEAAAAAAAAAAAAASSSSSSSSSSSSSSEEEEEEEEEEEEEEE

    init_globals();
    
    unsigned long ret = 0;

    uint32_t position_To_Go_To = file->cursor_position;
    int Current_Block_Index = position_To_Go_To/SOFTWARE_DISK_BLOCK_SIZE;
    int how_Much_Data_In_Current = position_To_Go_To%SOFTWARE_DISK_BLOCK_SIZE;
    int amount_To_Add_In_Current_Block = 4096 - how_Much_Data_In_Current;
    int how_Many_Blocks = file->inode->size;
    int how_many_blocks_left = file->inode->size - Current_Block_Index + 1;
    int how_many_blocks_to_allocate;
    
    int how_Many_More_Blocks;

    //gives inode an index for first write
    
    if(file->inode->size == 0){
            
            int idx = find_free_block();
            printf("\n\nFREED BLOCK : %d\n\n",idx);
            mark_block(idx);
            uint16_t num = idx;
            file->inode->direct_addresses[0] = num;
            file->inode->size++;
            printf("inode size = %u\n", file->inode->size);
            how_Many_Blocks++;

            write_inode_blocks_to_disk();

        }


    //gives us how many more blocks we need to add to
    if(numbytes >= amount_To_Add_In_Current_Block ){
        //this if statement is so that our plus one doesn't fuck everything up we did the math
        if((numbytes - amount_To_Add_In_Current_Block)%SOFTWARE_DISK_BLOCK_SIZE == 0){
            how_Many_More_Blocks = ((numbytes - amount_To_Add_In_Current_Block)/SOFTWARE_DISK_BLOCK_SIZE);
        }else{
            how_Many_More_Blocks = ((numbytes - amount_To_Add_In_Current_Block)/SOFTWARE_DISK_BLOCK_SIZE)+1;
        }
    }else{
        how_Many_More_Blocks = 0;
    }
    //if we need more blocks get them
    if(how_many_blocks_left < how_Many_More_Blocks){
        how_many_blocks_to_allocate = how_Many_More_Blocks - how_many_blocks_left;
        uint16_t indicies[how_many_blocks_to_allocate];
        DataBlock *blocks = fetch_data_blocks(how_many_blocks_to_allocate, indicies);
        unsigned long enoughSpace = write_inode(file->inode, indicies, how_many_blocks_to_allocate);
    }
    
    
    DataBlock current_Block;
    uint16_t addressOfCurrentBlock;
//These statements pull the address
    if(Current_Block_Index > 13){
       //NOT FINISHED
        uint16_t current_Address_In_Indirect;
        int index_of_Current = Current_Block_Index - 13;
        current_Block = data_blocks[file->inode->indirect];
        memcpy(addressOfCurrentBlock,current_Block.block + (index_of_Current*2),2);
        current_Block = data_blocks[addressOfCurrentBlock];

    
    }else{
        current_Block = data_blocks[file->inode->direct_addresses[Current_Block_Index]];
        addressOfCurrentBlock = file->inode->direct_addresses[Current_Block_Index];
    }
//These statements fill the current block
    if(how_Many_More_Blocks > 0){

        memcpy(current_Block.block + how_Much_Data_In_Current, buf, amount_To_Add_In_Current_Block);
        write_sd_block(current_Block.block, addressOfCurrentBlock);
        printf("A\n");
    }else{
   
        printf("NUMBYTES : %lu\n",numbytes);
        memcpy(current_Block.block + how_Much_Data_In_Current, buf, numbytes);
        write_sd_block(current_Block.block, addressOfCurrentBlock);
        write_data_block_bitmap_to_disk();
        write_inode_dir_bitmap_to_disk();
        
        file->size += numbytes;
        file->cursor_position += numbytes;
        return numbytes;

    }

    int num_Blocks_To_Write_In_Direct;
    int num_Blocks_To_Write_In_Indirect;

    //if inode already uses indirect block then set numToAdd to be 0
    num_Blocks_To_Write_In_Direct = 13 - Current_Block_Index + 1; if( num_Blocks_To_Write_In_Direct < 0){num_Blocks_To_Write_In_Direct = 0;}
    //if inode does not need indirect block then set numToAdd to be 0
    num_Blocks_To_Write_In_Indirect = how_Many_More_Blocks - num_Blocks_To_Write_In_Direct; if(num_Blocks_To_Write_In_Indirect < 0){num_Blocks_To_Write_In_Indirect = 0;}

    uint16_t indirect_Addresses_Array[num_Blocks_To_Write_In_Indirect];
    int startAtBeginning = 0;

    if(num_Blocks_To_Write_In_Direct > 0){
        startAtBeginning = 1;
    }

    for(int i = 0; i < num_Blocks_To_Write_In_Indirect;i++){
        if(startAtBeginning == 1){
            memcpy(indirect_Addresses_Array[i], data_blocks[file->inode->indirect].block + (i*2), sizeof(uint16_t));
        }else{
            
        }
    }

    int bytesLeft = numbytes - amount_To_Add_In_Current_Block;
    if(num_Blocks_To_Write_In_Indirect > 0){




    }




    if(num_Blocks_To_Write_In_Direct > 0){

    }
    if(num_Blocks_To_Write_In_Indirect > 0){

    }



    
    
   
    
    
   
    ret = write_data_blocks(buf, indicies, num_blocks, numbytes - amount_To_Add_In_Current_Block, amount_To_Add_In_Current_Block) + amount_To_Add_In_Current_Block;
    
    write_data_blocks_to_disk(num_blocks, indicies, numbytes);
    printf("ret = %lu\n",ret);


    write_data_block_bitmap_to_disk();
    write_inode_dir_bitmap_to_disk();



    
    //file->cursor_position = file->size;
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
    if(bytepos > MAX_FILE_SIZE){
        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }
    unsigned long ret;
    file->cursor_position = bytepos;
    int how_Many_Blocks = file->inode->size;
    int how_many_blocks_needed = (bytepos / SOFTWARE_DISK_BLOCK_SIZE+1) - (how_Many_Blocks) + 1;

    if(how_many_blocks_needed > 0){
        
        ret = seek_past_file_length(file,how_many_blocks_needed);
        if(ret == 0){
            return ret;
        }else{
           file->size = bytepos;
           return ret; 
        }


    }else{
        return 1;
    }




    // printf("file size = %u\n",file->size);
    // printf("inode size = %u\n", file->inode->size);
    // unsigned long ret = 0;
    // int how_Many_Blocks = file->inode->size;
    // int how_Much_Data = file->size;
    // int how_Much_In_Last_Block = how_Much_Data % 4096;
    // int amount_To_Add_In_Last_Block = 4096 - how_Much_In_Last_Block;
    // DataBlock last_Block;
    // uint16_t addressOfLastBlock;

    // int how_many_blocks_needed = (bytepos / SOFTWARE_DISK_BLOCK_SIZE+1) - (how_Many_Blocks) + 1; // this could be so wrong
    // int position_in_last_block;
    // int last_block_index;



    // //at this point we should know how many new addresses we need
    // //debating calling fetch data blocks or write inode first
    // if(how_many_blocks_needed > 0){
    // uint16_t indicies[how_many_blocks_needed];
    // DataBlock *blocks = fetch_data_blocks(how_many_blocks_needed, indicies);
    // last_block_index = write_inode_seek_file(file->inode, indicies, how_many_blocks_needed);
    // //last_Block = data_blocks[last_block_index];
    // position_in_last_block = (bytepos) % SOFTWARE_DISK_BLOCK_SIZE;
    // file->cursor_position = data_blocks[last_block_index].block[position_in_last_block];
    // }else{
    //     //how do I get the index of the last block without calling write_inode
    //     //don't know if it will be in direct or indirect
        
    //     position_in_last_block = bytepos % SOFTWARE_DISK_BLOCK_SIZE;
    //     file->cursor_position = data_blocks;
    // }

    // bytepos is the position relative to the new file size decided
    // will probably use how_many_blocks_needed * SOFTWARE_DISK_BLOCK_SIZE
    // assume start at 0 then we want to go to file->size
    // probably need conditionals to determine how to calculate it


    // now have the location of the last block allocated for the seek_file operation
    // how do I actually put the cursor there.

    // i want to put it at data_blocks[last_block_index].blocks[position_in_last_block]
    

    

    
    // fserror = FS_NONE;
    // // printf("%u", file->cursor_position);
    // return 1;

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

int main(int argc, char *argv[]){
    init_software_disk();
    check_structure_alignment();
    File file1 = create_file("hello");
    // File file2 = create_file("hello");
    File file2 = create_file("howdy partner");
    // File file4 = create_file("howdy partner");
    File file3 = create_file("hiya");
    

    printf("\n\nBEFORE FILE 1 INODE SIZE : %u\n\n",file1->inode->size);
    char buf[2000];
    memset(buf, 'A', 2000);
    write_file(file1, buf, sizeof(buf));
    printf("\n\nAFTER FILE 1 INODE SIZE : %u\n\n",file1->inode->size);

    char buf2[SOFTWARE_DISK_BLOCK_SIZE-1000];
    memset(buf2, 'B', SOFTWARE_DISK_BLOCK_SIZE-1000);
    write_file(file2, buf2, sizeof(buf2));

    char buf3[SOFTWARE_DISK_BLOCK_SIZE];
    memset(buf3, 'C', SOFTWARE_DISK_BLOCK_SIZE);
    write_file(file3, buf3, sizeof(buf3));

    printf("\n\nFILE 1 INODE SIZE : %u\n\n",file1->inode->size);

    char buf4[2096];
    memset(buf4, 'D', 2096);
    write_file(file1, buf4, sizeof(buf4));
    

    return 0;
}