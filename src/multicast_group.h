/*
 * multicast_group.h
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#ifndef MULTICAST_GROUP_H_
#define MULTICAST_GROUP_H_

#include <cstdint>

class MulticastGroup
{
public:
    inline MulticastGroup():index(size()){}
    inline explicit MulticastGroup(uint8_t x):index(x<size()?x:size()){}			//initialise from integer - must be less than size
	explicit MulticastGroup(const char* name);			//initialise by name - slow but easy
	inline explicit operator uint8_t(){return index;}			//get the index back
	inline bool operator==(const MulticastGroup& rhs) const { return index==rhs.index; }
	inline bool operator!=(const MulticastGroup& rhs) const { return index!=rhs.index; }
	inline bool operator<(const MulticastGroup& rhs) const { return index<rhs.index; }
	const char* getName() const; //returns the name based on the index
	static uint8_t size();
private:
	const static char* names[];
	uint8_t index;
};





#endif /* MULTICAST_GROUP_H_ */
