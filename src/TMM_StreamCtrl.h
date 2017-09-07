/*
 * TMM_StreamCtrl.h
 *
 *  Created on: 7 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_STREAMCTRL_H_
#define TMM_STREAMCTRL_H_
#include <atomic>
#include "media_source.h"
#include "multicast_group.h"
#if (ATOMIC_BOOL_LOCK_FREE==2)  //bool supports all operations without a lock


class TMM_CntlBuffer
{
public:

	inline bool checkActiveAndClear(MediaSourceMask sm, MulticastGroup g){ return checkAndClear(active,sm,g); }	//check if any are active
    inline bool checkActiveAndClear(MediaSource s, MulticastGroup g){return active[uint8_t(s)][uint8_t(g)].exchange(false);}
    inline bool isActive(MediaSource s, MulticastGroup g){return active[uint8_t(s)][uint8_t(g)];}
    inline void setActive(MediaSourceMask sm, MulticastGroup g){set(active,sm,g);}
    inline void setActive(MediaSource s, MulticastGroup g){active[uint8_t(s)][uint8_t(g)]=true;}
    inline void clearActive(MediaSource s, MulticastGroup g){active[uint8_t(s)][uint8_t(g)]=false;}
    inline bool checkEnabledAndClear(MediaSourceMask sm, MulticastGroup g) { return checkAndClear(enabled,sm,g); }
    inline bool checkEnabledAndClear(MediaSource s, MulticastGroup g){return enabled[uint8_t(s)][uint8_t(g)].exchange(false);}
    inline bool isEnabled(MediaSource s, MulticastGroup g){return enabled[uint8_t(s)][uint8_t(g)];}
    inline void setEnabled(MediaSourceMask sm, MulticastGroup g){set(enabled,sm,g);}
    inline void setEnabled(MediaSource s, MulticastGroup g){enabled[uint8_t(s)][uint8_t(g)]=true;}
    inline void clearEnabled(MediaSource s, MulticastGroup g){enabled[uint8_t(s)][uint8_t(g)]=false;}


	void Dump(void) { Dump(enabled,"Enabled"); Dump(active,"Active"); }



private:
    std::atomic<bool>                   enabled[256][256];
    std::atomic<bool>                   active[256][256];
    std::atomic<bool>                   domain_mask[8];        //mask saying which domains are permitted to pass
    void set(std::atomic<bool> v[256][256], MediaSourceMask sm, MulticastGroup g);
    bool checkAndClear(std::atomic<bool> v[256][256], MediaSourceMask sm, MulticastGroup g);
    void Dump(std::atomic<bool> v[256][256], const char* name);

};

#else
	1=0 //we do not have a atomic operator
#endif
#endif /* TMM_STREAMCTRL_H_ */
