#include "filesystem.h"
#include "softwaredisk.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, char *argv[]){

init_software_disk();
check_structure_alignment();

return 1;

}