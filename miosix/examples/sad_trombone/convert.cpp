/*
 * How to convert a .wav sound file into a .h file encoded in ADPCM on Linux
 * =========================================================================
 * 1) Open file in Audacity, make sure it's mono and 44100Hz sample rate.
 *    If it's not mono, use 'Tracks > Stereo Track to Mono' to convert it to
 *    mono. If the sample rate is not 44100Hz use 'Tracks > Resample' to
 *    convert it to 44100Hz
 * 2) From audacity choose 'File > Export', and select the output file
 *    format as 'Other uncompressed files'. Click 'Options' and select
 *    'RAW (header-less)' and 'Signed 16 bit PCM', save.
 *    This will produce a .raw file.
 * 3) Compile this program: 'g++ -O2 -o convert convert.cpp adpcm.c'
 * 4) Run this program './convert <name of generated raw file>'
 * 5) (Optional) Validate the generated file by opening Scilab and executing
 *    this code 'a=fscanfMat('validation.txt'); plot(a);'. It should look
 *    like in Audacity
 * 6) remove temporary files: 'rm *.raw *.bin Validation.txt convert'
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include "adpcm.h"

using namespace std;

void run(const string& s)
{
	cout<<"Running command "<<s<<"..."<<endl;
	system(s.c_str());
}

int main(int argc, char *argv[])
{
	string filename=argv[1];
	ifstream in(filename.c_str(),ios::binary);
	vector<short> data;
	for(;;)
	{
		short x;
		in.read(reinterpret_cast<char*>(&x),2);
		if(!in) break;
		data.push_back(x);
	}

	vector<unsigned char> encoded;
	int nsample=data.size()%2==0?data.size():(data.size()-1);
	for(int i=0;i<nsample;i+=2)
	{
		unsigned char in=ADPCM_Encode(data.at(i)) & 0xf;
		in|=ADPCM_Encode(data.at(i+1))<<4;
		encoded.push_back(in);
	}
	string outname=filename.substr(0,filename.find_last_of('.'))+".bin";
	ofstream out(outname.c_str(),ios::binary);
	out.write(reinterpret_cast<char*>(&encoded[0]),encoded.size());

	ofstream validation("Validation.txt");
	for(int i=0;i<encoded.size();i++)
	{
		unsigned char in=encoded.at(i);
		validation<<ADPCM_Decode(in & 0xf)<<' '<<endl;
		validation<<ADPCM_Decode(in>>4)<<' '<<endl;
	}

	string headername=filename.substr(0,filename.find_last_of('.'))+".h";
	run(string("xxd -i ")+outname+" "+headername);
	run(string("sed -i 's/unsigned/const unsigned/g' ")+headername);
}
