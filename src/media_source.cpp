/*
 * media_source.cpp
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#include "media_source.h"



const char* MediaSource::names[] =
{
    "RRadio 1",
    "RRadio 2",
    "OOperator 1",
    "OOperator 2"
};

const char* MediaSource::getName() const
{
	return &names[index][1]; //skip the first char as it is the type indicator
}

uint8_t MediaSource::size()
{
	return sizeof(names)/sizeof(names[0]);
}

MediaSource::MediaSource(const char* name)
{
	for (index=0; index<size(); index++ )
	{
		if(std::string(&names[index][1]) == std::string(name)) //clip off first letter - its the type indicator
			break;
	}
}

MediaSourceMask::MediaSourceMask():mask(MediaSource::size(), false)
{
}

MediaSourceMask::MediaSourceMask(const MediaSource& src):mask(MediaSource::size(), false)
{
    uint8_t x=uint8_t(src);
    mask[x]=true;
}

MediaSourceMask MediaSourceMask::operator&(const MediaSourceMask& rhs) const
{
	MediaSourceMask newMask;
	for (auto i=0; i<MediaSource::size(); i++)
		newMask.mask[i] = mask[i] && rhs.mask[i];

	return newMask;
}

MediaSourceMask MediaSourceMask::operator|(const MediaSourceMask& rhs) const
{
	MediaSourceMask newMask;
	for (auto i=0; i<MediaSource::size(); i++)
		newMask.mask[i] = mask[i] || rhs.mask[i];

	return newMask;
}

MediaSourceMask MediaSourceMask::operator~(void) const
{
	MediaSourceMask newMask;
	for (auto i=0; i<MediaSource::size(); i++)
		newMask.mask[i] = ~mask[i];

	return newMask;
}

bool MediaSourceMask::operator==(const MediaSourceMask& rhs) const
{
	bool result(true);
	for (auto i=0; i<MediaSource::size() && result; i++)
		result = result && (mask[i] == rhs.mask[i]);

	return result;
}

MediaSourceMask MediaSourceMask::allOtherOperators(const MediaSource& src)
{
	static MediaSourceMask allOperators;
	static bool initialised(false);
	if(!initialised)
	{
		initialised=true;	//small race if called on multiple threads
		for (uint8_t k=0; k<MediaSource::size(); k++)
		{
			if (MediaSource::names[k][0]=='O' || MediaSource::names[k][0]=='o' )
				allOperators.mask[k]=true;
		}
	}
    return allOperators & ~MediaSourceMask(src);
}

MediaSourceMask MediaSourceMask::allOtherRadios(const MediaSource& src)
{
	static MediaSourceMask allRadios;
	static bool initialised(false);
	if(!initialised)
	{
		initialised=true;	//small race if called on multiple threads
		for (uint8_t k=0; k<MediaSource::size(); k++)
		{
			if (MediaSource::names[k][0]=='R' || MediaSource::names[k][0]=='r' )
				allRadios.mask[k]=true;
		}
	}
    return allRadios & ~MediaSourceMask(src);
}
