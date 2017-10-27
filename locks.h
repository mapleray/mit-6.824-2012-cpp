#ifndef __LOCKS_H__
#define __LOCKS_H__

#include <map>
#include <pthread.h>
#include "lock_protocol.h"

class Locks {
private:
  enum state {
    FREE,
    LOCKED
  };
  pthread_mutex_t lock_mutex_;
  pthread_cond_t lock_cond_;
  std::map<lock_protocol::lockid_t, state> lock_state_;

public:
  Locks();
  ~Locks();

  bool lock(lock_protocol::lockid_t);
  bool unlock(lock_protocol::lockid_t);
};

#endif
