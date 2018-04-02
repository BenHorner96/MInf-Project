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

void create_config(int t){
	fstream cf;
	
	cf.open(CONFIG,fstream::out);

	if(cf){
		cf << "mode\t0" << endl;
		cf << "time\t" << t << endl;
		cf << "name\tNONE" << endl;
		cf << "total_t\t0" << endl;
		cf << "brightness\t0" << endl;
		cf << "contrast\t32" << endl;
		cf << "saturation\t64" << endl;
		cf << "hue\t0" << endl;
		cf << "white_balance_temperature_auto\t1" << endl;
		cf << "gamma\t0" << endl;
		cf << "gain\t0" << endl;
		cf << "power_line_frequency\t1" << endl;
		cf << "white_balance_temperature\t4600" << endl;
		cf << "sharpness\t3" << endl;
		cf << "backlight_compensation\t1" << endl;
		cf << "exposure_auto\t3" << endl;
		cf << "exposure_absolute\t156" << endl;
		cf << "exposure_auto_priority\t0" << endl;
	} else {
		throw configOpenException();
	}	

	cf.close();

	return;
}

void edit_config(ConfigPair setting){
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

void add_config(ConfigPair setting){
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

