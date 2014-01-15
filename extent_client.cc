// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "extent_protocol.h"
extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

// a demo to show how to use RPC
extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  cout<<"----------in extent_client::getattr"<<endl;

   extent_protocol::status ret = extent_protocol::OK;
   if(!cache.count(eid))
   {
     cout<<"cache don't contain,"<<endl;
     //eid doesn't exist
     cl->call(extent_protocol::getattr, eid, attr);
     cout<<"type file:"<<extent_protocol::T_FILE<<" attr.type:"<<attr.type<<endl;
   }
   else 
   {
     if(!cache[eid].is_valid && ! cache[eid].is_dirty) cl->call(extent_protocol::getattr, eid, attr);
     else
     {
       cout<<"cache exists, type file:"<<extent_protocol::T_FILE<<"a.type:"<<cache[eid].a.type<<endl;
       attr = cache[eid].a;
     }
   }
   return ret;
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &eid)
{
  MutexLockGuard lock(mutex_);
  cout<<"-----------------in extent_client::create"<<endl;
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  
  ret = cl->call(extent_protocol::create,type, eid);
  //this->put(eid, "");
  cout<<"type file:"<<extent_protocol::T_FILE<<" attr/tupe:"<<type<<endl;
  cout<<"-----------------end extent_client::create"<<endl;
  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  MutexLockGuard lock(mutex_);
/*  extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  ret = cl->call(extent_protocol::get, eid, buf);
  return ret;
*/
  cout<<"-----------------in extent_client::get"<<endl;
extent_protocol::status ret = extent_protocol::OK;
extent_protocol::attr attr;   
bool flag;
  //not initializad and been flushed
  cout<<"in extent_client::get, eid="<<eid<<" false:"<<false<<" is_valid:"<<cache[eid].is_valid<<" is_dirty:"<<cache[eid].is_dirty<<endl;
  cout<<"cache.count:"<<cache.count(eid)<<endl;
  if (!(flag=cache.count(eid)) || (!cache[eid].is_valid && !cache[eid].is_dirty))
    {
      cout<<"should be here"<<endl;
        ret = cl->call(extent_protocol::get, eid, buf);
      
        if (ret == extent_protocol::OK)
        {
            cache[eid].buf = buf;
            cache[eid].is_dirty = false;
            cache[eid].is_valid = true;
            cl->call(extent_protocol::getattr, eid, attr);
            cache[eid].a = attr;
            cache[eid].a.atime = time(NULL);
            if (!flag)
            {
                cache[eid].a.ctime = cache[eid].a.mtime = 0;
                cache[eid].a.size = buf.size();
            }
            cout<<"----------------end extent_client::get"<<endl;
            return ret;
        }
        else 
        {
            cout<<"----------------end extent_client::get"<<endl;
            return ret;
        }
    }
    cout<<"in extent_client:not call extent_server"<<endl;
    buf = cache[eid].buf;
            cout<<"----------------end extent_client::get"<<endl;
    return ret;

    }

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  /*extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  int r=0;
  r = buf.size();
  ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;*/

  //there should be some mechanism to put to extent_server.
  //
  MutexLockGuard lock(mutex_);
  cout<<"----------------in extent_client::put  eid:"<<eid<<endl;
 extent_protocol::status ret = extent_protocol::OK;

    extent_protocol::attr a;
    a.mtime = a.ctime = a.atime = time(NULL);
    a.size = buf.size();
    if (cache.count(eid)) {
      //eid cacheed
        a.atime = cache[eid].a.atime;
        a.type = cache[eid].a.type;
        if (!cache[eid].is_valid) {
        }
    }
    //else cl->call(extent_protocol::put, eid, buf, r);
    cache[eid].buf = buf;
    cache[eid].is_dirty = true;
    cache[eid].a = a;
    cache[eid].is_valid = true;
    cout<<"-----------------end extent_client::put"<<endl;
    return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  MutexLockGuard lock(mutex_);
  cout<<"----------------in extent_client::remove"<<endl;
  /*extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  int r;
  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
  */
    extent_protocol::status ret = extent_protocol::OK;
    if (!cache.count(eid)) {
      //not exists.
        extent_protocol::attr &a = cache[eid].a;
        cache[eid].clear_attr();
        return extent_protocol::NOENT;
    }
    cache[eid].set_valid(false);
    cache[eid].set_dirty(true);
    cout<<"--------------------end extent_client::remove"<<endl;
    return ret;
}

extent_protocol::status extent_client::setattr(extent_protocol::extentid_t eid, extent_protocol::attr &a)
{
    extent_protocol::status ret = extent_protocol::OK;
//    ret = cl->setattr( eid, a);
    return ret;
}

extent_protocol::status extent_client::flush(extent_protocol::extentid_t eid) 
{
    extent_protocol::status ret = extent_protocol::OK;
    if (!cache.count(eid)) 
    {
      //don't exist
        return ret;
    }
    int r;
    if(cache[eid].get_dirty()) 
    {
        if (!cache[eid].get_valid()) 
        {
           cout<<"in extent_client::flush, call remove"<<endl;
          ret = cl->call(extent_protocol::remove, eid, r);
    cache[eid].is_valid = false;
    cache[eid].is_dirty = false;
    cache[eid].clear_attr();
        } 
        else 
        {
          cout<<"in extent_client::flush, call put on eid:"<<eid<<endl;
          cout<<"and eid data:"<<cache[eid].get_buf()<<endl;
    ret = cl->call(extent_protocol::put, eid, cache[eid].get_buf(), r);
    cache[eid].is_valid = false;
    cache[eid].is_dirty = false;
    //extent_protocol::attr temp = cache[eid].get_attr();
    //extent_protocol::attr &a = temp;
    cache[eid].clear_attr();
        }
    } 
    return ret;
}
