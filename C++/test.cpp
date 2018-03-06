#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <mutex> 
#include <csignal>
#include <string>
#include <sstream>
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

using namespace std;

int main(){
	int t = 20, status = 0;
	pid_t pid = 0;
	string ffmpeg_cmd[] = {"/usr/bin/ffmpeg","-i","/dev/video1","-s","1280x720",
		"-vcodec","copy","-t",to_string(t),"sass.flv",
		"-t",to_string(t),"-vf", "fps=1/5",
		"-update","1","frame.jpg"};
	vector<char*> cmd_v;

	// add null terminators
	transform(begin(ffmpeg_cmd),end(ffmpeg_cmd),
		back_inserter(cmd_v),
		[](string& s){ s.push_back(0); return &s[0];});
	cmd_v.push_back(nullptr);

	char** c_cmd = cmd_v.data();

	if (!(pid=fork())) {
		//cout << "shild" << endl;
		//int dev_null = open("/dev/null",O_WRONLY);
		//if(dev_null < 0){
		//	perror("Error opening dev/null");
		//	exit(1);
		//}
			
		//if(dup2(dev_null,STDOUT_FILENO) < 0){
		//	perror("Error in dup2");
		//	exit(1);
		//}

		//close(dev_null);

		if(execv("/usr/bin/ffmpeg",c_cmd) < 0){
			perror("Error execv");
			exit(1);
		}
		exit(0);
	} else {
		wait(&status);

		cout << status << endl;

		if(WIFEXITED(status))
			cout << WEXITSTATUS(status) << endl;
	}

}
