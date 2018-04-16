#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>
#include <map>


#include "config.h"

using namespace std;

std::istream& operator>>(std::istream& is, ConfigPair& setting){
	is >> setting.option >> setting.value;
	return is;
}

std::ostream& operator<<(std::ostream& os, const ConfigPair& setting){
	os << setting.option << "\t" << setting.value << std::endl;
	return os;
}

void create_config(){
	fstream cf;
	
	cf.open(CONFIG,fstream::out);

	if(cf){
		cf << "mode\t" 				 << DEFAULT_mode << endl;
		cf << "video_time\t" 			 << DEFAULT_video_time << endl;
		cf << "name\t" 				 << DEFAULT_name << endl;
		cf << "experiment_time\t" 		 << DEFAULT_experiment_time<< endl;
		cf << "brightness\t" 			 << DEFAULT_brightness<< endl;
		cf << "contrast\t" 			 << DEFAULT_contrast<< endl;
		cf << "saturation\t" 			 << DEFAULT_saturation<< endl;
		cf << "hue\t" 				 << DEFAULT_hue<< endl;
		cf << "white_balance_temperature_auto\t" << DEFAULT_white_balance_temperature_auto<< endl;
		cf << "gamma\t" 			 << DEFAULT_gamma<< endl;
		cf << "gain\t" 				 << DEFAULT_gain<< endl;
		cf << "power_line_frequency\t" 		 << DEFAULT_power_line_frequency<< endl;
		cf << "white_balance_temperature\t"	 << DEFAULT_white_balance_temperature<< endl;
		cf << "sharpness\t" 			 << DEFAULT_sharpness<< endl;
		cf << "backlight_compensation\t" 	 << DEFAULT_backlight_compensation<< endl;
		cf << "exposure_auto\t" 		 << DEFAULT_exposure_auto<< endl;
		cf << "exposure_absolute\t" 		 << DEFAULT_exposure_absolute<< endl;
		cf << "exposure_auto_priority\t" 	 << DEFAULT_exposure_auto_priority<< endl;
	} else {
		throw configOpenException();
	}	

	cf.close();

	return;
}

void edit_config(string option, string value){

	ConfigPair setting;
	setting.option = option; setting.value = value;
	vector<ConfigPair> v;
	int found = 0;	
	
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf)	throw configOpenException();

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	//try {
		cf.open(".temp",fstream::out);
	
		if(!cf) throw configEditException();
		for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it){
			if(it->option == setting.option){
				it->value = setting.value;
				found = 1;
			}
			cf << it->option << "\t" << it->value << endl;
		}
		cf.close();

		remove(CONFIG);
		rename(".temp",CONFIG);
	//} catch (){return -3;}
	
	if(!found) throw configNotFoundException(setting.option);
	
	return;
}

void add_config(string option, string value){
	ConfigPair setting;
	setting.option = option; setting.value = value;

	fstream cf;
	
	cf.open(CONFIG,fstream::app);

	if(!cf)	throw configOpenException();

	cf << setting.option << "\t" << setting.value << endl;

	return;
}

void rem_config(string option){
	vector<ConfigPair> v;
	int found = 0;	
	
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf)	throw configOpenException();

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	cf.open(".temp",fstream::out);

	if(!cf) throw configEditException();
	for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it){
		if(it->option == option){
			found = 1;
			continue;
		}
		cf << it->option << "\t" << it->value << endl;
	}
	cf.close();

	if(found){
		remove(CONFIG);
		rename(".temp",CONFIG);
	} else {
		remove(".temp");
		throw configNotFoundException(option);
	}
	
	return;

}

string read_config(string option){
	vector<ConfigPair> v;
		
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf) throw configOpenException();

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it)
		if(it->option == option){
			return it->value;
		}

	throw configEditException();
}

map<string,string> camera_config(){
	vector<ConfigPair> v;

	map<string,string> cam_settings;

		
	cam_settings["brightness"] = "none";
	cam_settings["contrast"] = "none";
	cam_settings["saturation"] = "none";
	cam_settings["hue"] = "none";
	cam_settings["white_balance_temperature_auto"] = "none";
	cam_settings["gamma"] = "none";
	cam_settings["gain"] = "none";
	cam_settings["power_line_frequency"] = "none";
	cam_settings["white_balance_temperature"] = "none";
	cam_settings["sharpness"] = "none";
	cam_settings["backlight_compensation"] = "none";
	cam_settings["exposure_auto"] = "none";
	cam_settings["exposure_absolute"] = "none";
	cam_settings["exposure_auto_priority"] = "none";
		
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf) throw configOpenException();

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	map<string,string>::iterator check;

	for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it)
		if((check = cam_settings.find(it->option)) != cam_settings.end()){
			check->second = it->value;
		}

	for(auto& setting : cam_settings)
		if(setting.second == "none")
			throw configNotFoundException("");
	

	return cam_settings;
}

