#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex> 
#include <condition_variable>
#include <csignal>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <chrono>
#include <ctime>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "config.h"

using namespace std;
/*
typedef struct {
	int t;
} ffmpeg_manager_struct;
*/

extern int errno;


mutex config_mutex;
mutex t_mutex;
mutex proc_mutex;
mutex stop_mutex;
condition_variable stop_cv;

time_t last_checked;
pid_t ffmpeg_proc = -1;
int stop = 0;


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
		
	stop_cv.notify_one();
		
	cout << "Stop = " << stop << endl;
	
	stop_mutex.unlock();
	proc_mutex.unlock();
}



// Set camera parameters when changed in config file
int setup_camera(){
	config_mutex.lock();

	try {
		map<string,double> cam_settings = camera_config();	

		config_mutex.unlock();
		ostringstream sStream;

		sStream << "v4l2-ctl";

		for (auto& setting : cam_settings)
			sStream << " -c " << setting.first << "=" << (int)setting.second;
	
		string tmp = sStream.str();
		const char * cmd = tmp.c_str();
	
		system(cmd);
	} catch (const configNotFoundException& e) {
		cout << "Error: Not all camera parameters in config file" << endl;
		return -1;
	} catch (const exception& e){
		cout << "Error: " << e.what() << endl; 
		return -1;
	} 

	return 0;

}

// Function from which thread manages the ffmpeg process
void ffmpeg_process_manager(int t){
	int status = 0;
	string ffmpeg_cmd[] = {"/usr/bin/ffmpeg","-y","-i","/dev/video1","-s","1280x720",
		"-vcodec","copy","-t",to_string(t),"",
		"-t",to_string(t),"-vf", "fps=1/5",
		"-update","1","frame.jpg",
		"-vcodec","copy","-t",to_string(t),"f","flv","rtmp:/mousehotelserver@mousehotelserver.inf.ac.uk::8080/live"};
		
	time_t rawt;
	pid_t pid;
	struct tm *lt;	
	struct stat config_stat;

	while(1){
		unique_lock<mutex> stop_lock(stop_mutex);
		while(stop)
			stop_cv.wait(stop_lock);
		stop_lock.unlock();

		// Check config hasn't been modified
		if(stat(CONFIG,&config_stat)<0){
			perror("Error using stat");
		} else {
			if(difftime(config_stat.st_mtime,last_checked)>0){
				last_checked = config_stat.st_mtime;
				try{
					t = read_config("Time") * 60;
				} catch (const configNotFoundException& e){
					cout << "Error: " << e.what() << " - Time" << endl;
					cout << "Using previous value Time=" << t << endl;
				} catch (const exception& e){
					cout << e.what() << endl;
					exit(1);
				}
				//setup_camera();
			}
		}



		time(&rawt);
		lt = localtime(&rawt);
		ostringstream sStream;
		sStream << lt->tm_year+1900 << "-";
		sStream	<< setfill('0') << setw(2)
			<< lt->tm_mon+1 << "-";
		sStream	<< setfill('0') << setw(2)
			<< lt->tm_mday << "_";
		sStream	<< setfill('0') << setw(2)
			<< lt->tm_hour << ":";
		sStream	<< setfill('0') << setw(2)
			<< lt->tm_min << ":";
		sStream	<< setfill('0') << setw(2)
			<< lt->tm_sec << ".flv";

		string filename = sStream.str();		

		// rewrite time and filename
		ffmpeg_cmd[9] = ffmpeg_cmd[12] = to_string(t);
		ffmpeg_cmd[10] = filename;

		// Convert to char* for execv
		vector<char*> cmd_v;

		// add null terminators
		transform(begin(ffmpeg_cmd),end(ffmpeg_cmd),
			back_inserter(cmd_v),
			[](string& s){ s.push_back(0); return &s[0];});
		cmd_v.push_back(nullptr);

		char** c_cmd = cmd_v.data();

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

			//close(dev_null);

			if(execv("/usr/bin/ffmpeg",c_cmd) < 0){
				perror("Error execv");
				exit(1);
			}
			exit(0);
		} else {
			proc_mutex.lock();
			ffmpeg_proc = pid;
			proc_mutex.unlock();

			wait(&status);

			proc_mutex.lock();
			ffmpeg_proc = -1;
			proc_mutex.unlock();

			if(WIFEXITED(status))
				if(WEXITSTATUS(status) > 0){
				cout << "Ffmpeg Error, please check camera is attached" << endl;
				cout << "Run ffmpeg natively to debug" << endl;
				stop_mutex.lock();
				stop = 1;
				stop_mutex.unlock();
				}
		}
	}
	
}


int main(int argc, char *argv[]){
	double t = 15;
	int new_t = 0;
	string str;
	struct stat config_stat;

	if (argc > 1){
		t = strtod(argv[1],NULL);
		new_t = 1;
	}
	
	ConfigPair toSave;
	toSave.option = "Time"; toSave.value = t;

	config_mutex.lock();

	try {
		if(new_t)
			edit_config(toSave); 
		else
			t = read_config("Time");

	} catch (const configOpenException& e){
		create_config(t);
	} catch (const configNotFoundException& e){
		cout << "Error: " << e.what() << " - Time" << endl;
		cout << "Adding Time as " << t;
		add_config(toSave);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	}
	
	toSave.option = "pid"; toSave.value = getpid();

	try {
		edit_config(toSave); 
	} catch (const configNotFoundException& e){
		add_config(toSave);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	}

	config_mutex.unlock();

	if (setup_camera() < 0) {
		perror("Error setting up camera");
		exit(1);
	}

	if (signal(SIGUSR1,catch_SIGUSR) == SIG_ERR)
		perror("Error setting USRSIG1");

	if (signal(SIGUSR2,catch_SIGUSR) == SIG_ERR)
		perror("Error setting USRSIG2");

	t *= 60;

	if(stat(CONFIG,&config_stat)<0){
		perror("Error in stat");
		exit(1);
	}

	last_checked = config_stat.st_mtime;

	thread manager(ffmpeg_process_manager,t);

	// Posibly do things here

	manager.join();
	

	return 0;
}
