// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

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
/*  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;*/
 extent_protocol::status ret = extent_protocol::OK;
    extent_protocol::attr *a;
    if (!cache.count(eid)) {
        a = &cache[eid].a;
        a->atime = a->ctime = a->mtime = a->size = 0;
        cache[eid].is_valid = false;
        cache[eid].is_dirty = false;
    };
    a = &cache[eid].a;
    if (!a->ctime || !a->mtime || !a->atime) {
        extent_protocol::attr temp;
        ret = cl->call(extent_protocol::getattr, eid, temp);
        if (ret == extent_protocol::OK) {
            if (!a->ctime) {
                a->ctime = temp.ctime;
            }
            if (!a->mtime) {
                a->mtime = temp.mtime;
            }
            if (!a->atime) {
                a->atime = temp.atime;
            }
            if (cache[eid].is_dirty) {
                a->size = cache[eid].a.size;
            } else {
                a->size = temp.size;
            }
            attr = *a;
        } else {
        }
    } else {
        attr = *a;
    }
    return ret;

}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here

  ret = cl->call(extent_protocol::create,type, eid);
  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  /*extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  ret = cl->call(extent_protocol::get, eid, buf);
  return ret;*/

extent_protocol::status ret = extent_protocol::OK;
    bool flag;
    if (!(flag=cache.count(eid)) || (!cache[eid].is_valid && !cache[eid].is_dirty)) {
        ret = cl->call(extent_protocol::get, eid, buf);
        if (ret == extent_protocol::OK) {
            cache[eid].buf = buf;
            cache[eid].is_dirty = false;
            cache[eid].is_valid = true;
            cache[eid].a.atime = time(NULL);
            if (!flag) {
                cache[eid].a.ctime = cache[eid].a.mtime = 0;
                cache[eid].a.size = buf.size();
            }
        } else {
            return ret;
        }
    };
    buf = cache[eid].buf;
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
 extent_protocol::status ret = extent_protocol::OK;

    extent_protocol::attr a;
    a.mtime = a.ctime = a.atime = time(NULL);
    a.size = buf.size();
    if (cache.count(eid)) {
        a.atime = cache[eid].a.atime;
        if (!cache[eid].is_valid) {
        }
    }
    cache[eid].buf = buf;
    cache[eid].is_dirty = true;
    cache[eid].a = a;
    cache[eid].is_valid = true;
    return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  /*extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  int r;
  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
  */
    extent_protocol::status ret = extent_protocol::OK;
    if (!cache.count(eid)) {
        extent_protocol::attr &a = cache[eid].a;
        cache[eid].clear_attr();
        return extent_protocol::NOENT;
    }
    cache[eid].set_valid(false);
    cache[eid].set_dirty(true);
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
        return ret;
    }
    int r;
    if(cache[eid].get_dirty()) 
    {
        if (!cache[eid].get_valid()) 
        {
            ret = cl->call(extent_protocol::remove, eid, r);
    //        if (ret != extent_protocol::OK){ }
        } 
        else 
        {
    ret = cl->call(extent_protocol::put, eid, cache[eid].get_buf(), r);
      //      if (ret != extent_protocol::OK){}
        }
    } 
    
    cache[eid].set_valid(false);
    cache[eid].set_dirty(false);
    extent_protocol::attr temp = cache[eid].get_attr();
    extent_protocol::attr &a = temp;
    cache[eid].clear_attr();
    return ret;
}
