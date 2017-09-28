/*
 * TMM_reader_thread.cpp
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#include "TMM_ReaderThread.h"
#include "Configuration.h"
#include "global_defs.h"
#include "multicast_group.h"
#include "media_source.h"
#include "sharedmemory.h"

#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

#include <iostream>
#include "TMM_Frame.h"
#include <assert.h>



bool TMM_ReaderThread::halt_threads = false;


class TMM_ReaderThread::Context
{
public:
	Context () :config(nullptr)
{
}

	Context (Configuration* config_) :
		ctx (),config(config_)
	{
	}


	std::thread ctx;
	Configuration* config;
	static void do_thread (std::shared_ptr<Context> pctx);
};

#define CHECK(FUNC,...) if(FUNC( __VA_ARGS__ )<0) 																					\
{												 																					\
	std::cerr << "Socket Operation " << #FUNC << " failed in " << __PRETTY_FUNCTION__ << "(" << __FILE__ << ":" << __LINE__ <<")"; 	\
	perror(0); 																														\
	break; 																															\
}



void TMM_ReaderThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	TMM_Frame frame; 	//allocate a 2K buffer
	ssize_t  buffer_used(0);	//how much of the buffer is actually in use


	int TMM_sock(0);
	int external_sock(0);
	int TMM_loopback_sock(0);

	do
	{
		//first step - create socket to read TMM
		SockAddr_In tmm_local_addr = 	pctx->config->GetMyTMM_Interface(); 		//this is the local interface to use - so we dont have to listen on in addr_any - specify the port we need to listen on;

		if ((TMM_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) break;

		int reuse = 1;																		//permit the same  local address to be reused - you need this as the filtering of mc groups takes place at a higher layer
		//the OS sees this as multiple sockets bound to the same local address and port number....
		CHECK (setsockopt, TMM_sock, SOL_SOCKET, SO_REUSEADDR, (int *)&reuse, sizeof(reuse));


		CHECK (bind , TMM_sock, (struct sockaddr *) &tmm_local_addr.addr, sizeof(tmm_local_addr.addr)) //bind the interface

		//we may not need to do this -as we only recv on this socket - it doesnt need to be in the connected state
		//if (connect (TMM_sock, (struct sockaddr *) &tmm_remote_addr.addr, sizeof(tmm_remote_addr.addr)) < 0) break;//connect the interface - we now only listen to data from the right address - on the correct interface

		//create a loopback socket - incase we need it
		SockAddr_In tmm_local_loopback_addr = 	pctx->config->GetMyTMM_Interface().port(0); //this is the local interface to use - any port - we are sending to the TMM
		SockAddr_In tmm_remote_loopback_addr = 	pctx->config->GetAddress();  				//this is the address of the TMM - with the correct port;

		if ((TMM_loopback_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) break;

		CHECK (bind ,TMM_loopback_sock, (struct sockaddr *) &tmm_local_loopback_addr.addr, sizeof(tmm_local_loopback_addr.addr)); //bind the interface

		CHECK (connect , TMM_loopback_sock, (struct sockaddr *) &tmm_remote_loopback_addr.addr, sizeof(tmm_remote_loopback_addr.addr));//connect the interface - we now only listen to data from the right address

		SockAddr_In external_local_addr = 	pctx->config->GetMyExternalInterface();  		//this is my address to the outside world

		if ((external_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) break;

		//set up multicast if differently - effectively this binds the interface so that packets sent originate from here
		CHECK (setsockopt , external_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&external_local_addr.addr, sizeof(external_local_addr.addr)) ;
		char loopch = 0;
		//and disable loopback - we don't want to see our own packets
		CHECK (setsockopt , external_sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch));


		std::vector< SockAddr_In > SendToAddressCache(MulticastGroup::size());
		for (uint32_t g=0; g< MulticastGroup::size(); g++)
			SendToAddressCache[g]=pctx->config->GetAddress(MulticastGroup(g));

		//now spin around reading from the recv socket and sending to valid, enabled sendto adresses on the send sock
		do
		{
			buffer_used=recv(TMM_sock,frame.frame(),TMM_Frame::maxFrameSize,0);
			assert(buffer_used == frame.packet_sz());
			pctx->config->ctrl_buffer->setTMM_MicPower(frame.pwr()+frame.gain()+pctx->config->ctrl_buffer->getTMM_MicGain()); //store the mic power - so the ctrl agent can see the mic levels
			frame.gain(frame.gain()+pctx->config->ctrl_buffer->getTMM_MicGain()); //and aply the mic gain setting
			//local loopback mirror function - send back packets from the TMM back to the TMM
			if (pctx->config->ctrl_buffer->remote_loop_back ) //loop it back if the domain mask matches and we are looping back
			{
				if (pctx->config->ctrl_buffer->domain_mask[frame.domain_ID()])
					send(TMM_loopback_sock,frame.frame(),frame.packet_sz(),0); //mirror it out as a remote loopback
			}
			else
			{
				//			pctx->config->ctrl_buffer->Dump();
				if (buffer_used >0)
				{
					//we have recevied a packet - now forward it to all active destinations
					for (uint8_t g=0; g< MulticastGroup::size(); g++)
					{
						if (SendToAddressCache[g].isValid() && pctx->config->ctrl_buffer->isEnabled(pctx->config->me,MulticastGroup(g))) //we know how to route it and we want to
						{
							//						std::cout << "Send to mcg " << MulticastGroup(g).getName() << " source " << pctx->config->me.getName() << std::endl;
							sendto(external_sock,frame.frame(),frame.packet_sz(),0, (struct sockaddr*)(&SendToAddressCache[g].addr),sizeof(SendToAddressCache[g].addr));
						}
					}
				}
			}


		}
		while (!TMM_ReaderThread::halt_threads);
	} while(false);
	close(TMM_sock);
	close(TMM_loopback_sock);
	close(external_sock);
}



TMM_ReaderThread::TMM_ReaderThread () :
											  pcontext (new Context)
{
}


TMM_ReaderThread::TMM_ReaderThread (Configuration* config_) :
											  pcontext (new Context (config_))
{
}


void TMM_ReaderThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}


void TMM_ReaderThread::join (void)
{
	return pcontext->ctx.join ();
}



