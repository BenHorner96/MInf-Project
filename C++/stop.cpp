#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

using namespace std;

int main(){
	try {
		pid_t proc = (pid_t)stoi(read_config("pid"));
	
		kill(proc,SIGUSR2);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	} 

	exit(0);
}
