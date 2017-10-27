#include "locks.h"

Locks::Locks()
{
  pthread_mutex_init(&lock_mutex_, NULL);
  pthread_cond_init(&lock_cond_, NULL);
}

Locks::~Locks()
{
  pthread_mutex_destroy(&lock_mutex_);
  pthread_cond_destroy(&lock_cond_);
}

bool
Locks::lock(lock_protocol::lockid_t lid)
{
  ScopedLock ml(&lock_mutex_);
  auto it = lock_state_.find(lid);
  if (it == lock_state_.end()) {
    lock_state_[lid] = LOCKED;
    return true;
  }
  while (it->second == LOCKED) {
    pthread_cond_wait(&lock_cond_, &lock_mutex_);
  }
  lock_state_[lid] = LOCKED;
  return true;
}

bool
Locks::unlock(lock_protocol::lockid_t lid)
{
  ScopedLock ml(&lock_mutex_);
  lock_state_[lid] = FREE;
  pthread_cond_signal(&lock_cond_);
  return true;
}
