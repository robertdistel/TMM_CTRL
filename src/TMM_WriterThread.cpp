/*
 * TMM_WriterThread.cpp
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

/*
 * TMM_reader_thread.cpp
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#include "TMM_WriterThread.h"
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
#include <map>
#include <iostream>



bool TMM_WriterThread::halt_threads = false;


class TMM_WriterThread::Context
{
public:
	Context () :config(nullptr){}

	Context (Configuration* config_,  MulticastGroup mg_) :
		ctx (),config(config_), mg(mg_)
	{
	}


	std::thread ctx;
	Configuration* config;
	MulticastGroup mg;
	static void do_thread (std::shared_ptr<Context> pctx);
};



void TMM_WriterThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	uint8_t buffer[2048]; 	//allocate a 2K buffer
	ssize_t  buffer_used(0);	//how much of the buffer is actually in use


	int TMM_sock(0);
	int external_sock(0);

	do
	{
		//first step - create socket to read TMM
		SockAddr_In tmm_local_addr = 	pctx->config->GetMyTMM_Interface().port(0); //this is the local interface to use - so we dont have to listen on in addr_any-allow any port as we are writing form this port only
		SockAddr_In tmm_remote_addr = 	pctx->config->GetAddress();  		//this is the address of the TMM;

		if ((TMM_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			break;

		if (bind (TMM_sock, (struct sockaddr *) &tmm_local_addr.addr, sizeof(tmm_local_addr.addr)) < 0)
			break; //bind the interface

		if (connect (TMM_sock, (struct sockaddr *) &tmm_remote_addr.addr, sizeof(tmm_remote_addr.addr)) < 0)
			break;//connect the interface - we now only listen to data from the right address


		SockAddr_In external_local_addr = 	pctx->config->GetMyExternalInterface();  		//this is my address to the outside world

		//initialise the look up table
		std::map<SockAddr_In,MediaSource> source_lookup_table;
		for (uint8_t s=0; s< MediaSource::size(); s++)
			if (MediaSource(s) !=pctx->config->me)
				source_lookup_table[pctx->config->GetAddress(MediaSource(s))]=MediaSource(s); //we dont send back to ourselves

		if ((external_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			break;			//create socket
		int reuse = 1;																		//permit the same  local address to be reused - you need this as the filtering of mc groups takes place at a higher layer
		//the OS sees this as multiple sockets bound to the same local address and port number....
		if(setsockopt(external_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
			break;

		//now bind the socket to a multicast group - step one bind (os level)

		if (bind (external_sock, (struct sockaddr *) &external_local_addr.addr, sizeof(external_local_addr.addr)) < 0)
			break;//bind the interface

		//and register it to the multicast group - causes IGMP monkey business to take place
		struct ip_mreq group;
		group.imr_multiaddr = pctx->config->GetAddress(pctx->mg).addr.sin_addr;
		group.imr_interface = external_local_addr.addr.sin_addr;
		if(setsockopt(external_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
			break;


		//now spin around reading from the recv socket and forward if the source was one we care about
		do

		{
			SockAddr_In source_address;
			socklen_t socklen(sizeof(source_address.addr));

			buffer_used=recvfrom(external_sock,buffer,sizeof(buffer),0,(struct sockaddr*)(&source_address.addr),&socklen);
//			pctx->config->ctrl_buffer->Dump();
			if (buffer_used >0)
			{
				auto k=source_lookup_table.find(source_address);
				if (k!=source_lookup_table.end()) //the address is one we want to process
				{
//					std::cout << "Recv from mcg " << pctx->mg.getName() << " source " << k->second.getName() << std::endl;
					pctx->config->ctrl_buffer->setActive(k->second,pctx->mg);		//flag it as active (even if we dont forward it
					if (pctx->config->ctrl_buffer->isEnabled(k->second,pctx->mg)) //its enabled so it needs to be forwarded as well
					{
						send(TMM_sock,buffer,static_cast<size_t>(buffer_used),0);
					}
				}
			}
		}
		while (!TMM_WriterThread::halt_threads);
	} while(false);
	close(TMM_sock);
	close(external_sock);
}



TMM_WriterThread::TMM_WriterThread () :
							  pcontext (new Context)
{
}


TMM_WriterThread::TMM_WriterThread (Configuration* config_, const MulticastGroup& mg_) :
							  pcontext (new Context (config_,mg_))
{
}


void TMM_WriterThread::start_thread ()
{
	pcontext->ctx = std::thread (Context::do_thread, pcontext);
}


void TMM_WriterThread::join (void)
{
	return pcontext->ctx.join ();
}






