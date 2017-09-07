/*
 * media_source.h
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#ifndef MEDIA_SOURCE_H_
#define MEDIA_SOURCE_H_

#include <cstdint>
#include <vector>
#include <string>
class MediaSource;

class MediaSourceMask
{
public:
    MediaSourceMask();
    MediaSourceMask(const MediaSource& src);


    MediaSourceMask operator&(const MediaSourceMask& rhs) const;
    MediaSourceMask operator|(const MediaSourceMask& rhs) const;
    MediaSourceMask operator~(void) const;
    bool operator==(const MediaSourceMask& rhs) const;
    inline bool operator!=(const MediaSourceMask& rhs) const {return !operator==(rhs);}
    static MediaSourceMask allOtherOperators(const MediaSource& src);
    static MediaSourceMask allOtherRadios(const MediaSource& src);
    bool state(uint8_t x) const {return mask[x];}


private:
    std::vector<bool> mask;
};

class MediaSource
{

public:

	inline MediaSource():index(size()){};				//initialise from integer - must be less than size
	inline explicit MediaSource(uint8_t x):index(x<size()?x:size()){};				//initialise from integer - must be less than size
	explicit MediaSource(const char* name);			//initialise by name - slow but easy
    inline explicit operator uint8_t() const {return index;}			//get the index back
	inline bool operator==(const MediaSource& rhs) const { return index==rhs.index; }
	inline bool operator!=(const MediaSource& rhs) const { return index!=rhs.index; }
	inline bool operator<(const MediaSource& rhs) const { return index<rhs.index; }
	const char* getName() const; //returns the name based on the index
    MediaSourceMask allOtherOperators(void) const {return MediaSourceMask::allOtherOperators(*this);}
    MediaSourceMask allOtherRadios(void) const {return MediaSourceMask::allOtherRadios(*this);}

    static uint8_t size();
private:
	friend MediaSourceMask;
	const static char* names[];
	uint8_t index;
};






#endif /* MEDIA_SOURCE_H_ */
