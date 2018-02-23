#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>


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
		cf << "Time\t" << t << endl;
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
	
	if(!found) throw configNotFoundException();
	
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
		throw configNotFoundException();
	}
	
	return;

}

double read_config(string option){
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
			cf.close();
			return it->value;
		}

	cf.close();
	throw configEditException();
}

