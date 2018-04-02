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
#include "spdlog/spdlog.h"

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
	auto capture = spdlog::get("capture");
	capture->info("Received SIGUSR, stopping ffmpeg process");
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
		
	capture->info("Setting stop to " + stop);
	
	stop_mutex.unlock();
	proc_mutex.unlock();
}



// Set camera parameters when changed in config file
int setup_camera(){
	auto capture = spdlog::get("capture");
	config_mutex.lock();

	try {
		map<string,string> cam_settings = camera_config();	

		config_mutex.unlock();
		ostringstream sStream;

		sStream << "v4l2-ctl";

		for (auto& setting : cam_settings)
			sStream << " -c " << setting.first << "=" << setting.second;
	
		string tmp = sStream.str();
		const char * cmd = tmp.c_str();
	
		system(cmd);
	} catch (const configNotFoundException& e) {
		capture->error("Not all camera parameters in config file");
		return -1;
	} catch (const exception& e){
		capture->error(e.what()); 
		return -1;
	} 

	return 0;

}

// Function from which thread manages the ffmpeg process
void ffmpeg_process_manager(int t, int mode, string name){
	auto capture = spdlog::get("capture");
	int status = 0; int total_t = -1;
	int recorded_t = 0;
	string ffmpeg_cmd[] = {"/usr/bin/ffmpeg","-y","-i","/dev/video1","-s","1280x720",
		"-vcodec","copy","-t",to_string(t),"",
		"-t",to_string(t),"-vf", "fps=1/5",
		"-update","1","../Network/frame.jpg",
		"-vcodec","copy","-t",to_string(t),"-f","flv","rtmp://mousehotelserver.inf.ed.ac.uk::8080/live/test"};
		
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
			capture->warn("Error using stat to check last access of config file");
		} else {
			if(difftime(config_stat.st_mtime,last_checked)>0){
				// If it has then reload time, mode, name, and set camera params
				config_mutex.lock();
				last_checked = config_stat.st_mtime;
				try{
					t = stoi(read_config("time")) * 60;
					if(t < 1){
						capture->warn("time in config is less than one, setting to default of 15");
						t = 15;
						ConfigPair toSave;
						toSave.option = "time";
						toSave.value = t;
						edit_config(toSave);
						t *= 60;
					}

					// If the mode changes, reset the recorded time
					int mode_change = mode;
					mode = stoi(read_config("mode"));

					if (mode_change != mode)
						recorded_t = 0;

					name = read_config("name");

				} catch (const configNotFoundException& e){
					capture->warn(e.what());
					capture->info("Using previous value of " + t);
				} catch (const exception& e){
					capture->error(e.what());
					exit(1);
				}
				config_mutex.unlock();
				//setup_camera();
			}
		}

		string filename;
		// Get current time
		time(&rawt);
		lt = localtime(&rawt);
	
		if(mode){
			// If the setting is mode 1 then read the config file for
			// total time of session
			// total_t == 0 means to record indefinitely
			config_mutex.lock();
			try {
				if(total_t < 0)
					total_t = stoi(read_config("total_t"));
			} catch (const configNotFoundException& e) {
				capture->error(e.what());
				exit(1);
			} catch (const exception& e){
				capture->error(e.what()); 
				exit(1);
			} 
			 
			config_mutex.unlock();

			// TODO: set filename	
		} else {
			ostringstream sStream;
			sStream << "Video/";
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
			
			filename = sStream.str();		
		}


		capture->info("Filename set as " + filename);
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

		// Fork and child process will execute ffmpeg command
		if (!(pid=fork())) {
			int out_file = open("ffmpeg.log",O_RDWR|O_CREAT|O_TRUNC,0666);
			if(out_file < 0){
				capture->error("Error opening ffmpeg.log");
				exit(1);
			}
			
			// Redirect output, must be able to run a daemon
			if(dup2(out_file,STDOUT_FILENO) < 0){
				capture->error("Error using dup2 to redirect stdout of ffmpeg process to ffmpeg.log");
				exit(1);
			}
			if(dup2(out_file,STDERR_FILENO) < 0){
				capture->error("Error using dup2 to redirect stderr of ffmpeg process to ffmpeg.log");
				exit(1);
			}

			// Execute command
			capture->info("Executing ffmpeg");
			if(execv("/usr/bin/ffmpeg",c_cmd) < 0){
				capture->error("Error using execv to start ffmpeg");
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

			capture->info("Recording complete");

			
			// Check that the process exited with a status
			if(WIFEXITED(status)){
				// Check whether there was an error
				if(WEXITSTATUS(status) > 0){
					capture->warn("Ffmpeg returned with an error, stopping recording");
					capture->warn("Run ffmpeg natively to debug");
					stop_mutex.lock();
					stop = 1;
					stop_mutex.unlock();
				}

				// If in mode == 1 with total time set
				// must track total recorded mins
				if(mode){
					if(total_t){
						recorded_t += t;
						if(recorded_t >= total_t){
							capture->info("Experiment completed, stopping recording");
		
							stop_mutex.lock();
							stop = 1;
							stop_mutex.unlock();
						}
					}
				}
			}
		}
	}
	
}


int main(int argc, char *argv[]){
	int t = DEFAULT_time;
	
	// mode = 0 means to record at all times without session title
	// mode = 1 means to wait until session title has been provided
	int mode = DEFAULT_mode;
	int new_t = 0;
	int new_m = 0;
	string name = DEFAULT_name;
	struct stat config_stat;
    	auto capture = spdlog::basic_logger_mt("capture","logs/capture.log");

	// Check if mode was provided
	if (argc > 1){
		mode = stoi(argv[1],NULL);
		new_m = 1;
		if(mode != 0) mode = 1;
	}

	// Check if time was provided
	if (argc > 2){
		t = stoi(argv[1],NULL);
		new_t = 1;
		if(t < 1){
			capture->warn("time less than one, ignoring input");
			new_t = 0;
			t = DEFAULT_time;
		}
	}

	capture->info("Starting setup");


	int out_file = open("logs/capture.log",O_RDWR|O_CREAT|O_TRUNC,0666);
	if(out_file < 0){
		capture->error("Error opening capture.log");
		exit(1);
	}

	
	if(dup2(out_file,STDOUT_FILENO) < 0){
		capture->error("Error using dup2 to redirect stdout to capture.log");
		exit(1);
	}
	if(dup2(out_file,STDERR_FILENO) < 0){
		capture->error("Error using dup2 to redirect stderr to capture.log");
		exit(1);
	}
	
	config_mutex.lock();


	// Read mode from config or save to config if new mode provided
	ConfigPair toSave;
	toSave.option = "mode"; toSave.value = mode;

	try {
		if(new_m)
			edit_config(toSave); 
		else
			mode = stoi(read_config("mode"));

		if(mode != 0) mode = 1;

	} catch (const configOpenException& e){
		create_config(t);
	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding mode as " + to_string(mode));
		add_config(toSave);
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	// Read time from config or save to config if new one provided
	toSave.option = "time"; toSave.value = t;

	try {
		if(new_t)
			edit_config(toSave); 
		else
			t = stoi(read_config("time"));

		if(t < 1){
			capture->warn("time in config is less than one, setting to default of " + to_string(DEFAULT_time));
			t = DEFAULT_time;
			toSave.value = t;
			edit_config(toSave);
		}

	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding time as " + to_string(t));
		add_config(toSave);
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}
	
	// Read name from config
	try {
		name = read_config("name");
	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding name as default of " + string(DEFAULT_name));
		add_config(toSave);
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}
	// Save pid in config such that signals can easily be sent
	toSave.option = "pid"; toSave.value = getpid();

	try {
		edit_config(toSave); 
	} catch (const configNotFoundException& e){
		add_config(toSave);
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	config_mutex.unlock();

//	if (setup_camera() < 0) {
//		capture->warn("Could not setup camera correctly");
//	}


	// Set to catch user signals
	if (signal(SIGUSR1,catch_SIGUSR) == SIG_ERR)
		capture->error("Error setting USRSIG1");

	if (signal(SIGUSR2,catch_SIGUSR) == SIG_ERR)
		capture->error("Error setting USRSIG1");

	// Convert time to seconds
	t *= 60;

	// Check time config file was last modified
	if(stat(CONFIG,&config_stat)<0){
		capture->error("Error using stat to check last access of config file");
		exit(1);
	}

	last_checked = config_stat.st_mtime;

	// Set stop to 1 if mode is not 0
	stop_mutex.lock();
	stop = mode;
	stop_mutex.unlock();


	capture->info("Setup complete, starting ffmpeg_process_manager thread");

	thread manager(ffmpeg_process_manager,t,mode,name);

	// Posibly do things here relating to image processing

	manager.join();
	

	return 0;
}
