#include "inode_manager.h"
#include <math.h> 
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;
  memcpy(buf, blocks[id], BLOCK_SIZE);

  //printf("disk::read_block: the buf is %s\n", buf);
  //printf("disk::read_block: the block[%i] is %s\n", id, blocks[id]);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(blocks[id], buf, BLOCK_SIZE);
  //printf("disk:write_block: the buf is %s\n", buf);
  //printf("disk::write_block: the block[%i] is %s\n", id, blocks[id]);
}

void disk::indirect_read_block( blockid_t id, uint32_t *buf)
{

  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void disk::indirect_write_block( blockid_t id, const uint32_t * buf)
{

  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(blocks[id], buf, BLOCK_SIZE);

}
// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */

    //1. keep a cursor to record the last allocated block.
    //2. mark the bitmap.
    //3. ++ the cursor.
  
   //allocate the buffer contains a block. 
   char buf[BLOCK_SIZE];
   char *block_char;

   int char_num = (int) (cur_block)/8;
   this->read_block(BBLOCK(cur_block), buf);
   block_char = (char*)buf + char_num;
   //and see the cur_block's bit
   //if it is 0(free) or 1(used)
   //because the initial is set to 0;


   bitset<8> char_bit( *block_char );
   
   int setbit = cur_block - (char_num * 8);
   //printf("setbit: %i\n", setbit);
   char_bit.set(setbit);
   *block_char = (char) char_bit.to_ulong();

   this->write_block(BBLOCK(cur_block), buf);

   cur_block++;

   return (cur_block-1);
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  
   char buf[BLOCK_SIZE];
   char *block_char;

   int char_num = (int) (cur_block)/8;
   this->read_block(BBLOCK(cur_block), buf);
   block_char = (char*)buf + char_num;
   //and see the cur_block's bit
   //if it is 0(free) or 1(used)
   //because the initial is set to 0;


   bitset<8> char_bit( *block_char );
   
   int setbit = cur_block - (char_num * 8);
   //printf("setbit: %i\n", setbit);
   char_bit.reset(setbit);
   *block_char = (char) char_bit.to_ulong();

   this->write_block(BBLOCK(cur_block), buf);

  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

  //initial the block begin at 3, since it is required to reserve space for super node, bitmap block and inode table.
  cur_block = 50;
}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

void block_manager::indirect_read_block(uint32_t id, uint32_t* buf)
{
  d->indirect_read_block( id, buf);
}

void block_manager::indirect_write_block(uint32_t id, const uint32_t* buf)
{
 d->indirect_write_block(id, buf);
}
// inode layer -----------------------------------------

inode_manager::inode_manager()
{
    isFirst = true;
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

static int cur_inum;

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
    int inode_num;
    //initialize the cur_inum since the inum begin at 1.
    if(isFirst)
    {
        cur_inum = 1;
        inode_num = cur_inum;
        isFirst = false;
    }
    else 
    {
       //check if there are any freed inodes, if it is, re-alloc the freed inode to the new requirer. And delete it from the freed_inode list.
       if(freed_inode.size() == 0)
       {
           cur_inum++;
           inode_num = cur_inum;
       }
       else
       {
           inode_num = freed_inode.at(0);
           vector<uint32_t>::iterator iter;
           //iter = freed_inode.find(inode_num);
           iter = find( freed_inode.begin(), freed_inode.end(), inode_num );
           if( iter != freed_inode.end() ) freed_inode.erase(iter);
       }
    }
    inode* root_ino = (struct inode*)malloc(sizeof(struct inode));
    root_ino->type = type;
    root_ino->size = 0;
    root_ino->atime = static_cast<unsigned int>(time(NULL));
    root_ino->mtime = static_cast<unsigned int>(time(NULL));
    root_ino->ctime = static_cast<unsigned int>(time(NULL));
    bzero( root_ino->blocks, sizeof( root_ino->blocks));
    root_ino->blocks[NDIRECT] = 0;
    put_inode( inode_num, root_ino );
    return inode_num;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
    //how to detect if an inode is a freed one.?
    //
    //if the read from the disk is NULL, then the inode must be freed.
    
    //release the blocks in the inode
    //and reset the bitmap of these blocks.
    uint32_t temp_id;
    inode* temp_inode = get_inode(inum);

    for(int i = 0; i < NDIRECT+1; i++)
    {
	if( i != NDIRECT)
	{
		temp_id = temp_inode->blocks[i];
		bm->free_block(temp_id);
	}
	else
	{
		if( temp_inode->blocks[NDIRECT] ==0 ) break;
		else
		{
		//release the indirect blocks, and the block itself.
		uint32_t temp_data[BLOCK_SIZE];
        bzero(temp_data, BLOCK_SIZE);
		bm->indirect_read_block(temp_inode->blocks[NDIRECT], temp_data);
		uint32_t* read_ptr = temp_data;
		for( int v = 0; v < BLOCK_SIZE; v++)
		{
            uint32_t id = *read_ptr;
            if(id != 0)
			{
			bm->free_block(id);
			read_ptr++;
			}
			else break;
		}
		bm->free_block(temp_inode->blocks[NDIRECT]);
		}

	}
    }
    
    //remember to clean the inode
    bzero(temp_inode, sizeof(struct inode));
    
    freed_inode.push_back( inum ); 
    return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
   //printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist, try to get inum: \n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("put_inode: \tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */

   char buf[BLOCK_SIZE];
   bzero( buf, BLOCK_SIZE );
   inode* cur_inode = get_inode(inum);
   *buf_out = (char*)malloc(cur_inode->size); 
   bzero( *buf_out, cur_inode->size);
	char* read_ptr = *buf_out;

   if( cur_inode == NULL ) return;
   //if( cur_inode->blocks[NDIRECT] == 0 )
   if( cur_inode->size <= BLOCK_SIZE* NDIRECT)
   {
       //that is the file doesn't contain indirect.
       //so just read every block's data.
       for(int i  = 0; i< NDIRECT; i++)
       {
           uint32_t block_num = cur_inode->blocks[i];
           if(block_num != 0)
           {
               bm->read_block(block_num, buf);
               //judge if the size is small than BLOCK_SIZE
               int read_size = MIN(BLOCK_SIZE, cur_inode->size - *size);
               memcpy( read_ptr, buf, read_size);
               read_ptr += BLOCK_SIZE;
               *size += read_size;
           }
           else break;
       }
   }  
   else
   {
       //first read the file in direct size.
       
       for(int i = 0; i< NDIRECT; i++)
       {
           uint32_t block_num = cur_inode->blocks[i];
	   if(block_num != 0)
           {
               bm->read_block(block_num, buf);
               memcpy( read_ptr, buf, BLOCK_SIZE);
               read_ptr += BLOCK_SIZE;
               *size += BLOCK_SIZE;
           }
           else
               cout<<"error: large file read error in inode_manager.cc 342"<<endl;
       }
           //then read the file oversize the BLOCK_SIZE * NDIRECT
       uint32_t indirect_block_num = cur_inode->blocks[NDIRECT];
       
       //this buffer contains the ids of blocks.
       uint32_t indirect_buf[BLOCK_SIZE];
       bzero(indirect_buf, BLOCK_SIZE);
       //this read pointer points to the block_number that shall be read out.
       uint32_t* indirect_read_ptr = indirect_buf;
       bm->indirect_read_block(indirect_block_num, indirect_buf);
       int indirect_array_size = BLOCK_SIZE;
       for(int i = 0; i< indirect_array_size; i++)
       {
           uint32_t block_id = *indirect_read_ptr;

           if(block_id == 0) return;
	       //the block_id shall never pass the block_id that has already allocated.  
           if(block_id != 0 && block_id < bm->get_cur_block())
           {
           
               char block_data_temp[BLOCK_SIZE];
               bzero(block_data_temp, BLOCK_SIZE);
               bm->read_block( block_id, block_data_temp);
                
               int size_temp = cur_inode->size - *size;
               if( size_temp >= BLOCK_SIZE)
               {
                memcpy(read_ptr, block_data_temp, BLOCK_SIZE);
                
                *size += BLOCK_SIZE;
                read_ptr += BLOCK_SIZE;
                indirect_read_ptr += 1;
               }
               else
               {
                memcpy(read_ptr, block_data_temp, size_temp);
                *size += size_temp;
               }
        }
           else break;
       }
   }

   return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
  * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
    
 //?   //at first I should clear the inode->blocks, just in case two threads are
    //both writing the file.
    
    inode* cur_inode = NULL;
    /*cur_inode = get_inode(inum);
    bzero(cur_inode->blocks, (NDIRECT+1)*sizeof(uint32_t)); 
*/
  
  if(size <= 0) return;
    //check if the size is bigger than the max

   unsigned int block_num = ceil((double)size / (double)(BLOCK_SIZE));
  if( block_num > MAXFILE )
  {
      printf("error: over the max size, please pass in a smaller file.\n");
      return;
  } 
 
 if(block_num < NDIRECT)
 {
   //the size passed in here shall be checked, how about the buffer's size doesn't coinsident with the size passed in? say, lesser or larger? 
    write_file_within_direct_size(inum, buf, size, cur_inode, block_num);
 }
 else
 {
     //allocate an indirect block.
     write_file_within_direct_size(inum, buf, BLOCK_SIZE * NDIRECT, cur_inode, NDIRECT);
     int trunc_size = size - BLOCK_SIZE * NDIRECT;

     char* read_ptr =(char*) buf;
     read_ptr += (BLOCK_SIZE * NDIRECT);
     int trunc_block_num = ceil((double)trunc_size /(double) (BLOCK_SIZE));
     //no need to check the file size again since the check has been made in line 351 this file.
     
     cur_inode = this->get_inode(inum);
    /****************************NOTE:**************************/
    /* do not use char[] since the maximum value the char can store is only 127, when you allocate the 128th block then it will overflow and cause the reading process fail and it's hard to track the problem. So let's just add a group of support functions using uint32_t type, this type, namely, the unsigned int, and store at most 65535, it shall be suficient for this disk since the total block number is 1024*1024*16 / 512 = 32768.*/
    /***********************************************************/
     uint32_t indirect_block_data[BLOCK_SIZE]={0};
     uint32_t indirect_block_number = bm->alloc_block();
     cur_inode->blocks[NDIRECT] = indirect_block_number;
     cur_inode->size = size;
     char block_temp[BLOCK_SIZE];
     
     for(int i = 0; i < trunc_block_num; i++)
     {
         bzero(block_temp, BLOCK_SIZE);
	    uint32_t temp = bm->alloc_block();
        indirect_block_data[i] = temp;
        //1. write the temp id to the INDIRECT block
        //2. write the real data to the block of temp id. 
        
         bm->read_block( temp, block_temp );
         memcpy( block_temp, read_ptr, BLOCK_SIZE );

         char temp_buf[BLOCK_SIZE];
        bzero(temp_buf, BLOCK_SIZE);
        bm->write_block(temp, temp_buf);

         bm->write_block( temp, block_temp);
	     read_ptr += BLOCK_SIZE;
     }
     bm->indirect_write_block(indirect_block_number, indirect_block_data);
     cur_inode->ctime = static_cast<unsigned int>(time(NULL));
     cur_inode->mtime = static_cast<unsigned int>(time(NULL));
     put_inode( inum, cur_inode );
 }
//change the ctime and mtime of the inode

 return;
}

void inode_manager::write_file_within_direct_size(uint32_t inum, const char *buf, int size, inode* cur_inode, uint32_t block_num)
{

     char* read_ptr = (char*)buf;
     int trunc_size = BLOCK_SIZE;
     char block_data[BLOCK_SIZE];
     bzero(block_data, BLOCK_SIZE);

     cur_inode = this->get_inode(inum); 
     if(size <= BLOCK_SIZE * NDIRECT) 
    {
         cur_inode->size = size;
     }
         int cursor = size;
     for( unsigned int i = 0; i < block_num; i++)
     {
         if(cursor >=BLOCK_SIZE)
         {
            if(cur_inode->blocks[i] == 0)
            {
                uint32_t block_number = bm->alloc_block();
                cur_inode->blocks[i] = block_number;
            }
            //and write correct trunctuated buf into that allocated block.
            memcpy(block_data, read_ptr, trunc_size);
            //write the trunc data into disk
        
            char temp[BLOCK_SIZE];
            bzero(temp, BLOCK_SIZE);
            bm->write_block(cur_inode->blocks[i], temp);

            bm->write_block(cur_inode->blocks[i], block_data);
            //increase the pointer to next segment.
            read_ptr += trunc_size;
            cursor -= BLOCK_SIZE;
         }
         else
         {
             if(cur_inode->blocks[i] == 0)
             {
             uint32_t block_number = bm->alloc_block();
             cur_inode->blocks[i] = block_number;
             }

             memcpy(block_data, read_ptr, cursor);
             char temp[BLOCK_SIZE];
             bzero(temp, BLOCK_SIZE);
             bm->write_block(cur_inode->blocks[i], temp);
             bm->write_block(cur_inode->blocks[i], block_data);
         }
     }

     cur_inode->mtime = static_cast<unsigned int>(time(NULL));
     cur_inode->ctime = static_cast<unsigned int>(time(NULL));
     //write the modified inode bakc to disk.
     put_inode(inum, cur_inode);

}
void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
    vector<uint32_t>::iterator iter;
    iter = find( freed_inode.begin(), freed_inode.end(), inum);
    //the inode has been freed
    if( iter != freed_inode.end() ) return;

    inode* cur_inode = get_inode(inum);
    if(cur_inode != NULL)
    {
      a.type= cur_inode->type;
      a.ctime = cur_inode->ctime;
      a.mtime = cur_inode->mtime;
      a.atime = cur_inode->atime;
      a.size = cur_inode->size;
  }
  else
  {
  }
}

void inode_manager::setattr(uint32_t inum, extent_protocol::attr &a )
{
    inode* cur_inode = get_inode(inum);
    if(cur_inode != NULL)
    {
        cur_inode->ctime = a.ctime;
        cur_inode->mtime = a.mtime;
    }
}
void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  free_inode(inum);   
  return;
}
