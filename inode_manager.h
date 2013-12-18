// inode layer interface.

#ifndef inode_h
#define inode_h

#include <stdint.h>
#include "extent_protocol.h" // TODO: delete it
#include <map>
#include <list>
#include <bitset>
#include <vector>
#include <algorithm>
#include <functional>

using namespace std;

#define DISK_SIZE  1024*1024*16
#define BLOCK_SIZE 512
#define BLOCK_NUM  (DISK_SIZE/BLOCK_SIZE)

typedef uint32_t blockid_t;

// disk layer -----------------------------------------

class disk {
 private:
  unsigned char blocks[BLOCK_NUM][BLOCK_SIZE];

 public:
  disk();
  void read_block(uint32_t id, char *buf);
  void write_block(uint32_t id, const char *buf);
  //this following two functions are used in operating indirect blocks.
  void indirect_read_block(uint32_t id, uint32_t* buf);
  void indirect_write_block(uint32_t id, const uint32_t* buf);
};

// block layer -----------------------------------------

typedef struct superblock {
  uint32_t size;
  uint32_t nblocks;
  uint32_t ninodes;
} superblock_t;

class block_manager {
 private:
  disk *d;

  //the cursor which indicates the last allocated block's number
  uint32_t cur_block;

  //is this structure's uint32 is block number and the int is the 
  //flag which indicates if the block is used or not?
  //is this structure the in-memory bitmap?
  //if it is, then the 0 indicates the block is free and 1 indicates
  //it is used.
  std::map <uint32_t, int> using_blocks;
 public:
  block_manager();
  struct superblock sb;

  uint32_t alloc_block();
  
  inline uint32_t get_cur_block()
    {
    return cur_block;
    }
  
  void free_block(uint32_t id);
  void read_block(uint32_t id, char *buf);
  void write_block(uint32_t id, const char *buf);
  //this following functions used for indirect blocks.
  void indirect_read_block(uint32_t id, uint32_t *buf);
  void indirect_write_block(uint32_t id, const uint32_t *buf);
};

// inode layer -----------------------------------------

#define INODE_NUM  1024

// Inodes per block.
#define IPB           (BLOCK_SIZE / sizeof(struct inode))

// Block containing inode i
#define IBLOCK(i, nblocks)     ((nblocks)/BPB + (i)/IPB + 3)

// Bitmap bits per block
#define BPB           (BLOCK_SIZE*8)

// Block containing bit for block b
#define BBLOCK(b) ((b)/BPB + 2)

#define NDIRECT 32
#define NINDIRECT (BLOCK_SIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

typedef struct inode {
  short type;
  unsigned int size;
  unsigned int atime;
  unsigned int mtime;
  unsigned int ctime;
  blockid_t blocks[NDIRECT+1];   // Data block addresses
} inode_t;

class inode_manager {
 private:
  //if the isFirst is true, it must be allocating the root inode.
  bool isFirst;
  block_manager *bm;
  //this vector holds the inum of the inode that has been freed.
  vector<uint32_t> freed_inode;

  void write_file_within_direct_size( uint32_t inum, const char * buf, int size, inode* cur_inode, uint32_t block_num);  
  struct inode* get_inode(uint32_t inum);
  void put_inode(uint32_t inum, struct inode *ino);
 public:
  inode_manager();
  std::map<int, inode*> in_mem_inode_map;
  uint32_t alloc_inode(uint32_t type);
  void free_inode(uint32_t inum);
  void read_file(uint32_t inum, char **buf, int *size);
  void write_file(uint32_t inum, const char *buf, int size);
  void remove_file(uint32_t inum);
  void getattr(uint32_t inum, extent_protocol::attr &a);
  void setattr(uint32_t inum, extent_protocol::attr &a );
};

#endif

