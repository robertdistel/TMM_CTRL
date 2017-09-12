/*
 * main.cpp
 *
 *  Created on: 4 Sep. 2017
 *      Author: robert
 */


#include "global_defs.h"
#include "media_source.h"
#include "multicast_group.h"
#include "TMM_StreamCtrl.h"
#include "sharedmemory.h"
#include "Configuration.h"
#include "TMM_ReaderThread.h"
#include "TMM_WriterThread.h"

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

namespace po = boost::program_options;



int
main (int argc, char** argv)
{
	std::string supported_names;
	supported_names = "Supported identities:\n";

	for (uint8_t k=0; k< MediaSource::size(); k++)
	{
		supported_names = supported_names +  std::to_string(uint32_t(k)) + ") " + MediaSource(k).getName() + "\n";
	}




	  std::string ctrl_file_name(RootSwapName);
	  uint16_t identity(0);
	  bool initialise_swap(false);
	  bool local_loop_back(false);
	  bool remote_loop_back(false);
	  std::string TMM_name;

  po::options_description description ("MyTool Usage");
  description.add_options ()
        ("ctrl_file", po::value<std::string > (&ctrl_file_name), (std::string("file used as persistent swap \n default=")+ctrl_file_name).c_str())
		("re-init", po::value<bool> (&initialise_swap), "reset persistent swap back to default")
		("tmm",po::value<std::string>(&TMM_name),"tmm to use - defaults to raspi0.local")
		("identity,i", po::value<uint16_t > (&identity), supported_names.c_str())
		("local-loop-back",po::value<bool> (&local_loop_back),"loop back data to TMM to simulate data from TMM")
		("remote_loop-back",po::value<bool> (&remote_loop_back),"loop back data from TMM back to the TMM")
		("help,h", "Display this help message")
		("version,v", "Display the version number");
  po::variables_map vm;
  po::store (po::command_line_parser (argc, argv).options (description).run (), vm);
  po::notify (vm);



  if (vm.count ("help"))
    {
      std::cout << description;
      return -1;

    }

  if (vm.count ("version"))
    {
      std::cout << "MyTool Version 1.0" << std::endl;
      return -1;
    }

  if (identity>=MediaSource::size()) return -1;


  SharedMemory<TMM_CntlBuffer> ctrl_buffer(ctrl_file_name.c_str(),initialise_swap); //publish the buffer out to the world
  ctrl_buffer->local_loop_back=local_loop_back;
  ctrl_buffer->remote_loop_back=remote_loop_back;

  Configuration config(MediaSource(uint8_t(identity)), ctrl_buffer);

  TMM_ReaderThread reader_thread(&config);
  reader_thread.start_thread();
  std::vector<TMM_WriterThread> writer_threads;
  for (uint8_t k=0; k<MulticastGroup::size(); k++ )
  {
	  writer_threads.push_back(TMM_WriterThread(&config,MulticastGroup(k)));
	  writer_threads.back ().start_thread ();
  }



  for(uint32_t k=0; k<60; k++)
  {
	    sleep (1000);
	    std::cout << k << std::endl;

  }


}




