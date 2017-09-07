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




void TMM_ReaderThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	uint8_t buffer[2048]; 	//allocate a 2K buffer
	ssize_t  buffer_used(0);	//how much of the buffer is actually in use


	int TMM_sock(0);
	int external_sock(0);

	do
	{
		//first step - create socket to read TMM
		SockAddr_In tmm_local_addr = 	pctx->config->GetMyTMM_Interface(); 		//this is the local interface to use - so we dont have to listen on in addr_any;
		SockAddr_In tmm_remote_addr = 	pctx->config->GetAddress().port(0);  		//this is the address of the TMM - we dont care what port the data comes from

		if ((TMM_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) break;

		if (bind (TMM_sock, (struct sockaddr *) &tmm_local_addr.addr, sizeof(tmm_local_addr.addr)) < 0) break; //bind the interface

		if (connect (TMM_sock, (struct sockaddr *) &tmm_remote_addr.addr, sizeof(tmm_remote_addr.addr)) < 0) break;//connect the interface - we now only listen to data from the right address


		SockAddr_In external_local_addr = 	pctx->config->GetMyExternalInterface();  		//this is my address to the outside world

		if ((external_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) break;

		//set up multicast if differently - effectively this binds the interface so that packets sent originate from here
		if(setsockopt(external_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&external_local_addr.addr, sizeof(external_local_addr.addr)) < 0) break;
		char loopch = 0;
		//and disable loopback - we don't want to see our own packets
		if(setsockopt(external_sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) break;


		std::vector< SockAddr_In > SendToAddressCache(MulticastGroup::size());
		for (uint32_t g=0; g< MulticastGroup::size(); g++)
			SendToAddressCache[g]=pctx->config->GetAddress(MulticastGroup(g));

		//now spin around reading from the recv socket and sending to valid, enabled sendto adresses on the send sock
		do
		{
			buffer_used=recv(TMM_sock,buffer,sizeof(buffer),0);
//			pctx->config->ctrl_buffer->Dump();
			if (buffer_used >0)
			{
				//we have recevied a packet - now forward it to all active destinations
				for (uint8_t g=0; g< MulticastGroup::size(); g++)
				{
					if (SendToAddressCache[g].isValid() && pctx->config->ctrl_buffer->isEnabled(pctx->config->me,MulticastGroup(g))) //we know how to route it and we want to
					{
//						std::cout << "Send to mcg " << MulticastGroup(g).getName() << " source " << pctx->config->me.getName() << std::endl;
						sendto(external_sock,buffer,static_cast<size_t>(buffer_used),0, (struct sockaddr*)(&SendToAddressCache[g].addr),sizeof(SendToAddressCache[g].addr));
					}
				}
			}


		}
		while (!TMM_ReaderThread::halt_threads);
	} while(false);
	close(TMM_sock);
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



