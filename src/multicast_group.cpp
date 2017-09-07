/*
 * multicast_group.cpp
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */


#include "multicast_group.h"
#include <string>



const char* MulticastGroup::names[] =
{
    "Radio 1",
    "Radio 2",
    "Shoutdown"
};

const char* MulticastGroup::getName() const
{
	return names[index];
}

uint8_t MulticastGroup::size()
{
	return sizeof(names)/sizeof(names[0]);
}

MulticastGroup::MulticastGroup(const char* name)
{
	for (index=0; index<size(); index++ )
	{
		if(std::string(names[index]) == std::string(name))
			break;
	}
}


