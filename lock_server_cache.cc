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
}


int lock_server_cache::acquire(std::string id, lock_protocol::lockid_t lid, 
                               int &)
{
   MutexLockGuard lock(mutex_);
  lock_protocol::status ret = lock_protocol::OK;
  cout<<"---in acquire, lock id:"<<lid<<" the host:"<<id<<endl; 
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
 
  iter = lockinfo_map.find(lid);
    cout<<"in acquire 1, lock:"<<lid<<" host:"<<id<<endl;
  if( iter != lockinfo_map.end())
  {
      switch(iter->second.lock_state)
      {
          case state::free:
    cout<<"in acquire 2, lock:"<<lid<<" ******** get the lock ****host:"<<id<<endl;
    iter->second.set_client(id);
    iter->second.set(state::used);
      cout<<"****************************return acquire*********"<<endl;
    return lock_protocol::OK;
   case state::retry:
          cout<<"in acquire 15"<<endl;
          //current client id should be the client id that should get the lock
          if(iter->second.client_id.compare(id) == 0)
          {
              cout<<"in acquire 16"<<endl;
              iter->second.set(state::used);
              cout<<"****************get the lock id is:"<<iter->second.wait_queue.front()<<endl;
              cout<<"queue size:"<<iter->second.wait_queue.size()<<endl;
              iter->second.wait_queue.pop_front();
              cout<<"queue size after:"<<iter->second.wait_queue.size()<<endl;
              if(iter->second.wait_queue.size() != 0)
                cout<<"front ele:"<<iter->second.wait_queue.front()<<endl;
                
              cout<<"****************************return acquire*********"<<endl;
              return lock_protocol::OK;
          }
          else
          {
              cout<<"in acquire 20"<<endl;
          //send revoke to previous one if there does not contains itself in the waiting queue

          if( (find(iter->second.wait_queue.begin(), iter->second.wait_queue.end(), id)) == iter->second.wait_queue.end()) 
          {
            if(iter->second.wait_queue.size() == 0)
            {
              iter->second.set_client(id);
              iter->second.set(state::used);
              return rlock_protocol::retry;
            }
            else
            {
              cout<<"in acquire: send revoke to the previous: "<<iter->second.wait_queue.back()<<endl;
          handle h(iter->second.wait_queue.back());
      
                  iter->second.wait_queue.push_back(id);

          rpcc *cl = h.safebind();
          if(cl)
          {
              int r;
              int ret_temp = cl->call(rlock_protocol::revoke, lid, r);
      cout<<"****************************return acquire*********"<<endl;
              return rlock_protocol::retry;
          }
            }
          }

         else
         {cout<<"****************************return acquire*********"<<endl;
             return rlock_protocol::retry;
         }
          }
         return rlock_protocol::retry; 
      //if the wait queue is not empty, you should be carefule about revoke opt to revoke the right client.
   case state::used:

         if(iter->second.client_id.compare(id) == 0)
         {
           return lock_protocol::OK;
         }
         else
         {
         cout<<"in acquire 17"<<endl;
          if(iter->second.wait_queue.size() == 0)
              {
      //add current acquiring client to the waiting list.
      //check if the queue has alredy holds the id's info
          iter->second.wait_queue.push_back(id);
      cout<<"in acquire 3, lock:"<<lid<<" host:"<<id<<endl;
      //use handler to revoke the currently holding client.
      //should using the id in wait_queue, not the passing-in argument.
      cout<<"the queue size:"<<iter->second.wait_queue.size()<<endl;
      handle h (iter->second.client_id);
      //iter->second.client_id.clear();
      rpcc *cl = h.safebind();

      cout<<"in acquire 4, lock:"<<lid<<" host:"<<id<<endl;
      if(cl)
      {
          int r;
          cout<<"going to send revoke to client:"<<iter->second.client_id<<endl;
          //the client's revoke function will not call the release function.
        //  lock.unlock();
          int ret_temp2 = cl->call(rlock_protocol::revoke, lid, r);
          //only here can a revoke return RESET
            if(ret_temp2 == rlock_protocol::RESET)
          {
            iter->second.set_client(id);
            iter->second.set(state::used);
            cout<<"********************** reset get lock, id:"<<id<<endl;
            //delete the entry in waiting list
            cout<<"queue front:"<<iter->second.wait_queue.front()<<endl;
            iter->second.wait_queue.pop_front();
            return lock_protocol::OK;
          }
      }
      cout<<"****************************return acquire*********"<<endl;
          return rlock_protocol::retry;
              }
      else
      {
        //waiting queue is not null
          if( (find(iter->second.wait_queue.begin(), iter->second.wait_queue.end(), id)) == iter->second.wait_queue.end()) 
          {
              cout<<"in acquire: send revoke to the previous: "<<iter->second.wait_queue.back()<<endl;
          handle h(iter->second.wait_queue.back());
      
                  iter->second.wait_queue.push_back(id);

          rpcc *cl = h.safebind();
          if(cl)
          {
              int r;
               int ret_temp3 = cl->call(rlock_protocol::revoke, lid, r);
           
          }
      cout<<"****************************return acquire*********"<<endl;
          return rlock_protocol::retry;
          }

         else
         {
          cout<<"in the waiting list, request for twice"<<endl;
          cout<<"queue size:"<<iter->second.wait_queue.size()<<" and front:"<<iter->second.wait_queue.front()<<endl; 
           cout<<"current client id:"<<iter->second.client_id<<endl;
          cout<<"****************************return acquire*********"<<endl;
           handle h(iter->second.client_id);
           rpcc *cl = h.safebind();
           if(cl)
           {
             int r;
             int ret_temp4 = cl->call(rlock_protocol::revoke, lid, r);
             if(ret_temp4 == rlock_protocol::RESET) 
             {
               iter->second.client_id = id;
               return lock_protocol::OK;
             }
             else return rlock_protocol::retry;
           }
         }
      }
    }
      }
  }
  else
  {
      cout<<"in acquire 6, lock:"<<lid<<" host:"<<id<<endl;
      //the lock has not been initialize.
      s_lockInfo temp;
      temp.set(state::used);
      temp.set_client(id);
      lockinfo_map.insert(std::pair<lock_protocol::lockid_t, s_lockInfo>(lid, temp));
      cout<<"****************************return acquire*********"<<endl;
      return lock_protocol::OK;

  }
 
      cout<<"****************************return acquire*********"<<endl;
}

int 
lock_server_cache::release(std::string id, lock_protocol::lockid_t lid, 
         int &r)
{
    MutexLockGuard lock(mutex_);
    std::cout<<"-----in release. lock:"<<lid<<" *********** release on host:"<<id<<endl;
  lock_protocol::status ret = lock_protocol::OK;
 
   cout<<"1"<<endl; 
  /*
   * When a server receive a release to certain lock, it must do several thing s as following:
   * 1. Clear the client_id, that is modify the int to zero.
   * 2. Check the waiting list, if the list is not null, pick a client to send retry. If the 
   *    list is null, then modify the lock state to free.
   * */

   //what if a thread is requiring to release a lock it doesn't held?
  std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
  iter = lockinfo_map.find( lid);
  string retry_id;
   cout<<"2"<<endl; 
  if(iter != lockinfo_map.end())
  {
      if(iter->second.client_id.compare(id) == 0)
      {
          //current id is the releasing id
      //clear the client id.
          iter->second.client_id.clear();

          iter->second.set(state::retry); 
   cout<<"3"<<endl; 
     
     cout<<"the queue size:"<<iter->second.wait_queue.size()<<endl; 
     if(iter->second.wait_queue.size() > 0)
     { 
     retry_id = iter->second.wait_queue.front();
      iter->second.set_client(retry_id);
     }
      }
      //release a unheld lock
       else
       {
         return ret;
       }
  }
   cout<<"5"<<endl;
   if(!retry_id.empty())
      release_helper(retry_id, lid);

  cout<<"release lock"<<endl;
  return ret;
}
void lock_server_cache::release_helper(std::string id, lock_protocol::lockid_t lid)
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
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

