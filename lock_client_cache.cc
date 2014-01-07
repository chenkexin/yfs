// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"

using namespace std;
 
int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
                                     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  cond_ = new Condition(mutex_); 
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
    int ret = lock_protocol::OK;
  
  //the client now may not nessessary call the server rpc every time.
  //if wants, following code may help.
  //this->cl->call(lock_protocol::acquire, this->id, lid, r);  
  /*
   * When client want to acquire a lock, do as following.
   * 1. check the lock info table to see if it already has the lock. If not, send remote acquire, if it does, do seperately according to its lock state:
   *  (1). none: send remote acquire
   *  (2). free: get it.
   *  (3). locked: wait it.
   *  (4). acquiring: wait it.
   *  (5). releasing: wait it.
   *
   * 2. when been waked up by signal, first check the lock's state, if it is:
   *  (1). revoke: that means it must give the lock back right now. so call the release to release the lock. then go to wait on this lock again.
   *  (2). retry: the return value means nothing, it's different than retry_handler, whick should try to get the lock right now.
   *  so the state means we just wait on the lock, and wait. 
   *  (3). freed: get it, returns OK.
   *  others' processing is similar to the instructions above.
   * */
   // MutexLockGuard(mutex_);
 
 //note to check the return value when call remote acquire. 
   int r; 
    std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
    iter = lockinfo_map.find(lid);
    int ret_temp;
  if(iter != lockinfo_map.end())
  {
          MutexLockGuard lock(mutex_);
      switch(iter->second.lock_state)
      {
      case c_state::none:
          iter->second.set(c_state::acquiring);
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
            iter->second.set(c_state::locked);
            iter->second.if_revoke_before = true;
            return ret;
      case c_state::retry:
          iter->second.set(c_state::acquiring);
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
            iter->second.set(c_state::locked);
            iter->second.if_revoke_before = true;
            return ret;
      case c_state::free:
          iter->second.set(c_state::locked);
         //// iter->second.is_silent = false;
          return ret;
      case c_state::acquiring:
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
            iter->second.set(c_state::locked);
            iter->second.if_revoke_before = true;
            return ret;
      case c_state::locked:
          lock.unlock();
          wait(lid,r);
          this->acquire(lid);
         
          break;
      default:
          break;
      }
  }
  //if the lock is not initialized locally
  else
  {
       MutexLockGuard lock(mutex_); 
          c_lockInfo temp;
          temp.set(c_state::acquiring);
          lockinfo_map.insert(std::pair<lock_protocol::lockid_t, c_lockInfo>(lid, temp));
          int temp_ret = this->cl->call(lock_protocol::acquire, this->id, lid, r);
    
         std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter3;
         iter3 = lockinfo_map.find(lid);
         iter3->second.set(c_state::locked);
         iter3->second.if_revoke_before = true;
            return ret;
  }
}

int lock_client_cache::wait(lock_protocol::lockid_t lid, int cid)
{
    MutexLockGuard lock(mutex3_);
    //the variable waiting here is the lock's state
    while(lockinfo_map.find(lid)->second.lock_state == c_state::locked
        && (lockinfo_map.find(lid)->second.if_revoke_before== false))
    {
        cond_->wait();
    }
    //assert(lockinfo_map.find(lid)->second.lock_state != c_state::free);
    //now it has been waken up.
    //do other checking works.
}


lock_protocol::status
lock_client_cache::release( lock_protocol::lockid_t lid)
{
    MutexLockGuard lock(mutex_);
    std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
    iter = lockinfo_map.find(lid);
    int r;
    if(iter != lockinfo_map.end() )
    {
    if(iter->second.if_revoke_before == true)
    {
     //lockinfo_map.erase(iter);   
     iter->second.set(c_state::none);
     this->cl->call(lock_protocol::release, this->id, lid, r);
     iter->second.set_revoke(false);
     //should delete the element in the map.
     cond_->notify();
        return lock_protocol::OK;
    }
    else
    {
        switch(iter->second.lock_state)
        {
            case c_state::free:
                cond_->notify();
                break;
            case c_state::locked:
                iter->second.set(c_state::free);
                cond_->notify();
               // iter->second.is_silent = true;
                break;
            case c_state::releasing:
                iter->second.set(c_state::free);
                cond_->notify();
                break;
            case c_state::acquiring:
                iter->second.set(c_state::free);
                cond_->notify();
                break;
            case c_state::retry:
                iter->second.set(c_state::free);
               cond_->notify();
                break;
        }
    }
    }
  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
 //this function cannot be locked because the client is used by many threads,
 //even if one thread has get remote lock from server, others may still conatins
 //the same function lock in *acquire* function
  //MutexLockGuard lock(mutex2_);
  int ret = rlock_protocol::OK;
  //should give back the lock.

  std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
  iter = lockinfo_map.find(lid);
  //carefully exame this 
  if( iter != lockinfo_map.end() && iter->second.lock_state == c_state::free)
  {
    //should be that no thread is using the lock, it shall be safe to return it back to server.
    iter->second.set(c_state::none);
    return rlock_protocol::RESET;
  }
  if( iter != lockinfo_map.end())
  {
     // iter->second.set(c_state::none);
      iter->second.set_revoke(true);
      this->cond_->notify();
      return ret;
  }
  //if this notify nessessary?
// cond_->notify();
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;
  std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;

  iter = lockinfo_map.find(lid);
  if( iter != lockinfo_map.end() &&iter->second.lock_state == c_state::free)
  {
    iter->second.set(c_state::none);
    this->cond_->notify();
    return rlock_protocol::RESET;
  }
  if( iter != lockinfo_map.end() )
  {
      iter->second.set(c_state::retry);
  }
  cond_->notify();
  return ret;
}

void c_lockInfo::set(int status)
{
    MutexLockGuard lock(mutex_);
    this->lock_state = status;
}
void c_lockInfo::set_revoke(bool b)
{
  MutexLockGuard lock(mutex_);
  this->if_revoke_before = b;
}
