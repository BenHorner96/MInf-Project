#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <map>
#include <sstream>

#include "config.h"

using namespace std;

int main(){
	try {
		map<string,string> cam_settings = camera_config();	

		ostringstream sStream;

		sStream << "v4l2-ctl";

		for (auto& setting : cam_settings)
			sStream << " -c " << setting.first << "=" << setting.second;
	
		string tmp = sStream.str();
		const char * cmd = tmp.c_str();
	
		system(cmd);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	} 

	exit(0);

}
