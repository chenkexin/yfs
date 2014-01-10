#ifndef yfs_client_h
#define yfs_client_h
#include <stdio.h>
#include <string>
//<<<<<<< HEAD

#include "lock_protocol.h"
#include "lock_client.h"
#include "lock_client_cache.h"
//=======
#include <stdlib.h>
//>>>>>>> lab2
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
    public:
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  //have no idea what this integer could do
  //but in my yy opinion, this can be used as lock_id.
  int myid;
  static std::string filename(inum);
  static inum n2i(std::string);
  //this is a helper function that trunc the c++ string according to certain character
  vector<string> split( string &str, string trunc);
  string split_str(string &str, string trunc);
 public:
  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum, size_t);
  int getattr(inum, extent_protocol::attr &a);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &, uint32_t );
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
};

#endif 
