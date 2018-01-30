#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct {
	int t;
} ffmpeg_manager_struct;

extern int errno;

pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t proc_mutex = PTHREAD_MUTEX_INITIALIZER;

pid_t ffmpeg_proc = -1;


/*
** Catch USRSIG1 and kill current ffmpeg process.
** Usually used when t has been changed in config file
** and you want immediate effect, rather than after current
** video is finished (default).
*/ 
static void catch_USRSIG1(int signo) {
	pthread_mutex_lock(&proc_mutex);
	if(ffmpeg_proc != -1){
		kill(ffmpeg_proc,SIGKILL);
	}
	pthread_mutex_unlock(&proc_mutex);
}


// Create a new config file
static FILE *create_config(char *config_path, int t){
	FILE *config;

	if ((config = fopen(config_path,"w+")) == NULL){
		perror("Failed to create config file");
		exit(1);
	}

	if (fprintf(config, "Time\t%d\n",t) < 0)

	return config;
}

// Edit a value in the config file
// Acquire the config mutex before calling this function!!
static int edit_config(FILE *config, char *edit, double value){
	FILE *temp;
	char buf[255], save[255];
	double old = 0;

	if ((temp = fopen(".temp","w+")) == NULL){
		perror("Error editing config file");
		return -1;
	}

	rewind(config);

	while(1){
		if(fscanf(config,"%s\t%lf",buf,&old) == EOF)
			break;
		if(strcmp(buf,edit)){
			sprintf(save,"%s\t%lf\n",buf,old);
			fputs(save,temp);
			strcpy(save,"\0");
		}
	}

	sprintf(save,"%s\t%f",edit,value);
	fputs(save,temp);

	fclose(temp);
	fclose(config);

	// TODO: Need more error handling here
	remove(".capture");
	rename(".temp",".capture");

	if ((config = fopen(".capture","r+")) == NULL){
		perror("Error reopening config file after edit");
		return -2;
	}

	return 0;
}

// Set camera parameters when changed in config file
// TODO
static int setup_camera(){
	return 0;
}

// Function from which thread manages the ffmpeg process
static void *ffmpeg_process_manager(void *args){
	char ffmpeg_cmd[512];
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
			"-vcodec copy -t %d %d-%02d-%02d_%02d:%02d:%02d.flv"
			" > /dev/null 2>&1",
			t, lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,
			lt->tm_hour,lt->tm_min,lt->tm_sec);

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
	int new = 0;
	char str[255];
	char *config_path = ".capture";
	pthread_t manager;
	FILE *config;

	if (argc > 1){
		t = strtol(argv[1],NULL,0);
		new = 1;
	}

	pthread_mutex_lock(&config_mutex);
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
