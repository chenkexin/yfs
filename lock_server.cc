// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

lock_server::lock_server():
  nacquire (0)
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
    //note the r is the return value.
    //1. check if the map has contained such lock
    //2. if contains, then check if the lock is freed. if not freed, that means the lock is hold by some other client, then just wait. if is freed, then return the lock to the request client.
    //3. if not contains, then allocate a new lock to the requesting client.
    std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
    iter = lockinfo_map.find( lid );
while(true)
{
    MutexLockGuard lock(mutex_);
    if( iter != lockinfo_map.end() )
    {
        //do step 2.
        if( iter->second.getbit()==true )
        {
            r = iter->first;
            std::cout<<"going to set lock:"<<iter->first<<endl;
            iter->second.setbit();
           break;
         }   
    }
    else
    {
        //do step 3.
        s_lockInfo temp;
        temp.setbit();
        std::cout<<"going to set lock:"<<lid<<endl;
        lockinfo_map.insert( std::pair<lock_protocol::lockid_t, s_lockInfo>(lid, temp) );
        std::cout<<"I have allocate a new lock:"<<lid<<endl;
   break;
    }
}
    lock_protocol::status ret = lock_protocol::OK;
    return ret;    
}

lock_protocol::status lock_server::release( int clt, lock_protocol::lockid_t lid, int &r)
{
    MutexLockGuard lock(mutex_);
    lock_protocol::status ret = lock_protocol::OK;

    std::map<lock_protocol::lockid_t, s_lockInfo>::iterator iter;
    iter = lockinfo_map.find(lid);
    if( iter != lockinfo_map.end() )
    {
        if( iter->second.getbit() == false)
        {
        //only when the lid is in this map, can you release a lock
          
            iter->second.clearbit();
        std::cout<<"released the lock:"<<lid<<endl;
        }
        else
        {
        ret = lock_protocol::IOERR;
        std::cout<<"the lock:"<<lid<<" has been released, don't do it twice"<<endl;
        }
    }
    else
    {
        //you can't delete an obj that doesn't requesti
        ret = lock_protocol::IOERR;
        std::cout<<"the lock that requested to release:"<<lid<<" doesn't exist"<<endl;
    }

    return ret;    
}
bool s_lockInfo::getbit()
{
    return this->lock_state;
}

void s_lockInfo::setbit()
{
    MutexLockGuard lock(mutex_);
    std::cout<<"in setbit: lock_state:"<<lock_state<<endl;
    this->lock_state = state::used;
}

void s_lockInfo::clearbit()
{
    MutexLockGuard lock(mutex_);
    std::cout<<"in clearbit: lock_state:"<<lock_state<<endl;
    this->lock_state = state::free;
}

void s_lockInfo::set(int status)
{
    MutexLockGuard lock(mutex_);
    this->lock_state = status;
}

