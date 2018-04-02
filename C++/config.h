#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <map>

#define CONFIG ".config"
#define DEFAULT_time 15
#define DEFAULT_mode 0
#define DEFAULT_name "NONE"


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



void create_config(int t);
void edit_config(ConfigPair setting);
void add_config(ConfigPair setting);
void rem_config(std::string option);
std::string read_config(std::string setting);
std::map<std::string,std::string> camera_config();

#endif
