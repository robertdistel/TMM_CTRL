/*
 * TMM_WriterThread.h
 *
 *  Created on: 5 Sep. 2017
 *      Author: robert
 */

#ifndef TMM_WRITERTHREAD_H_
#define TMM_WRITERTHREAD_H_

#include <memory>

class Configuration;

class MulticastGroup;

class TMM_WriterThread
{
private:

  class Context;
  std::shared_ptr<Context> pcontext;

public:


  TMM_WriterThread () ;


  TMM_WriterThread (Configuration* config_, const MulticastGroup& mg) ;


  void start_thread ();

  void  join (void);

  static bool halt_threads;

};






#endif /* TMM_WRITERTHREAD_H_ */
