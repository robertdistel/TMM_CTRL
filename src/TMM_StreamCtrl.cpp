/*
 * TMM_SteramCtrl.cpp
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#include "TMM_StreamCtrl.h"
#include <iostream>
#include <iomanip>
#include <regex>

bool TMM_CntlBuffer::checkAndClear(std::atomic<bool> v[256][256], MediaSourceMask sm, MulticastGroup g)
{
	bool result(false);
	for (uint8_t k =0; k< MediaSource::size(); k++)
	{
		if (sm.state(k))
			result=result || v[k][uint8_t(g)].exchange(false); //must clear all of them
	}
	return result;
}

void TMM_CntlBuffer::set(std::atomic<bool> v[256][256], MediaSourceMask sm, MulticastGroup g)
{
	for (uint8_t k =0; k< MediaSource::size(); k++)
	{
		if (sm.state(k))
			v[k][uint8_t(g)]=true; //must clear all of them
	}
}

void TMM_CntlBuffer::Dump(std::atomic<bool> v[256][256], const char* name)
{
	std::cout << name <<std::endl;
	const char* p;
	std::cout << " " << " "<< std::setw(10)<< " " << " ";
	for(uint8_t k = 0; k<MediaSource::size(); k++)
		std::cout << int(k%10);
	std::cout <<std::endl;
	for(uint8_t j = 0; j<MulticastGroup::size(); j++)
	{
		std::cout << int(j) << " "<< std::setw(10)<< MulticastGroup(j).getName() << " ";
		for(uint8_t k = 0; k<MediaSource::size(); k++)
		{
			if (v[k][j])
				p="*";
			else
				p="_";

			std::cout << p;
		}
		std::cout << std::endl;
	}
}

template <class T>
struct commands
{
	const char*								name;
	std::atomic<T>*							object;
	size_t									maxX;
	size_t 									maxY;
};

#define COMMAND(C)  #C, &C



template<class T>
void doCommand(std::atomic<T>* ptr,size_t x, size_t y,int val, commands<T>* c)
{
	if ((x>c->maxX) || (y>c->maxY))
		return;
	if (val!=-1)
		ptr[y*c->maxY+x]=(T)val;
	std::cout << "=" << (int)ptr[y*c->maxY+x] << std::endl;
}


void TMM_CntlBuffer::doCommandLine(const std::string& command_line)
{

	commands<bool> c1[] = {
			{ COMMAND(local_loop_back),1,1},
			{ COMMAND(remote_loop_back),1,1},
			{ COMMAND(domain_mask)[0],8,1},
			{ COMMAND(enabled)[0][0],256,256},
			{ COMMAND(active)[0][0],256,256}
	};

	commands<int8_t> c2[] = {
			{ COMMAND(TMM_MicPower),1,1},
			{ COMMAND(power)[0],256,1},
			{ COMMAND(TMM_MicPower),1,1},
			{ COMMAND(TMM_MicGain),1,1},
			{ COMMAND(TMM_HeadsetGain),1,1},
			{ COMMAND(gain)[0],256,1},
	};




	std::smatch sm;
	std::regex rex ("([\\w_]*)\\s*(?:\\[(\\d+)\\])?\\s*(?:\\[(\\d+)\\])?(?:\\s*=\\s*(\\d+))?");
	std::regex_match (command_line, sm, rex);
	std::string name(sm[1]);
	size_t indx1 = sm[2].length()==0?0:atoi(std::string(sm[2]).c_str());
	size_t indx2 = sm[3].length()==0?0:atoi(std::string(sm[3]).c_str());
	int val=sm[4].length()==0?-1:atoi(std::string(sm[4]).c_str());
	for (size_t k=0; k< sizeof(c1)/sizeof(c1[0]); k++)
	{
		if (name==std::string(c1[k].name))
		{
			std::atomic<bool>* p=(c1[k].object);
			doCommand(p,indx1,indx2, val, &c1[k]);
			return;
		}
	}
	for (size_t k=0; k< sizeof(c2)/sizeof(c2[0]); k++)
	{
		if (name==std::string(c2[k].name))
		{
			std::atomic<int8_t>* p=(c2[k].object);
			doCommand(p,indx1,indx2, val, &c2[k]);
			return;
		}
	}
	if (name=="dump")
	{
		Dump();
		return;
	}
	std::cout << "Commands:"<< std::endl;

	for (size_t k=0; k< sizeof(c1)/sizeof(c1[0]); k++)
	{
		std::cout << std::string(c1[k].name )<< std::endl;
	}

	for (size_t k=0; k< sizeof(c2)/sizeof(c2[0]); k++)
	{
		std::cout << std::string(c2[k].name ) << std::endl;
	}
	std::cout << "register [MEDIA SOURCE][MULTICAST GROUP]\n";

	std::cout << "Multicast Groups" << std::endl;

	for(size_t k = 0; k<MulticastGroup::size(); k++)
	{
		std::cout << k<< std::setw(10)<< MulticastGroup(k).getName() << std::endl;
	}
	std::cout << "Media Sources" << std::endl;

	for(size_t k = 0; k<MediaSource::size(); k++)
	{
		std::cout << k<< std::setw(10)<< MediaSource(k).getName() << std::endl;
	}



}


