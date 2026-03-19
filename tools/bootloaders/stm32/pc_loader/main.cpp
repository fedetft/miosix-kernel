
/*
 * pc_loader: application to load a program in the external RAM of an
 * stm32f103ze through a serial port.
 * Developed by TFT: Terraneo Federico Technologies
 */

//Want to keep assert enabled also in release builds
#undef NDEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <boost/shared_array.hpp>
#include <boost/date_time.hpp>
#include "serialstream.h"

using namespace std;
using namespace boost;
using namespace boost::posix_time;

union IntChar
{
	unsigned int intVal;
	char charVal[4];
};

/**
 * Converts an unsigned int into 5 characters with bit #7 set.
 * \param in unsigned int to convert
 * \param out array of 5 bytes where the conversion will be written
 */
void encoder_5x4(unsigned int in, unsigned char out[5])
{
	for(unsigned int i=0;i<5;i++)
	{
		out[i]=static_cast<unsigned char>(in>>(i*7));
		out[i] |= 0x80;
	}
}

int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		cerr<<"Usage pc_loader serial_port file.bin"<<endl;
		return 1;
	}

	cout<<"Loading file... "<<flush;
	ifstream file(argv[2],std::ios::binary);
	if(!file)
	{
		cerr<<"File not found."<<endl;
		return 1;
	}
	file.seekg(0,ios::end);
	int size=file.tellg();
	if((size % 4) !=0)
	{
		size+=4;
		size &= ~3;
		assert((size % 4)==0);
	}
	file.seekg(0,ios::beg);
	shared_array<char> buffer(new char[size]);
	memset(buffer.get(),0,size);
	file.read(buffer.get(),size);
	cout<<"Done (size "<<size/1024<<"KB)."<<endl;

	cout<<"Calculating CRC... "<<flush;
	//This is the same CRC as the stm32 hardware CRC unit.
	crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0x0, false, false> crc;
	for(int i=0;i<size;i+=4)
	{
		char data[4];
		copy(&buffer[i],&buffer[i+4],data);
		//Do swapping because the .bin file is little endian
		swap(data[0],data[3]);
		swap(data[1],data[2]);
		crc.process_bytes(data,4);
	}
	unsigned int checksum=crc.checksum();
	cout<<" Done (0x"<<hex<<checksum<<")."<<endl;

	cout<<"Opening serial port... "<<flush;
	SerialOptions options(argv[1],115200,seconds(2));
	SerialStream serial(options);
	serial.exceptions(ios::badbit | ios::failbit);
	cout<<"Done."<<endl;

	cout<<"Checking bootloader version..."<<endl;
	serial<<"D"<<flush;//Reset
	serial<<"@"<<flush;//Status read
	for(int i=0;i<20;i++) //Maximum 20 description lines
	{
		string line;
		getline(serial,line);
		if(line.size()<=1) break;
		cout<<"  "<<line<<endl;
		switch(i)
		{
			case 0:
				if(line.substr(0,6)!="Loader")
				{
					cerr<<"Bootloader error"<<endl;
					return 1;
				}
				break;
			case 1:
				if(line.substr(0,19)!="Device: stm32f103ze" &&
				   line.substr(0,19)!="Device: stm32f207zg" &&
				   line.substr(0,19)!="Device: stm32f207ig")
				{
					cerr<<"Bootloader error"<<endl;
					return 1;
				}
				break;
		}
	}

	cout<<"Sending data... "<<flush;
	ptime t_start(microsec_clock::local_time());
	serial<<"A"<<flush;//Start sending data
	for(int i=0;i<size;i+=4)
	{
		IntChar temp;
		copy(&buffer[i],&buffer[i+4],temp.charVal);
		unsigned char bytes[5];
		encoder_5x4(temp.intVal,bytes);
		serial.write(reinterpret_cast<char *>(bytes),5);
	}
	ptime t_end(microsec_clock::local_time());
	cout<<"Done (Time "<<t_end-t_start<<
			 ", Speed "<<(static_cast<double>(size)/1.024)/
			 (t_end-t_start).total_milliseconds()<<"KB/s)."<<endl;

	cout<<"Checking CRC... "<<flush;
	serial<<"C"<<flush;//CRC check
	unsigned int received_checksum;
	serial>>hex>>received_checksum;
	if(checksum!=received_checksum)
	{
		cout<<"Failed (Received CRC 0x"<<hex<<received_checksum<<")."<<endl;
	} else {
		cout<<"CRC ok."<<endl<<"Running program."<<endl;
		serial<<"B"<<flush;//Run
	}
}
