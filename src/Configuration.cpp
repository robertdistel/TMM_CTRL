/*
 * Configuration.cpp
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#include "global_defs.h"
#include "Configuration.h"
#include <netdb.h>          /* hostent struct, gethostbyname() */
#include <cstring>
#include <stdlib.h>
#include <unistd.h>    //close
#include <string>


const char * MulticastGroupFQDN[] = {
		"radio1_MC.local",
		"radio2_MC.local",
		"shoutdown_MC.local"
};

const char * EntityFQDN[] = {
		"radio1.local",
		"radio2.local",
		"operator1.local",
		"operator2.local"
};

const char  TMM_FQDN[] = {
		"raspi0.local"
};

const short port=12345;

bool SockAddr_In::isValid(){ return addr.sin_family !=0; }
bool SockAddr_In::operator<(const SockAddr_In& rhs) const {return addr.sin_port<rhs.addr.sin_port || (addr.sin_port==rhs.addr.sin_port && addr.sin_addr.s_addr<rhs.addr.sin_addr.s_addr);}


SockAddr_In::SockAddr_In() {memset(&addr,0,sizeof(sockaddr_in));}
SockAddr_In::SockAddr_In(const SockAddr_In& rhs){if (this!=&rhs) memcpy(&addr,&rhs.addr,sizeof(sockaddr_in));}
SockAddr_In::SockAddr_In(const char* name)
{
	memset(&addr,0,sizeof(sockaddr_in));
	struct hostent *host; /* host information */
	if ((host = gethostbyname (name)) == NULL)
		return;

	/* Construct the server sockaddr_in structure */
	addr.sin_family = AF_INET; /* Internet/IP */
	addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]); /* Incoming addr */
}
short SockAddr_In::port() const {return ntohs(addr.sin_port);}
SockAddr_In& SockAddr_In::port(const short p) {addr.sin_port=htons(p); return *this;}
uint32_t SockAddr_In::host() const {return ntohl(addr.sin_addr.s_addr);}
SockAddr_In& SockAddr_In::host(uint32_t h) {addr.sin_addr.s_addr = htonl(h); return *this;}

SockAddr_In Configuration::GetAddress(MulticastGroup g)
{
	return SockAddr_In(MulticastGroupFQDN[uint8_t(g)]).port(port);
}

SockAddr_In Configuration::GetAddress()
{
	return SockAddr_In(TMM_FQDN).port(port);
}

SockAddr_In Configuration::GetAddress(MediaSource s)
{


	return SockAddr_In(EntityFQDN[uint8_t(s)]).port(port);
}


SockAddr_In Configuration::GetMyExternalInterface()
{
	return GetAddress(me);
}

SockAddr_In Configuration::GetInterfaceAddress(const SockAddr_In& s)		//the address of the interface used to send datagrams to supplied address - so you can bind to a specific interface only
{
	SockAddr_In out;

	int sock = socket ( AF_INET, SOCK_DGRAM, 0);

	//Socket  created
	if(sock >= 0)
	{

		if(connect( sock , (const struct sockaddr*) &s.addr , sizeof(s.addr) ) ==0)
		{

			socklen_t namelen = sizeof(out.addr);
			getsockname(sock, (struct sockaddr*) &out.addr, &namelen); //should have Af family set if succesfull

		}

		close(sock);


	}
	return out.port(port);
}

SockAddr_In Configuration::GetMyTMM_Interface()								//used for the TMM  -often the TMM has a non-routable interface
{
	return Configuration::GetInterfaceAddress(Configuration::GetAddress()).port(port);
}

Configuration::Configuration(MediaSource me_, SharedMemory<TMM_CntlBuffer>&  ctrl_buffer_):me(me_), ctrl_buffer(ctrl_buffer_)
{
}









