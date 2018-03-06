#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <map>

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
	public:
		virtual const char* what() const throw() {
			return "Could not find option in config file";
		}
}; 

struct ConfigPair {
	std::string option;
	double value;
};



void create_config(int t);
void edit_config(ConfigPair setting);
void add_config(ConfigPair setting);
void rem_config(std::string option);
double read_config(std::string setting);
std::map<std::string,double> camera_config();

#endif
