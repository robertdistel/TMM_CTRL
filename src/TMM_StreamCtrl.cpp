/*
 * TMM_SteramCtrl.cpp
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#include "TMM_StreamCtrl.h"
#include <iostream>
#include <iomanip>

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
	for(uint8_t j = 0; j<MulticastGroup::size(); j++)
	{
		std::cout << std::setw(10)<< MulticastGroup(j).getName() << " ";
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

