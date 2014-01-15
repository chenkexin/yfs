// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() 
{
  im = new inode_manager();
}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id)
{
  MutexLockGuard lock(mutex_);
  // alloc a new inode and return inum
  printf("extent_server: create inode\n");
  
  id = im->alloc_inode(type);
  cout<<"in create, inod:"<<id<<"type:"<<type<<" file is:"<<extent_protocol::T_FILE<<endl;  
  return extent_protocol::OK;

}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  MutexLockGuard lock(mutex_);
  id &= 0x7fffffff;
  char* buf_c = (char*)malloc(buf.size());
  int size = buf.size();
  std::cout<<"in extent_server:, the size of buf is:"<<buf.size();
  std::cout<<"the context:"<<buf<<endl;
  memcpy(buf_c, buf.data(), size);
  //memset((void*)buf_c,(void*) buf.c_str(), size);
  std::cout<<"the writing buf_c is:"<<buf_c<<endl;
  char* temp = NULL;
  int size_temp = 0;
  im->read_file(id, &temp, &size_temp);
//  std::cout<<"read before write:"<<temp<<endl;
  im->write_file(id, buf_c, size);
  return extent_protocol::OK;
/*
  extent_protocol::attr a;
  //a.mtime = a.ctime = a.atime = time(NULL);
  bzero( &a, sizeof(extent_protocol::attr));  
  im->getattr(id, a);
  a.size = buf.length();
  //MutexLockGuard lock(mutext_);
  std::map<extent_protocol::extentid_t, extent_t>::iterator iter;
  iter = extents.find(id);
  if (iter != extents.end()) 
  {
    a.atime = extents[id].attr.atime; 
  }
  extents[id].data= buf;
  extents[id].attr = a;
  return extent_protocol::OK;
*/
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  MutexLockGuard lock(mutex_);
  printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  cout<<"im->read_file return size:"<<size<<endl;
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    std::cout<<"************reading inode:"<<id<<" ***buf assign:"<<buf<<endl;
    //memcpy( (void*)buf.c_str(), (void*)cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
  /*
  //MutexLockGuard lock(mutext_);
  id &= 0x7fffffff;
  cout<<"in extent_server: get"<<endl;
  std::map<extent_protocol::extentid_t, extent_t>::iterator iter;
  iter = extents.find(id);
 if (iter == extents.end()) 
 {
  printf("extent_server: cache%d not exist\n", id);
   cout<<"id:"<<id<<"doesn't exist in cache"<<endl;
  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0) buf = "";
  else 
  {
    buf.assign(cbuf, size);
    //memcpy( (void*)buf.c_str(), (void*)cbuf, size);
    free(cbuf);
  }
   return extent_protocol::OK;
  
 }
  printf("extent_server: cache exist\n");
 cout<<"id:"<<id<<"exist"<<endl; 
  buf = extents[id].data;
  extents[id].attr.atime = time(NULL);
  return extent_protocol::OK;
*/
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{

  printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->getattr(id, attr);
  a = attr;

  return extent_protocol::OK;
/*
  MutexLockGuard lock(mutex_);
  std::map<extent_protocol::extentid_t, extent_t>::iterator iter;
    iter = extents.find(id);
  if (iter == extents.end()) 
  {
     extent_protocol::attr attr;
     memset(&attr, 0, sizeof(attr));
     im->getattr(id, attr);
     a = attr;
        return extent_protocol::OK;
    }
    a = extents[id].attr;
    return extent_protocol::OK;
*/
    }

int extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr &a )
{
    printf("extent_server: setattr\n");
    im->setattr(id, a);
    return extent_protocol::OK;
}


int extent_server::remove(extent_protocol::extentid_t id, int &)
{

  printf("extent_server: remove %lld\n", id);

  id &= 0x7fffffff;
  im->remove_file(id);
 
  return extent_protocol::OK;
/*
    MutexLockGuard lock(mutex_);
    std::map<extent_protocol::extentid_t, extent_t>::iterator iter;
    iter = extents.find(id);
    if (iter == extents.end())
    {
  id &= 0x7fffffff;
  im->remove_file(id);
 
  return extent_protocol::OK;
    }
    extents.erase(iter);
    return extent_protocol::OK;*/
}

