#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex> 
#include <csignal>
#include "config.h"
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <chrono>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;
/*
typedef struct {
	int t;
} ffmpeg_manager_struct;
*/


mutex config_mutex;
mutex t_mutex;
mutex proc_mutex;
mutex stop_mutex;

pid_t ffmpeg_proc = -1;
stop = 0;


/*
** Catch USRSIG1 and kill current ffmpeg process.
** Usually used when t has been changed in config file
** and you want immediate effect, rather than after current
** video is finished (default).
*/ 
void catch_SIGUSR(int signo) {
	proc_mutex.lock();
	stop_mutex.lock();
	if(ffmpeg_proc > 0){
		kill(ffmpeg_proc,SIGKILL);
	}
	
	if(signo == SIGUSR1)
		stop = 0;
	else
		stop = 1 - stop;
		
	stop_mutex.unlock();
	proc_mutex.unlock();
}



// Set camera parameters when changed in config file
// TODO
int setup_camera(){
	return 0;
}

// Function from which thread manages the ffmpeg process
void *ffmpeg_process_manager(void *args){
	string ffmpeg_cmd;
	time_t rawt;
	pid_t pid;
	struct tm *lt;	
	ffmpeg_manager_struct *fms = args;
	int t = fms->t;

	free(args);
	
	while(1){
		// TODO: Check config has been updated

		time(&rawt);
		lt = localtime(&rawt);
		//char *ffmpeg_args[] = {"
		sprintf(ffmpeg_cmd,"ffmpeg -i /dev/video1 -s 1280x720 " 
			"-vcodec copy -t %d %d-%02d-%02d_%02d:%02d:%02d.flv "
			"-vcodec copy -t %d -r 1/5 -f image2 "
			"-update 1 frame.jpg "
//			"> /dev/null 2>&1"
			,t, lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,
			lt->tm_hour,lt->tm_min,lt->tm_sec,t);

		if (!(pid=fork())) {
			int dev_null = open("/dev/null",O_WRONLY);
			if(dev_null < 0){
				perror("Error opening dev/null");
				exit(1);
			}
			
			if(dup2(dev_null,STDOUT_FILENO) < 0){
				perror("Error in dup2");
				exit(1);
			}

			close(dev_null);

			system(ffmpeg_cmd);
			exit(0);
		} else {
			pthread_mutex_lock(&proc_mutex);
			ffmpeg_proc = pid;
			pthread_mutex_unlock(&proc_mutex);

			wait(NULL);

			pthread_mutex_lock(&proc_mutex);
			ffmpeg_proc = -1;
			pthread_mutex_unlock(&proc_mutex);

			strcpy(ffmpeg_cmd,"\0");
		}
	}
	
}


int main(int argc, char *argv[]){
	double t = 15;
	int new_t = 0;
	string str;
	pthread_t manager;

	if (argc > 1){
		t = strtod(argv[1],NULL);
		new_t = 1;
	}

	config_mutex.lock();
	if ((config = fopen(config_path,"r")) == NULL){
		if(errno == ENOENT){
			printf("Creating new config file\n");
			config = create_config(config_path,t);
		} else {
			perror("Error accessing config file");
			exit(1);
		}
	} else {
		if(new){
			if(edit_config(config,"Time",t) < 0)
				exit(1);
		} else {
			while(strcmp(str,"Time")){
				fscanf(config,"%s\t%lf",str,&t);
				printf("%s\t%lf\n",str,t);	
			}
			rewind(config);
		}
		
	}

	fclose(config);
	
	pthread_mutex_unlock(&config_mutex);

	if (signal(SIGUSR1,catch_USRSIG1) == SIG_ERR)
		perror("Error setting USRSIG1");

	t *= 60;

	ffmpeg_manager_struct *args = malloc(sizeof *args);
	args->t = t;


	if(pthread_create(&manager,NULL,ffmpeg_process_manager,args)){
		perror("Error creating manager thread");
		free(args);
		exit(1);
	}

	// Posibly do things here
	pthread_join(manager,NULL);


	return 0;
}
