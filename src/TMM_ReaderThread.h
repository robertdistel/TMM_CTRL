/*
 * TMM_ReaderThread.h
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_READERTHREAD_H_
#define TMM_READERTHREAD_H_

#include <memory>

class Configuration;

class TMM_ReaderThread
{
private:

  class Context;
  std::shared_ptr<Context> pcontext;

public:


  TMM_ReaderThread () ;


  TMM_ReaderThread (Configuration* config_) ;


  void start_thread ();

  void  join (void);

  static bool halt_threads;

};



#endif /* TMM_READERTHREAD_H_ */
