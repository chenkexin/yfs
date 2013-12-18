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
lock_client_cache::acquire(lock_protocol::lockid_t lid, int cid)
{
    cout<<"----------in lock_client::acquire the acquiring lock:"<<lid<<" thread id:"<<cid<<endl;
    int ret = lock_protocol::OK;
  int r;
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
    
    std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
 cout<<"acquire 1"<<" cid:"<<cid<<endl;
    iter = lockinfo_map.find(lid);
    int ret_temp;
  if(iter != lockinfo_map.end())
  {
          MutexLockGuard lock(mutex_);
 cout<<"acquire 2"<<" cid:"<<cid<<endl;
      switch(iter->second.lock_state)
      {
      case c_state::none:
          cout<<"acquire 3"<<" cid"<<cid<<endl;
          iter->second.set(c_state::acquiring);
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
          if(ret_temp == lock_protocol::OK)
          {
              iter->second.set(c_state::locked);
              //iter->second.is_silent = false;
            
              cout<<"cid:"<<cid<<"get the lock*************"<<endl;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
              return ret;
          }
          else if(ret_temp == rlock_protocol::retry)
          {
              lock.unlock();
          cout<<"^^^^^^^^^^^^^^^^^6cid:"<<cid<<" begin to wait on lid:"<<lid<<endl;
              wait(lid, cid);
              this->acquire(lid, cid);
          }
      case c_state::retry:
 cout<<"acquire 4"<<" cid:"<<cid<<endl;
          iter->second.set(c_state::acquiring);
         cout<<"----------------------state::retry--------------------------------calling remote acquire"<<cid<<endl;
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
          if(ret_temp == lock_protocol::OK)
          {
              iter->second.set(c_state::locked);
           //   iter->second.is_silent = false;
              cout<<"cid:"<<cid<<"get the lock*************"<<endl;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
              return ret;
          }
          else if(ret_temp == rlock_protocol::retry)
          {
              lock.unlock();
              //iter->second.set(c_state::none);
              wait(lid,cid);
              this->acquire(lid,cid);
              cout<<"retry lock should never get a return value of protocol::Retry!"<<endl;
          }
      case c_state::free:
          iter->second.set(c_state::locked);
 cout<<"acquire 5"<<" cid"<<cid<<endl;
         //// iter->second.is_silent = false;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
              cout<<"cid:"<<cid<<"get the lock*************"<<endl;
          return ret;
      case c_state::acquiring:

      
          cout<<"acquire 6"<<" cid"<<cid<<endl;
         cout<<"---------------------------------state::acquire---------------------calling remote acquire:"<<cid<<endl;
          ret_temp = this->cl->call(lock_protocol::acquire, this->id, lid, r);
          if(ret_temp == lock_protocol::OK)
          {
              iter->second.set(c_state::locked);
              //iter->second.is_silent = false;
            
              cout<<"cid:"<<cid<<"get the lock*************"<<endl;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
              return ret;
          }
          else if(ret_temp == rlock_protocol::retry)
          {
              lock.unlock();
          cout<<"^^^^^^^^^^acquire but retry^^^^^^^6cid:"<<cid<<" begin to wait on lid:"<<lid<<endl;
              wait(lid,cid);
              this->acquire(lid, cid);
          } 
      
      case c_state::locked:
          lock.unlock();
          cout<<"^^^^^^^^c_state::locked^^^^^^^^^^^^^66cid:"<<cid<<" begin to wait on lid:"<<lid<<endl;
          wait(lid,cid);
          this->acquire(lid,cid);
         
          break;
      default:
          //iter->second.is_silent = false;
          cout<<"what happened, cid:"<<cid<<endl;
              cout<<"this si going to happen:cid:"<<cid<<"get the lock*************"<<endl;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
      }
  }
  //if the lock is not initialized locally
  else
  {
       MutexLockGuard lock(mutex_); 
          cout<<"acquire 7"<<" cid:"<<cid<<endl;
          c_lockInfo temp;
          temp.set(c_state::acquiring);
          lockinfo_map.insert(std::pair<lock_protocol::lockid_t, c_lockInfo>(lid, temp));
          int temp_ret = this->cl->call(lock_protocol::acquire, this->id, lid, r);
    
         cout<<"finish remote call:"<<cid<<endl; 
      if(temp_ret == lock_protocol::OK)
      {
          //here cannot call this->acquire directly
          //in case of endless recursive function call

        iter->second.set(c_state::locked);
        cout<<"wake check"<<lockinfo_map.find(lid)->second.lock_state<<"  the if_revoke_before:"<<lockinfo_map.find(lid)->second.if_revoke_before<<endl;
          
             // iter->second.is_silent = false;
              cout<<"cid:"<<cid<<"get the lock*************"<<endl;
              cout<<"the if_revoke_before:"<<iter->second.if_revoke_before<<endl;
          return ret;
      }
      if(temp_ret == rlock_protocol::retry)
      {
          //add the lock info
          cout<<"acquire 13"<<" cid:"<<cid<<endl;
          lock.unlock();
          cout<<"^^^^^^^^^^^^^^^cid:"<<cid<<" begin to wait on lid:"<<lid<<endl;
          wait(lid,cid);
          this->acquire(lid, cid);
      }
  }
}

int lock_client_cache::wait(lock_protocol::lockid_t lid, int cid)
{
    MutexLockGuard lock(mutex3_);
    //the variable waiting here is the lock's state
    while(lockinfo_map.find(lid)->second.lock_state == c_state::locked
        && (lockinfo_map.find(lid)->second.if_revoke_before== false))
    {
        cout<<"finally wait"<<endl;
        cond_->wait();
        cout<<"free code:"<<c_state::free<<endl;
        cout<<"wake check"<<lockinfo_map.find(lid)->second.lock_state<<"  the if_revoke_before:"<<lockinfo_map.find(lid)->second.if_revoke_before<<endl;
    }
        cout<<"cid:"<<cid<<" wake check"<<lockinfo_map.find(lid)->second.lock_state<<"  the if_revoke_before:"<<lockinfo_map.find(lid)->second.if_revoke_before<<endl;
    cout<<"cid:"<<cid<<"**wake in wait!"<<endl;
    //assert(lockinfo_map.find(lid)->second.lock_state != c_state::free);
    //now it has been waken up.
    //do other checking works.
}


lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid, int cid)
{
    MutexLockGuard lock(mutex_);
    std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
    iter = lockinfo_map.find(lid);
    cout<<"------in client release, lock:"<<lid<<" cid:"<<cid<<" if_revoke_before:"<<iter->second.if_revoke_before<<endl;
cout<<"release 1"<<" cid:"<<cid<<endl;
    int r;
    if(iter != lockinfo_map.end() )
    {
cout<<"release 2"<<" cid:"<<cid<<endl;
    if(iter->second.if_revoke_before == true)
    {
     cout<<"release 7"<<" cid:"<<cid<<endl;
     //lockinfo_map.erase(iter);   
     iter->second.set(c_state::none);
     this->cl->call(lock_protocol::release, this->id, lid, r);
     iter->second.if_revoke_before =false;
     //should delete the element in the map.
     cout<<"^^^^^^^^^^^^^^^^^^^cid: "<<cid<<" going to notify"<<endl;
     cond_->notify();
        return lock_protocol::OK;
    }
    else
    {
        switch(iter->second.lock_state)
        {
            case c_state::free:
cout<<"release 3"<<" cid:"<<cid<<endl;
     cout<<"^^^^^^^^^^^^^^^cid: "<<cid<<" going to notify"<<endl;
                cond_->notify();
                break;
            case c_state::locked:
cout<<"release 4"<<" cid:"<<cid<<endl;
                iter->second.set(c_state::free);
     cout<<"^^^^^^^^^^^^^^^^^cid: "<<cid<<" going to notify"<<endl;
                cond_->notify();
               // iter->second.is_silent = true;
                break;
            case c_state::releasing:
                iter->second.set(c_state::free);
       cout<<"^^^^^^^^^^^^releasing release"<<endl;
                cond_->notify();
                break;
            case c_state::acquiring:
                iter->second.set(c_state::free);
                cout<<"^^^^^^^^^^acquiring release"<<endl;
                cond_->notify();
                break;
            case c_state::retry:
                iter->second.set(c_state::free);
     cout<<"^^^^^^^^^^^^^^^cid: "<<cid<<" going to notify"<<endl;
               cond_->notify();
                break;
        }
    }
    }
cout<<"finish releas"<<endl;
  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
 //this function cannot be locked because the client is used by many threads,
 //even if one thread has get remote lock from server, others may still conatins
 //the same function lock in *acquire* function
 // MutexLockGuard lock(mutex_);
  int ret = rlock_protocol::OK;
 cout<<"----in revoke"<<endl; 
  //should give back the lock.

  std::map<lock_protocol::lockid_t, c_lockInfo>::iterator iter;
  iter = lockinfo_map.find(lid);
  cout<<"map size:"<<lockinfo_map.size()<<endl;
  cout<<"revoke 0"<<endl;
  //carefully exame this 

  if( iter != lockinfo_map.end())
  {
      cout<<"revoke 1"<<endl;
     // iter->second.set(c_state::none);
      
      iter->second.if_revoke_before = true;
      cout<<"has set the revoke = true:"<<true<<endl;
      cout<<"current state::"<<iter->second.lock_state<<endl;
      std::cout<<"*************revoke is going to notif*******************"<<endl;
      this->cond_->notify();
      return ret;
  }
  else
  {
    
  }
  //if this notify nessessary?
// cond_->notify();
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
    cout<<"-----in client retry_handler"<<endl;
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
      cout<<" retry 1"<<endl;
      iter->second.set(c_state::retry);
  }
  else
  {
  }
  std::cout<<"*************retry is going to notif*******************"<<endl;
  cond_->notify();
  return ret;
}

void c_lockInfo::set(int status)
{
    MutexLockGuard lock(mutex_);
    this->lock_state = status;
}
