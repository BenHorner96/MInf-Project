#include "config.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>

using namespace std;


void create_config(int t){
	fstream cf;
	
	cf.open(CONFIG,fstream::out);

	if(cf){
		cf << "Time\t" << t << endl;
	} else {
		throw 1;
	}	

	cf.close();

	return;
}

void edit_config(ConfigPair setting){
	vector<ConfigPair> v;
	int found = 0;	
	
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf)	throw 1;

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	//try {
		cf.open(".temp",fstream::out);
	
		if(!cf) throw 2;
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
	
	if(!found) throw 3;
	
	return;
}

void add_config(ConfigPair setting){
	fstream cf;
	
	cf.open(CONFIG,fstream::app);

	if(!cf)	throw 1;

	cf << setting.option << "\t" << setting.value << endl;

	return;
}

void rem_config(string option){
	vector<ConfigPair> v;
	int found = 0;	
	
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf)	throw 1;

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	//try {
		cf.open(".temp",fstream::out);
	
		if(!cf) throw 2;
		for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it){
			if(it->option == option){
				found = 1;
				continue;
			}
			cf << it->option << "\t" << it->value << endl;
		}
		cf.close();

		remove(CONFIG);
		rename(".temp",CONFIG);
	//} catch (){return -3;}
	
	if(!found) throw 3;
	
	return;

}

double read_config(string option){
	vector<ConfigPair> v;
		
	fstream cf;
	
	cf.open(CONFIG,fstream::in);

	if(!cf) throw 1;

	copy(istream_iterator<ConfigPair>(cf),
		istream_iterator<ConfigPair>(),
		back_inserter(v));

	cf.close();

	for(vector<ConfigPair>::iterator it = v.begin(); it != v.end(); ++it)
		if(it->option == setting->option){
			setting->value = it->value;
			cf.close();
			return 0;
		}

	cf.close();
	throw 3;
}

