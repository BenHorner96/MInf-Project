#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <map>

#define DEFAULT_video_time 15
#define DEFAULT_mode 0
#define DEFAULT_name "NONE"
#define DEFAULT_experiment_time 15
#define DEFAULT_brightness 0 
#define DEFAULT_contrast 32
#define DEFAULT_saturation 64
#define DEFAULT_hue 0
#define DEFAULT_white_balance_temperature_auto 1
#define DEFAULT_gamma 0
#define DEFAULT_gain 0
#define DEFAULT_power_line_frequency 1
#define DEFAULT_white_balance_temperature 4600
#define DEFAULT_sharpness 3
#define DEFAULT_backlight_compensation 1
#define DEFAULT_exposure_auto 3
#define DEFAULT_exposure_absolute 156
#define DEFAULT_exposure_auto_priority 0

#define CONFIG ".config"

class configOpenException : public std::exception {
	public :
		virtual const char* what() const throw() {
			return "Could not open config file";
		}
}; 

class configEditException : public std::exception {
	public:
		virtual const char* what() const throw() {
			return "Could not open temporary file for editing config file";
		}
}; 

class configNotFoundException : public std::exception {
		std::string msg;
	public:
		configNotFoundException(const std::string& option)
			: msg(std::string("Could not find option "+option+" in config file"))
		{}

		virtual const char* what() const throw() {
			return msg.c_str();
		}
}; 

struct ConfigPair {
	std::string option;
	std::string value;
};



void create_config();
void edit_config(std::string option, std::string value);
void add_config(std::string option, std::string value);
void rem_config(std::string option);
std::string read_config(std::string setting);
std::map<std::string,std::string> camera_config();

#endif
