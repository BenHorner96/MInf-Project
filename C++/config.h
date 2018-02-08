#include <iostream>

#define CONFIG ".config"

using namespace std;

struct ConfigPair {
	string option;
	double value;
};

istream& operator>>(istream& is, ConfigPair& setting){
	is >> setting.option >> setting.value;
	return is;
}

ostream& operator<<(ostream& os, const ConfigPair& setting){
	os << setting.option << "\t" << setting.value << endl;
	return os;
}

void create_config(void);
void edit_config(ConfigPair setting);
void add_config(ConfigPair setting);
void rem_config(string option);
double read_config(string setting);

