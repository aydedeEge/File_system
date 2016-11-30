#include "tests.h"

int verySimpleMKSFSTest(){
	mksfs(1);
	return 0;
}

void getNextFileTest(){
	char *fname = malloc(16*sizeof(char));
	sfs_get_next_file_name(fname);
	printf("%s\n", fname);
	free(fname);
}

void createFileTest(char* filename){
	sfs_create(filename);
}

int openFileTest(char *name){
	return sfs_fopen(name);
}

void writeFileTest(int FileID, char* buf, int length){
	sfs_fwrite(FileID, buf, length);
}

void WSeekTest(int fileID, int loc){
	sfs_fwseek(fileID, loc);
}

void RSeekTest(int fileID, int loc){
	sfs_frseek(fileID, loc);
}

int getFileSizeTest(char * path){
	return sfs_get_file_size(path);
}

void closeFileTest(int FileID){
	sfs_fclose(FileID);
}

void readFileTest(int FileID, char* buf, int length){
	sfs_fread(FileID, buf, length);
}

void removeFileTest(char *name){
	sfs_remove(name);
}

int main(){
	verySimpleMKSFSTest();

	int a = openFileTest("Red");
	char * buf = "Blue";

	int b = openFileTest("Purple");

	writeFileTest(a, buf, 4);

	char * read_buf = malloc(1024*sizeof(char));
	readFileTest(a, read_buf, 4);
	printf("Read1: %s\n", read_buf);
	free(read_buf);
	WSeekTest(a, 3);

	char * buf2 = "Green";
	int s1 = getFileSizeTest("Red");
	printf("%d\n", s1);

	writeFileTest(a, buf2, 5);
	read_buf = malloc(1024*sizeof(char));
	readFileTest(a, read_buf, 9);
	printf("Read2: %s\n", read_buf);
	free(read_buf);
	removeFileTest("Red");

	read_buf = malloc(1024*sizeof(char));
	readFileTest(a, read_buf, 9);
	printf("Read3: %s\n", read_buf);
	free(read_buf);

	s1 = getFileSizeTest("Red");
	printf("%d\n", s1);
	getNextFileTest();
	closeFileTest(b);
	getNextFileTest();
	getNextFileTest();
}