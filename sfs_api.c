#include "sfs_api.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 1024
#define MAX_BLOCK 100
#define INODE_COUNT 40
char *filename = "file_system";

/*ROOT DIRECTORY STRUCT*/
typedef struct root_directory_entry{
  int inode_id : 32;
  /*Should equate to a file name with max 16 charcters*/
  char filename[16];
  int in_use : 8;
}root_directory_entry;

/*I_NODE STRUCT*/
typedef struct I_Node{
  int size;
  int is_free;
  int block_pointers[12];
  int indirect_pointer;
}I_Node;

/*SUPER NODE STRUCT*/
typedef struct Super_Node{
  int magic_number : 32;
  int block_size : 32;
  int block_amount : 32;
  int i_node_block_length : 32;
  I_Node root_node;
}Super_Node;

/*FILE DESCRIPTOR TYPE*/
typedef struct File_Descriptor{
  int inode_id;
  int read_pointer;
  int write_pointer;
  int is_free;
}File_Descriptor;

/*All in memory tables and variables*/
root_directory_entry rt[INODE_COUNT];
int rt_pointer = 0;
I_Node inode_table[INODE_COUNT];
int bm[MAX_BLOCK];
int current_file_count;
File_Descriptor fd_table[INODE_COUNT];

/*Initialize all open fd entries to be empty*/
void init_fd_table(){
  for(int i=0; i<INODE_COUNT; i++){
    fd_table[i].is_free = 1;
  }
}

/*Return first free fd_table entry*/
int find_free_fd_entry(){
  for(int i=0; i<INODE_COUNT; i++){
    if(fd_table[i].is_free == 1){
      return i;
    }
  }
  return -1;
}

/*Initialize all inodes as empty except for inode pointing to root_directory*/
void init_inode_table(){

  for(int i=0; i<INODE_COUNT; i++){
    inode_table[i].size = 0;
    inode_table[i].is_free = 1;
    for(int j=0; j<12; j++){
      inode_table[i].block_pointers[j] = -1;
    }
  }

  write_blocks(1, 1, &inode_table);
}

/*Return first free inode in inode table*/
int find_free_inode(){
  for(int i=0; i<INODE_COUNT; i++){
    if(inode_table[i].is_free){
      return i;
    }
  }
  return -1;
}

/*Initialize root directory which links inode pointers to filenames*/
/*Maximum size of root_directory will be 22bytes per entry * 40 entries = 880bytes*/
void init_root_directory(){

  current_file_count = 0;

  for(int i=0; i<INODE_COUNT; i++){
    rt[i].inode_id = 0;
    // rt[i].filename = "";
    strcpy(rt[i].filename, "");
    rt[i].in_use = 0;
  }

  write_blocks(3, 1, &rt);

  /*Place it in the inode table*/
  void * buffer = malloc(1024*sizeof(char));
  read_blocks(1, 1, buffer);

  I_Node * inode = (I_Node*) buffer;
  free(buffer);

  inode[0].size = 1;
  inode[0].is_free = 0;
  inode[0].block_pointers[0] = 3;

  write_blocks(1, 1, inode);
}

/*Find the first free root directory entry*/
int get_free_directory_entry(){
  for(int i=0; i<INODE_COUNT; i++){
    if(!rt[i].in_use){
      return i;
    }
  }

  return -1;
}

/*Find the inode id of the file with name "filename"*/
int get_inode_id(char * filename){
  for(int i=0; i<INODE_COUNT; i++){
    if(!strcmp(rt[i].filename, filename)){
      return i;
    }
  }
  return -1;
}

/*Initialize the super node in the first block of the SFS*/
int init_fresh_super_node(){
  int magic_number = 666;
  /*Write the super block to the first block in the file system*/
  Super_Node super_node;
  super_node.magic_number = magic_number;
  super_node.block_size = BLOCK_SIZE;
  super_node.block_amount = MAX_BLOCK;
  super_node.i_node_block_length = INODE_COUNT;

  write_blocks(0, 1, &super_node);

  return 0;
}

/*Set inital values of BIT_MAP (4-99 inclusively will be empty)*/
void init_bit_map(){
  /*Super block in block 0*/
  bm[0] = 1;

  /*Inode table in block 1*/
  bm[1] = 1;

  /*Bit map in block 2*/
  bm[2] = 1;

  /*Directory table in block 3*/
  bm[3] = 1;

  /*All other blocks set to empty i.e. 0*/
  for(int i=4; i<MAX_BLOCK; i++){
    bm[i] = 0;
  }

  write_blocks(2, 1, &bm);

}

/*Iterate through bit map on disk and find first empty block i.e 0 value*/
int get_first_empty_block(){
  /*Itialize empty buffer of one block size to read bit map from disk*/
  void * buffer = malloc(1024*(sizeof(char)));
  read_blocks(2, 1, buffer);
  int * disk_bit_map = (int*) buffer;
  free(buffer);

  for(int i=0; i<MAX_BLOCK; i++){
    if(!disk_bit_map[i]){
      return i;
    }
  }

  return -1;
}

int * find_contiguous_blocks(int size){
  int to_return[size];
  for(int i=0; i<size; i++){
    to_return[i] = get_first_empty_block();
  }

  return to_return;

}

/*Return the number of files in the root directory*/
int get_file_count(){
  int count = 0;

  for(int i=0; i<INODE_COUNT; i++){
    if(rt[i].in_use){
      count++;
    }
  }

  return count;
}

void mksfs(int fresh){

	/*Init disc if it does not already exist*/
	if(fresh == 0){
		init_disk(filename, BLOCK_SIZE, MAX_BLOCK);
	}else{
		/*Disc does not already exist*/
		init_fresh_disk(filename, BLOCK_SIZE, MAX_BLOCK);
    init_fresh_super_node();
    init_bit_map();
    init_inode_table();
    init_root_directory();
    init_fd_table();
    
    /*Testing purposes*/
    void * buffer = malloc(1024*(sizeof(char)));
    read_blocks(0, 1, buffer);
    Super_Node* s = (Super_Node*) buffer;
    printf("Magic number: %d\n", s->magic_number);
    free(buffer);

    void * buffer2 = malloc(1024*(sizeof(char)));
    read_blocks(2, 1, buffer2);
    int * a = (int*) buffer2;
    printf("Bit map: %d\n", a[3]);
    free(buffer2);

    void * buffer3 = malloc(1024*(sizeof(char)));
    read_blocks(1, 1, buffer3);
    I_Node * inode = (I_Node*) buffer3;
    printf("INode table: %d\n", inode[0].block_pointers[0]);
    free(buffer3);

		fresh = 1;
	}

}

/*Find the next file being pointed in root_directory to and write filename into fname*/
int sfs_get_next_file_name(char *fname){

  /*Search through directory until end of directory reached or file found*/
  /*Root directory can be fragmented, hence the need for a loop*/
  while(1){
    printf("%d\n", rt_pointer);
    if(rt_pointer == INODE_COUNT){
      rt_pointer = 0;
      return 0;
    }

    if(!rt[rt_pointer].in_use){
      rt_pointer++;
    }else{
      strcpy(fname, rt[rt_pointer].filename);
      rt_pointer++;
      return 1;
    }
  }
}

/*Return the size of a file stored in the inode of that file.*/
int sfs_get_file_size(char* path){
  int inode = get_inode_id(path);

  return inode_table[inode].size;
}

/*Allocate inode and directory entry for new file*/
int sfs_create(char *name){
  int inode_index = find_free_inode();
  int free_directory_entry = get_free_directory_entry();

  /*No empty inodes*/
  if(inode_index > INODE_COUNT){
    return -1;
  }

  /*Setting inode values*/
  inode_table[inode_index].size = 0;
  inode_table[inode_index].is_free = 0;

  /*Setting directory values*/
  rt[free_directory_entry].inode_id = inode_index;
  rt[free_directory_entry].in_use = 1;
  strcpy(rt[free_directory_entry].filename, name);

  /*Increment number of files counter*/
  current_file_count++;
  return inode_index;
}

/*Steps to open file
1. Search for file in rt and find corresponding inode
2. If found, fopen file with append mode and store FILE pointer in open_files table and return inode
3. Else, create file on top of everything else*/
int sfs_fopen(char *name){
  int fd_table_index;
  int index = get_inode_id(name);

  /*File exists*/
  if(index>0){
    fd_table_index = find_free_fd_entry();

    fd_table[fd_table_index].inode_id = index;
    fd_table[fd_table_index].read_pointer = 0;
    /*Change this to actual end of file*/
    fd_table[fd_table_index].write_pointer = inode_table[index].size;
    fd_table[fd_table_index].is_free = 0;

    return fd_table_index;

  /*File does not exist*/
  }else{
    /*Create file*/
    int inode_index = sfs_create(name);
    fd_table_index = find_free_fd_entry();

    fd_table[fd_table_index].inode_id = inode_index;
    fd_table[fd_table_index].read_pointer = 0;
    fd_table[fd_table_index].write_pointer = 0;
    fd_table[fd_table_index].is_free = 0;

    return fd_table_index;
  }

  return -1;
}

/*Find the file in the fd_table and set all attributes of that entry to empty/free*/
int sfs_fclose(int fileID){
  fd_table[fileID].inode_id = -1;
  fd_table[fileID].is_free = 1;
  fd_table[fileID].read_pointer = 0;
  fd_table[fileID].write_pointer = 0;
  return 0;
}

/*Move the read pointer between the start and end of the file*/
int sfs_frseek(int fileID, int loc){
  int inode_id = fd_table[fileID].inode_id;
  I_Node in = inode_table[inode_id];

  if(loc > in.size){
    return -1;
  }

  fd_table[fileID].read_pointer = loc;

  return 0;
}

/*Move the write pointer between the start and end of the file*/
int sfs_fwseek(int fileID, int loc){
  int inode_id = fd_table[fileID].inode_id;
  I_Node in = inode_table[inode_id];

  if(loc > in.size){
    return -1;
  }

  fd_table[fileID].write_pointer = loc;

  return 0;
}

/*Write the contents of buf of size length to fileID
Strategy:
1. Get file descriptor from file descriptor table
2. Get Inode associated to file
3. Compute which block to write to based on file's write pointer
4. Compute size after write: write pointer + length
5. Compute remaining space after write_pointer
6. Read block containing write pointer and place in buffer
*/
int sfs_fwrite(int fileID, char *buf, int length){
  /*Check if file is open*/
  if(fd_table[fileID].is_free){
    return -1;
  }

  int inode_id = fd_table[fileID].inode_id;
  int write_pointer = fd_table[fileID].write_pointer;

  I_Node in = inode_table[inode_id];

  /*If block not yet written to, find free block to write to*/
  if(in.block_pointers[0] == -1){
    /*Search bitmap for empty block*/
    int free_block = get_first_empty_block();
    bm[free_block] = 1;
    in.block_pointers[0] = free_block;
  }

  /*Determinine the amount to be added to file.*/
  /*If the size of the write_pointer offset and the length of the file to be added
  is larger than the total size of the file before the write operation, than the new size 
  will be write pointer offset + length of file.
  Else size stays the same.*/
  int new_size = write_pointer + length;
  int size_in_blocks = (length/1024)+1;
  if(new_size>in.size){
    in.size = new_size;
  }

  /*block to write is the block which contains the write_pointer 
  and the block in which writing will being*/
  int block_to_write = write_pointer/1024;

  /*block to end is the block in which the writing will stop*/
  int block_to_end = (write_pointer+length)/1024;

  /*Check if the starting node is greater than 10, requiring a search in the indirection node*/
  if(block_to_write>11){
    /*Check indirection block*/
  }

  if(block_to_end>11){
    /*Check indrection block*/
  }

  /*
  1) Container will act as a container for the old content before the write pointer, the new content,
    and the content after the write_pointer+length.
  2) Read the block in which the write pointer is located into void buffer.
  3) Cast to char pointer and copy into container.
  4) Copy the write content into the container starting at the write_pointer offset.
  */
  char * container = malloc(size_in_blocks*1024*sizeof(char));

  void * start_buffer = malloc(1024*sizeof(char));
  read_blocks(in.block_pointers[block_to_write], 1, start_buffer);
  char * start_block = (char*) start_buffer;
  free(start_buffer);

  /*Block holding write pointer is placed in container*/
  strcpy(container, start_block);

  /*Content to be written, now written to position of write_pointer
  Container now contains the old block and the new content*/
  strcpy(container+write_pointer, buf);

  /*Keep writing what's in container until nothing left*/
  int current_block = block_to_write;
  char * current_content_block;
  int free_block;
  while(size_in_blocks>0){

    /*If block not yet written to, find free block to write to*/
    if(in.block_pointers[current_block] == -1){
      /*Search bitmap for empty block*/
      free_block = get_first_empty_block();
      bm[free_block] = 1;
      in.block_pointers[current_block] = free_block;
    }

    /*Segment contianer into block increments*/
    current_content_block = malloc(1024*sizeof(char));
    strncpy(current_content_block, container, 1024);

    /*Remove 1 blocks worth of content*/
    strcpy(container, container+1024);

    /*Write the block segment into */
    write_blocks(in.block_pointers[current_block], 1, current_content_block);
    free(current_content_block);

    current_block++;
    size_in_blocks--;
  }

  /*Update fd_table and inode table*/
  fd_table[fileID].write_pointer = write_pointer + length;
  inode_table[inode_id].size = in.size;
  for(int k=0; k<12; k++){
    inode_table[inode_id].block_pointers[k] = in.block_pointers[k];
  }
  

  /*Flush changes to inode table and bitmap*/
  write_blocks(1, 1, &inode_table);
  write_blocks(2, 1, &bm);

  // /*PURELY FOR TESTING*/
  // void * b = malloc(4096*sizeof(char));

  // read_blocks(4,1,b);

  // char * c = (char*) b;
  // printf("Result: %s\n", c);


  free(container);
  return length;
}

/**/
int sfs_fread(int fileID, char *buf, int length){
  /*Check if file is open*/
  if(fd_table[fileID].is_free){
    return -1;
  }

  int inode_id = fd_table[fileID].inode_id;
  int read_pointer = fd_table[fileID].read_pointer;
  I_Node in = inode_table[inode_id];

  /*Find the block in which the read pointer is located*/
  int block_of_read_pointer = read_pointer/1024;

  /*Find the block which the read pointer will stop reading at*/
  int block_last_read = (read_pointer+length)/1024;

  /*Check to see if block_of_read_pointer and block_last_read are the same block*/
  if(block_of_read_pointer==block_last_read){
    void * only_read_buffer = malloc(1024*sizeof(char));
    read_blocks(in.block_pointers[block_of_read_pointer], 1, only_read_buffer);
    char * only_read_block = (char*)only_read_buffer;
    free(only_read_buffer);

    strcpy(buf, only_read_block+(read_pointer%1024));
    strcpy(buf+(read_pointer%1024)+(length%1024), "");
    return length;
  }

  /*Look in indirection pointer*/
  if(block_of_read_pointer>11){

  }

  /*Look in indirection pointer*/
  if(block_last_read>11){

  }

  /*Read the first block containing the read pointer.
    Then read every block in between the read pointer and the last read block.
    Then read the last read block.*/

  /*Read the first block into inital container*/
  void * first_read_buffer = malloc(1024*sizeof(char));
  read_blocks(in.block_pointers[block_of_read_pointer], 1, first_read_buffer);
  char * first_read_block = (char*)first_read_buffer;
  free(first_read_buffer);

  /*Buf will now contain the content of the block containing the read_pointer*/
  strcpy(buf, first_read_block+(read_pointer%1024));

  /*Loop through each block between the read_pointer block and the last read block*/
  int current = block_of_read_pointer;
  void * read_buffer;
  char * read_block;
  while(current<block_last_read){

    read_buffer = malloc(1024*sizeof(char));
    read_blocks(in.block_pointers[current], 1, read_buffer);
    read_block = (char*)first_read_buffer;
    free(read_buffer);

    strcpy(buf+strlen(buf), read_block);

    current++;
  }

  /*Read the last block up to the specified length*/
  void * last_read_buffer = malloc(1024*sizeof(char));
  read_blocks(in.block_pointers[block_last_read], 1, last_read_buffer);
  char * last_read_block = (char*)last_read_buffer;
  free(last_read_buffer);

  /*Copy the last block portion into the buf*/
  strcpy(buf+strlen(buf), last_read_block);

  return length;
}
int sfs_remove(char *file){
  	return 0;
}

