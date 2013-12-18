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
  Condition *cond_;
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(std::string id, lock_protocol::lockid_t, int &);
  int release(std::string id, lock_protocol::lockid_t, int &);
  void wait(lock_protocol::lockid_t, std::string id);
  int revoke_helper(lock_protocol::lockid_t, std::string cid, std::string rid);
  void retry_helper(lock_protocol::lockid_t, std::string rid);
};

#endif
