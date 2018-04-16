#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

using namespace std;

int main(int argc, char *argv[]){

	if (argc < 3)
		exit(1);

	try {
		edit_config(argv[1],argv[2]);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	} 

	exit(0);
}
