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

extern int errno;


mutex config_mutex;
mutex proc_mutex;
mutex stop_mutex;
condition_variable stop_cv;

time_t last_checked;
pid_t ffmpeg_proc = -1;
int stop = 0;


/*
** Catch USRSIG1 and kill current ffmpeg process.
** Usually used when video_time has been changed in config file
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
		stop = 1;
		
	stop_cv.notify_one();
		
	capture->info("Setting stop to " + stop);
	
	stop_mutex.unlock();
	proc_mutex.unlock();
}



// Set camera parameters
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
void ffmpeg_process_manager(int video_time, int mode, string name, int experiment_time){
	auto capture = spdlog::get("capture");
	int status = 0;
	int recorded_time = 0;
	int session_number = 0;

	// exec family require a list of arguments
	string ffmpeg_cmd[] = {"/usr/bin/ffmpeg","-y","-i","/dev/video1","-s","1280x720",
		"-vcodec","copy","-t",to_string(video_time),"",
		"-t",to_string(video_time),"-vf", "fps=1/5",
		"-update","1","../Network/frame.jpg",
		"-vcodec","copy","-t",to_string(video_time),"-f","flv","rtmp://mousehotelserver.inf.ed.ac.uk::8080/live/test"};
		
	time_t rawt;
	pid_t pid;
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
				// If it has then reload video_time, mode, name, and set camera params
				int mode_prev = mode;
				string name_prev = name;
				
				last_checked = config_stat.st_mtime;

				config_mutex.lock();
				try{

					video_time = stoi(read_config("video_time")) * 60;
					mode = stoi(read_config("mode"));
					name = read_config("name");
					experiment_time = stoi(read_config("experiment_time"));


				} catch (const configNotFoundException& e){
					capture->warn(e.what());
					capture->warn("Setting stop = 1");
					
				} catch (const exception& e){
					capture->error(e.what());
					exit(1);
				}
				config_mutex.unlock();

				// Check mode changed
				if (mode_prev != mode){
					capture->info("Detecting mode change to " + to_string(mode));
					
					// Check mode
					if (mode){
						// Set stop = 1
						stop_mutex.lock();
						stop = 1;
						stop_mutex.unlock();
						continue;
					} else {
						// Reset session number and recorded time
						recorded_time = 0;
						session_number = 0;
					}
				}

				// Check mode
				if (mode){
					// Check name changed
					if (name_prev != name){
						capture->info("Name changed detected, starting new experiment");
						// Reset session number and recorded time
						recorded_time = 0;
						session_number = 0;

						// Set stop = 1
						stop_mutex.lock();
						stop = 1;
						stop_mutex.unlock();
						continue;
					}
				}
					
				if (video_time < 1){
					capture->warn("video_time in config is less than one, setting to default");
					video_time = DEFAULT_video_time;
					edit_config("video_time",to_string(video_time));
					video_time *= 60;
				}

				ffmpeg_cmd[9] = ffmpeg_cmd[12] = to_string(video_time);

				if (experiment_time < video_time){
					capture->warn("experiment_time in config is less than video_time, setting to video_time");
					experiment_time = video_time;
					edit_config("experiment_time",to_string(experiment_time));
					experiment_time *= 60;
				}

			}
		}

		string filename;
		// Get current time
		time(&rawt);
		ostringstream sStream;
	
		sStream << "../video/";
		sStream << rawt;
		
		if(mode){
			// If the setting is mode 1 then append session number and name

			sStream	<< "_";
			sStream	<< setfill('0') << setw(6)
				<< session_number;
			sStream	<< "_" << name;
			
		}

		sStream << ".flv";
		filename = sStream.str();		

		capture->info("Filename set as " + filename);
		// rewrite video_time and filename
		ffmpeg_cmd[10] = filename;

		// Convert to char* for execv
		vector<char*> cmd_v;

		// add null terminators
		transform(begin(ffmpeg_cmd),end(ffmpeg_cmd),
			back_inserter(cmd_v),
			[](string& s){ s.push_back(0); return &s[0];});
		cmd_v.push_back(nullptr);

		char** c_cmd = cmd_v.data();

		// Check stop in case it was set during previous set up
		stop_mutex.lock();
		if (stop){
			stop_mutex.unlock();
			 continue;
		}
		stop_mutex.unlock();

		// Fork and child process will execute ffmpeg command
		if (!(pid=fork())) {
			proc_mutex.lock();
			ffmpeg_proc = getpid();
			proc_mutex.unlock();

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

			wait(&status);

			proc_mutex.lock();
			ffmpeg_proc = -1;
			proc_mutex.unlock();

			capture->info("Recording complete");

			// Check that the process was terminated by a signal
			// Assume if so that it was capture control
			// And so do not update recorded time or stop
			if (WIFSIGNALED(status)) continue;

			
			// Check that the process exited normally
			if (WIFEXITED(status)){
				// Check whether there was an error
				if (WEXITSTATUS(status) > 0){
					capture->warn("Ffmpeg returned with an error, stopping recording");
					capture->warn("Run ffmpeg natively to debug");
					stop_mutex.lock();
					stop = 1;
					stop_mutex.unlock();
				}

				// If in mode 1 
				// must track total recorded time
				if (mode){
					recorded_time += video_time;
					if (recorded_time >= experiment_time){
						capture->info("Experiment completed, stopping recording");
	
						session_number = 0;
						recorded_time = 0;
 
						stop_mutex.lock();
						stop = 1;
						stop_mutex.unlock();
					}
				}
			}
		}
	}
	
}


int main(int argc, char *argv[]){
	int video_time = DEFAULT_video_time;
	int experiment_time = DEFAULT_experiment_time;
	
	// mode = 0 means to record at all video_times without session title
	// mode = 1 means to wait until session title has been provided
	int mode = DEFAULT_mode;
	string name = DEFAULT_name;
	struct stat config_stat;
    	auto capture = spdlog::basic_logger_mt("capture","logs/capture.log");


	capture->info("Starting setup");


	int out_file = open("logs/stdout.log",O_RDWR|O_CREAT|O_TRUNC,0666);
	if(out_file < 0){
		capture->error("Error opening stdout.log");
		exit(1);
	}

	
	if(dup2(out_file,STDOUT_FILENO) < 0){
		capture->error("Error using dup2 to redirect stdout to stdout.log");
		exit(1);
	}
	if(dup2(out_file,STDERR_FILENO) < 0){
		capture->error("Error using dup2 to redirect stderr to stdout.log");
		exit(1);
	}
	
	config_mutex.lock();


	// Read mode from config
	try {
		mode = stoi(read_config("mode"));

		if(mode != 0) mode = 1;

	} catch (const configOpenException& e){
		create_config();
	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding mode as " + to_string(mode));
		add_config("mode",to_string(mode));
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	// Read video_time from config
	try {
		video_time = stoi(read_config("video_time"));

		if(video_time < 1){
			capture->warn("video_time in config is less than one, setting to default of " + to_string(DEFAULT_video_time));
			video_time = DEFAULT_video_time;
			edit_config("video_time",to_string(video_time));
		}

	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding video_time as " + to_string(video_time));
		add_config("video_time",to_string(video_time));
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	// Read experiment_time from config
	try {
		experiment_time = stoi(read_config("experiment_time"));

		if(experiment_time < video_time){
			capture->warn("experiment_time in config is less than video_time, setting to video time");
			experiment_time = video_time;
			edit_config("experiment_time",to_string(video_time));
		}

	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding video_time as video_time of " + to_string(video_time));
		add_config("experiment_time", to_string(video_time));
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}
	
	// Read name from config
	try {
		name = read_config("name");
	} catch (const configNotFoundException& e){
		capture->warn(e.what());
		capture->info("Adding name as " + string(name));
		add_config("name",name);
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	// Save pid in config such that signals can easily be sent
	try {
		edit_config("pid",to_string(getpid())); 
	} catch (const configNotFoundException& e){
		add_config("pid",to_string(getpid()));
	} catch (const exception& e){
		capture->error(e.what());
		exit(1);
	}

	config_mutex.unlock();


	// Set to catch user signals
	if (signal(SIGUSR1,catch_SIGUSR) == SIG_ERR)
		capture->error("Error setting USRSIG1");

	if (signal(SIGUSR2,catch_SIGUSR) == SIG_ERR)
		capture->error("Error setting USRSIG1");

	// Convert time to seconds
	video_time *= 60;
	experiment_time *= 60;

	// Check video_time config file was last modified
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

	thread manager(ffmpeg_process_manager,video_time,mode,name, experiment_time);

	// Space here for extensions od functionality`

	manager.join();
	

	return 0;
}
