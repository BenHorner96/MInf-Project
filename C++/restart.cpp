#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

using namespace std;

int main(){
	pid_t proc = (pid_t)read_config("pid");

	kill(proc,SIGUSR1);

	return(1);
}
