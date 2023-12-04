#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

// RUN formatfs before conducting this test!

int main(int argc, char *argv[]){
    init_software_disk();
    check_structure_alignment();
    File file1 = create_file("hello");
    // File file2 = create_file("hello");
    File file2 = create_file("howdy partner");
    // File file4 = create_file("howdy partner");
    File file3 = create_file("hiya");
   
    int ret;

    char buf[(SOFTWARE_DISK_BLOCK_SIZE/2)];
    memset(buf, 'A', (SOFTWARE_DISK_BLOCK_SIZE/2));
    ret = write_file(file1, buf, sizeof(buf));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);
    char buf2[(SOFTWARE_DISK_BLOCK_SIZE/2)];
    memset(buf2, 'B', (SOFTWARE_DISK_BLOCK_SIZE/2));
    ret = write_file(file2, buf2, sizeof(buf2));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);
    char buf3[SOFTWARE_DISK_BLOCK_SIZE/2];
    memset(buf3, 'C', SOFTWARE_DISK_BLOCK_SIZE/2);
    ret = write_file(file3, buf3, sizeof(buf3));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);
    char buf4[SOFTWARE_DISK_BLOCK_SIZE/2];
    memset(buf4, 'D', SOFTWARE_DISK_BLOCK_SIZE/2);
    ret = write_file(file1, buf4, sizeof(buf4));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);
    char buf5[SOFTWARE_DISK_BLOCK_SIZE/2];
    memset(buf5, 'E', SOFTWARE_DISK_BLOCK_SIZE/2);
    ret = write_file(file2, buf5, sizeof(buf5));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);
    char buf6[SOFTWARE_DISK_BLOCK_SIZE/2];
    memset(buf6, 'F', SOFTWARE_DISK_BLOCK_SIZE/2);
    ret = write_file(file3, buf6, sizeof(buf6));
    printf("ret from write_file(f, buf, SOFTWARE_DISK_BLOCK_SIZE) = %d\n",ret);

    return 0;
}