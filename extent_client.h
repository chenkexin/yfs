// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"
class cache_info
{
public:
bool is_dirty;
bool is_valid;
std::string buf;
extent_protocol::attr a;
public:
cache_info()
{
is_dirty = false;
is_valid = false;
buf = "";
};

bool get_dirty(){return is_dirty;};
bool get_valid(){return is_valid;};
std::string get_buf(){return this->buf;};
extent_protocol::attr get_attr(){return this->a;};
void set_dirty(bool b){is_dirty = b;};
void set_valid(bool v){is_valid = v;};
void set_attr(extent_protocol::attr a){this->a = a;};
void set_buf(std::string buf){this->buf = buf;};
void clear_attr()
{
bzero(&a, sizeof(extent_protocol::attr));
};
};

class extent_client {
 private:
  rpcc *cl;
 
  std::map<extent_protocol::extentid_t, cache_info> cache;

 public:
  extent_protocol::status flush(extent_protocol::extentid_t eid);
  extent_client(std::string dst);

  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status setattr(extent_protocol::extentid_t eid, extent_protocol::attr &a);
};

#endif 

