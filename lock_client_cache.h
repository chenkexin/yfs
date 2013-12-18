// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
//include this header file is because the mutex lock and condition variable definition(the helper class) resides in there
#include "lock_server.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class c_state
{
    public:
    enum c_lock_state{
        none = 0x10000,
        free,
        locked,
        acquiring,
        releasing,
        retry
    };
};

//this class is not neccessary to record thread ids that are waiting on the lock,
//recording id is a method to wake up the thread, here the work is done by using 
//condition variables.
//the system will wake up certain thread, so release me from such work.
class c_lockInfo
{
    private:
        MutexLock mutex_;
    public:
    int lock_state;

    bool if_revoke_before;    
    bool is_silent;
    public:
   c_lockInfo():lock_state(c_state::none){
       if_revoke_before = false;
       is_silent = false;
   };
   void set(int);
};

class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;

  //this structure maintains the infomation about the locks.
  std::map<lock_protocol::lockid_t, c_lockInfo> lockinfo_map;
  MutexLock mutex_;
  MutexLock mutex2_;//seperate lock for acquiring and releasing
  MutexLock mutex3_;
  Condition *cond_;
 public:
  
  static int last_port;
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache() {};
  lock_protocol::status acquire(lock_protocol::lockid_t, int r);
  lock_protocol::status release(lock_protocol::lockid_t, int r);
   
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
  int wait(lock_protocol::lockid_t, int);
  /*
  lock_protocol::status free_proc(lock_protocol::lockid_t);
  lock_protocol::status retry_proc(lock_protocol::lockid_t);
  lock_protocol::status none_proc(lock_protocol::lockid_t);
*/
 };


#endif
