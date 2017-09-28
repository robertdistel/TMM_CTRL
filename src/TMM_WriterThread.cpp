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
#include "TMM_Frame.h"



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

#define CHECK(FUNC,...) if(FUNC( __VA_ARGS__ )<0) 																					\
{												 																					\
	std::cerr << "Socket Operation " << #FUNC << " failed in " << __PRETTY_FUNCTION__ << "(" << __FILE__ << ":" << __LINE__ <<")"; 	\
	perror(0); 																														\
	break; 																															\
}

void TMM_WriterThread::Context::do_thread (std::shared_ptr<Context> pctx)
{
	TMM_Frame frame; 	//allocate a frame
	ssize_t  buffer_used(0);	//how much of the buffer is actually in use


	int TMM_sock(0);
	int external_sock(0);
	int TMM_loopback_sock(0);


	do
	{
		//first step - create socket to read TMM
		SockAddr_In tmm_local_addr = 	pctx->config->GetMyTMM_Interface().port(0); //this is the local interface to use - so we dont have to listen on in addr_any-allow any port as we are writing form this port only
		SockAddr_In tmm_remote_addr = 	pctx->config->GetAddress();  		//this is the address of the TMM;

		if ((TMM_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			break;

		CHECK(bind , TMM_sock, (struct sockaddr *) &tmm_local_addr.addr, sizeof(tmm_local_addr.addr));

		CHECK(connect , TMM_sock, (struct sockaddr *) &tmm_remote_addr.addr, sizeof(tmm_remote_addr.addr));

		//create a loopback socket - incase we need it
		SockAddr_In tmm_remote_loopback_addr= 	pctx->config->GetMyTMM_Interface(); 	//this is the local interface to use - with the correct port - this is where the data is going on loopback
		SockAddr_In tmm_local_loopback_addr  = 	SockAddr_In().port(0);  				//this is where its coming from - we dont care about the source - its set to inaddr_any

		if ((TMM_loopback_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			break;

		CHECK (bind , TMM_loopback_sock, (struct sockaddr *) &tmm_local_loopback_addr.addr, sizeof(tmm_local_loopback_addr.addr)) ;

		CHECK (connect ,TMM_loopback_sock, (struct sockaddr *) &tmm_remote_loopback_addr.addr, sizeof(tmm_remote_loopback_addr.addr));


//		SockAddr_In external_local_addr = 	pctx->config->GetMyExternalInterface();  		//this is my address to the outside world
//		SockAddr_In external_local_addr = 	SockAddr_In().port(tmm_remote_loopback_addr.port());  							    //this is my address to the outside world
		SockAddr_In external_local_addr = pctx->config->GetAddress(pctx->mg); //set the local address to the multicast group
		//initialise the look up table
		std::map<SockAddr_In,MediaSource> source_lookup_table;
		for (uint8_t s=0; s< MediaSource::size(); s++)
			if (MediaSource(s) !=pctx->config->me)
			{
				std::cout << MediaSource(s).getName() <<std::endl << pctx->config->GetAddress(MediaSource(s)) << std::endl;
				source_lookup_table[pctx->config->GetAddress(MediaSource(s)).port(0)]=MediaSource(s); //we dont send back to ourselves
			}

		if ((external_sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			break;			//create socket
		int reuse = 1;																		//permit the same  local address to be reused - you need this as the filtering of mc groups takes place at a higher layer
		//the OS sees this as multiple sockets bound to the same local address and port number....
		CHECK (setsockopt, external_sock, SOL_SOCKET, SO_REUSEPORT, (int *)&reuse, sizeof(reuse));

		//now bind the socket to a multicast group - step one bind (os level)

		CHECK (bind ,external_sock,
				(struct sockaddr *) &external_local_addr.addr, sizeof(external_local_addr.addr));

		//and register it to the multicast group - causes IGMP monkey business to take place
		struct ip_mreq group;
		group.imr_multiaddr = pctx->config->GetAddress(pctx->mg).addr.sin_addr;
		group.imr_interface = SockAddr_In().port(tmm_remote_loopback_addr.port()).addr.sin_addr;

		CHECK(setsockopt, external_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));


		//now spin around reading from the recv socket and forward if the source was one we care about
		do

		{
			SockAddr_In source_address;
			socklen_t socklen(sizeof(source_address.addr));

			buffer_used=recvfrom(external_sock,frame.frame(),TMM_Frame::maxFrameSize,0,(struct sockaddr*)(&source_address.addr),&socklen);
//			pctx->config->ctrl_buffer->Dump();
			if (buffer_used >0 && pctx->config->ctrl_buffer->domain_mask[frame.domain_ID()]) //there is data and the domain is one we are currently interested in
			{
//				std::cout << "source_address\n" << source_address<<std::endl;
				auto k=source_lookup_table.find(source_address.port(0));
				if (k!=source_lookup_table.end()) //the address is one we want to process
				{
//					std::cout << "Recv from mcg " << pctx->mg.getName() << " source " << k->second.getName() << std::endl;
					frame.stream_idx(uint8_t(k->second));							//set the source index to the TMM so it can demux separate streams
					pctx->config->ctrl_buffer->setActive(k->second,pctx->mg);		//flag it as active (even if we dont forward it
					pctx->config->ctrl_buffer->setPower(k->second,frame.pwr()+frame.gain());		//set the frame power so the control head can read it later
					frame.gain(frame.gain()+pctx->config->ctrl_buffer->getGain(k->second)); //and add the current gain setting to the power
					if (pctx->config->ctrl_buffer->isEnabled(k->second,pctx->mg) && frame.pwr()>-60) //its enabled so it needs to be forwarded as well as not being effectively silence
					{
						if (pctx->config->ctrl_buffer->local_loop_back)
							send(TMM_loopback_sock,frame.frame(),frame.packet_sz(),0);
						else
							send(TMM_sock,frame.frame(),frame.packet_sz(),0);
					}
				}
			}
		}
		while (!TMM_WriterThread::halt_threads);
	} while(false);
	close(TMM_sock);
	close(TMM_loopback_sock);
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






