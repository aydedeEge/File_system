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

int main(){
	verySimpleMKSFSTest();
	int a = openFileTest("Red");
	char * buf = "Blue";
	writeFileTest(a, buf, 4);
	char * buf2 = "Green";
	writeFileTest(a, buf2, 5);
	getNextFileTest();
}