/*
 * Configuration.h
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_
#include <arpa/inet.h>
#include "global_defs.h"
#include "media_source.h"
#include "TMM_StreamCtrl.h"
#include "multicast_group.h"
#include "sharedmemory.h"

class SockAddr_In
{
public:
	sockaddr_in addr;
	SockAddr_In();
	SockAddr_In(const SockAddr_In& rhs);
	SockAddr_In(const char* name);
	short port()const ;
	SockAddr_In& port(const short p) ;
	uint32_t host() const;
	SockAddr_In& host(uint32_t h);
	bool isValid();
	bool operator<(const SockAddr_In& rhs) const;
};




class Configuration
{
public:
	Configuration(MediaSource me_, SharedMemory<TMM_CntlBuffer>&  ctrl_buffer_);
	SockAddr_In GetAddress(MulticastGroup g);  				//multicast address to send datagrams to
	SockAddr_In GetAddress(); 									//used for the TMM
	SockAddr_In GetAddress(MediaSource s);							//Unicast address to filter datagrams with
	SockAddr_In GetMyExternalInterface();						//the address of the interface used to send datagrams to supplied address - this is what the outside woreld knows me as
	SockAddr_In GetMyTMM_Interface();							//the interface for talking to the TMM


	MediaSource me;													//	the address by which everyone else uses to identify me
	SharedMemory<TMM_CntlBuffer>&  ctrl_buffer;
	SockAddr_In GetInterfaceAddress(const SockAddr_In& addr );	//the address of the interface used to send datagrams to supplied address - so you can bind to a specific interface only

};


#endif /* CONFIGURATION_H_ */
