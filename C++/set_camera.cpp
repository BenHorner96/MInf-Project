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

		for (auto& setting : cam_settings){
	
			if(setting.first != "exposure_absolute" && setting.first != "white_balance_temperature")
                		sStream << " -c " << setting.first << "=" << setting.second;

			if(cam_settings["exposure_auto"] == "0")
                	       sStream << " -c exposure_absolute=" << cam_settings["exposure_absolute"];
       
               		if(cam_settings["white_balance_temperature__auto"] == "0")
                		sStream << " -c white_balance_temperature=" << cam_settings["white_balance_temperature"];
		}

		string tmp = sStream.str();
		const char * cmd = tmp.c_str();
	
		system(cmd);
	} catch (const exception& e){
		cout << e.what() << endl;
		exit(1);
	} 

	exit(0);

}
