#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>


#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

class lock_server_cache {
 private:
  int nacquire;
  std::map<lock_protocol::lockid_t, s_lockInfo> lockinfo_map;
  MutexLock mutex_;  
  MutexLock mutex2_;
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(std::string id, lock_protocol::lockid_t, int &);
  int release(std::string id, lock_protocol::lockid_t, int &);
  void release_helper(std::string id, lock_protocol::lockid_t);
};

#endif
