#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

// RUN formatfs before conducting this test!


int main(int argc, char *argv[]) {
  int ret;
  File f1;
  File f2;
  File f3;
  char buf[1000];

  // should succeed
  f1=create_file("simple");
  printf("ret from create_file(\"blarg\") = %p\n",
	 f1);
  fs_print_error();

  if (f1) {
    // should succeed
    ret=write_file(f1, "hello", strlen("hello"));
    printf("ret from write_file(f, \"hello\", strlen(\"hello\") = %d\n",
	   ret);
    fs_print_error();
    
    //should fail with file not found
    f2 = open_file("howdy", READ_WRITE);
    printf("return from open_file = %p\n", f2);
    fs_print_error();
    
    // should succeed
    printf("Seeking to beginning of file.\n");
    seek_file(f1, 0);
    fs_print_error();
    
    // should succeed
    bzero(buf, 1000);
    ret=read_file(f1, buf, strlen("hello"));
    printf("ret from read_file(f, buf, strlen(\"hello\") = %d\n",
	   ret);
    printf("buf=\"%s\"\n", buf);
    fs_print_error();
    
    // should succeed
    close_file(f1);
    printf("Executed close_file(f).\n");
    fs_print_error();

    //should succeed
    ret = delete_file("simple");
    printf("return from delete_file = %d\n",ret);
    fs_print_error();
  }
  else {
    printf("FAIL.  Was formatfs run before this test?\n");
  }
  
  return 0;
}

  
  
  
  
