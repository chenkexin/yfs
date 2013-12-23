// the caching lock server implementation

#include "lock_server_cache.h"
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"
using namespace std;
lock_server_cache::lock_server_cache()
{
  cond_ = new Condition(mutex_);
}


int lock_server_cache::acquire(std::string id, lock_protocol::lockid_t lid, int& )
{
   MutexLockGuard lock(mutex_);
   int r;
  //lock_protocol::status ret = lock_protocol::OK;
  /*
    * When server receive an acquiring to certain lock, it must do several things like following:
   1. check the lockInfo map to see if the lock has been granted to other client, if not, grant it, if it does, do step 2.
   2. check the lock's state, if the state is free(at first, I doubt the state will never appears since the client has cached the lock, but when consider the retry function of the client, I realize that it may cause a endless recursive function call chain of *acquire*. So the state of lock is still nessessary), then grant it to the acquiring client, if not, do as following:
     (1). add current acquiring client to the waiting list.
     (2). send revoke to current holding client. If the call has return, that means the lock has been released.
     (3). pick a client in waiting list to send retry.
     (4). return RETRY.
   
   NOTE: 
   This algorithm will cause the retry arrives before the RETRY return value. 
   When the acquiring client is revoking, it will modify the lock state value. 
   So the acquiring client must check the  lock info when the acquire function has returned, 
   if then, the lock state indicates the lock can be granted(retry), then retry the acquiring, if not, just finish the acquire.
  */
  std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
  std::deque<std::string>::iterator iter_temp;
  
  iter = lockinfo_map.find(lid);
  if(iter != lockinfo_map.end())
  {
    //the lock info is in the table
    //then check the state of the lock.
    switch(iter->second.lock_state)
    {
      case state::free:
        iter->second.set_client(id);
        iter->second.set(state::used);
        if((iter_temp = find(iter->second.wait_queue.begin(), iter->second.wait_queue.end(), id)) != iter->second.wait_queue.end())
          iter->second.wait_queue.erase(iter_temp);
        
        if(iter->second.wait_queue.size() > 0)
        {
          return lock_protocol::RESET;
        }
        else
        {
        return lock_protocol::OK;
      // // break;
        }
        break;
      case state::used:
        //send revoke to previous one and then wait.
        if(iter->second.wait_queue.size() == 0)
        {
          //there is no client waiting on the lock, yet a client holds the lock, so revoke it.
          int ret = this->revoke_helper(lid, iter->second.client_id, id);
          if(ret == rlock_protocol::RESET)
          {
            //give the lock to the acquiring id.
            iter->second.set_client(id);
           if(iter->second.wait_queue.size() > 0)
             return lock_protocol::RESET;
           else
            return lock_protocol::OK;
          }
          else
          {
            iter->second.wait_queue.push_back(id);
            lock.unlock();
            wait(lid, id);
            this->acquire(id, lid, r);
          }

        }
        else
        {
          //there are some other clients waiting on the lock.
          //check if itself is in the list, if it is, then continue to wait. If it isn't, broadcast the revoke information, then go to wait..
  if( find(iter->second.wait_queue.begin(),iter->second.wait_queue.end(), id) == iter->second.wait_queue.end()) 
    {
        // the id is not in waiting queue.
      //at first, notify current holding client.  
      int ret = this->revoke_helper(lid, iter->second.client_id, id);
          if(ret == rlock_protocol::RESET)
          {
            //give the lock to the acquiring id.
            iter->second.set_client(id);
           if(iter->second.wait_queue.size() > 0)
             return lock_protocol::RESET;
           else
            return lock_protocol::OK;
          }

          //then, broadcast to wait queue
      for( int i = 0; i < iter->second.wait_queue.size(); i++)
      {
          int ret2 = this->revoke_helper(lid, iter->second.wait_queue[i], id);
          if(ret2 == rlock_protocol::RESET)
          {
            iter->second.set_client(id);
            //delete the released one.
            iter->second.wait_queue.erase(iter->second.wait_queue.begin()+i);
           if(iter->second.wait_queue.size() > 0)
             return lock_protocol::RESET;
           else
            return lock_protocol::OK;
          }
       }
          iter->second.wait_queue.push_back(id);
        lock.unlock();
        wait(lid, id);
        this->acquire(id, lid, r);
    }
    else
    {
      //continue to wait.
      lock.unlock();
      wait(lid, id);
      this->acquire(id, lid, r);
    }
  }
    }
  }
  else
  {
    //the lock has not been initialized
    s_lockInfo temp;
  cout<<"acquire 6, host:"<<id<<endl;
    temp.set_client(id);
    temp.set(state::used);
    lockinfo_map.insert(std::pair<lock_protocol::lockid_t, s_lockInfo>(lid, temp));
    return lock_protocol::OK;
  }

}

int 
lock_server_cache::release(std::string id, lock_protocol::lockid_t lid, int& r)
{
    MutexLockGuard lock(mutex_);
 
  /*
   * When a server receive a release to certain lock, it must do several thing s as following:
   * 1. Clear the client_id, that is modify the int to zero.
   * 2. Check the waiting list, if the list is not null, pick a client to send retry. If the 
   *    list is null, then modify the lock state to free.
   * */

    std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
    iter = lockinfo_map.find(lid);
    if(iter != lockinfo_map.end())
    {
      if(iter->second.client_id.compare(id) == 0)
      {
        iter->second.client_id.clear();
        iter->second.set(state::free);
        this->retry_helper(lid, id);
      }
    }
    
}
/*void lock_server_cache::release_helper(std::string id, lock_protocol::lockid_t lid)
{
    cout<<"in release_helper, going to retry lock:"<<lid<<" on host:"<<id<<endl;  
    handle h_retry(id);
          rpcc *cl_info = h_retry.safebind();
          if(cl_info)
          {
              int r_info;
              cout<<"going to retry the client:"<<id<<" lock:"<<lid<<endl;
              //retry_handler on the client only modify the lock state on client, when the function returns, it doeasn't means the client has get the lock.
              int ret = cl_info->call(rlock_protocol::retry, lid, r_info);
              if(ret == rlock_protocol::RESET)
              {
  std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
  iter = lockinfo_map.find( lid);
                iter->second.wait_queue.pop_front();
                iter->second.set_client(iter->second.wait_queue.front());
                release_helper( iter->second.wait_queue.front(), lid);
              }
              else
              {
              cout<<"remote retry finish"<<endl;
              }
          }
}*/

void lock_server_cache::wait(lock_protocol::lockid_t lid, std::string id)
{
  //condition wait
  MutexLockGuard lock(mutex2_);
  //condition remains to be verify.
  while(lockinfo_map.find(lid)->second.lock_state == state::used)
  {
    cond_->wait();
  }
}

int lock_server_cache::revoke_helper(lock_protocol::lockid_t lid, std::string cid, std::string rid)
{
  //this function is to revoke the specific client. rid is revoker's id, cid is revokee's id.
  int r;
  int ret = 0;
  handle h(cid);
  rpcc* cl = h.safebind();
  if(cl)
  {
     ret = cl->call(rlock_protocol::revoke, lid, r);
  }
  return ret;
}

void lock_server_cache::retry_helper(lock_protocol::lockid_t lid, std::string rid)
{
  //this function is to send retry message to the specific client.
  //rid is the retry_helper caller's id.
  cond_->notify(); 
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

