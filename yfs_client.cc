// ifs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  elr = new extent_lock_releaser(ec);
  lc = new lock_client_cache(lock_dst, elr);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}


yfs_client::inum yfs_client::rand_num(uint32_t type)
{
   inum ret = 0;
   ret = (unsigned long long)((rand() & 0x7fffffff)|(type<<31));
   ret &= 0xffffffff;
   return ret;
}



std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  cout<<"--in yfs_client::isfile"<<endl;
    extent_protocol::attr a;
     bzero(&a, sizeof(struct extent_protocol::attr));
    //lc->acquire(inum);

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        //elr->dorelease(inum);
        //lc->release(inum);
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        //elr->dorelease(inum);
        //lc->release(inum);
        cout<<"--end yfs_client::isfile"<<endl;
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
        //elr->dorelease(inum);
    //lc->release(inum);
        cout<<"--end yfs_client::isfile"<<endl;
    return false;
/*    if(inum & 0x80000000)
          return true;
      return false;*/
 }

bool
yfs_client::isdir(inum inum)
{
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
     
    //add lock
    //how to decide the lock id?
    //if the size < or = content, remains unchanged.
    //if the size > content, modify it.
    
    lc->acquire(ino);
    
    extent_protocol::attr a;
    ec->getattr( ino, a );
    if( size > a.size )
    {
        a.size = size;
    }
    //write the a back to inode.
    ec->setattr( ino, a);
    string buf;
    ec->get( ino, buf );
    ec->put( ino, buf );
   
        elr->dorelease(ino);
    lc->release(ino);

    return r;
}

int yfs_client::getattr( inum ino, extent_protocol::attr &a)
{
    //lc->acquire(ino);
    ec->getattr(ino, a);
    //lc->release(ino);
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out, uint32_t type)
{
    int r = OK;

    cout<<"*******************8in parent:"<<parent<<" create file:"<<name<<endl;
    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    //1. check if the file is existed in the dir
    //2. if not, create the file with inode, update the dir, and return the inode num.
    //3. if it exists, return the inode num that already exists.
    //
   // std::cout<<"require lock:"<<parent<<endl;
   lc->acquire(parent); 
   //std::cout<<"in yfs_client::create:, holding lock:"<<parent<<endl; 
    
    bool found;
    inum ino;
   if( this->lookup( parent, name, found, ino ) == EXIST)
    {
        found = true;
        ino_out = ino;
        r = EXIST;
        string parent_block;  
        ec->get(parent, parent_block);
        ec->put(parent, parent_block);
    }
    else
    {
        //else alloc a new one;
        extent_protocol::extentid_t id;
        if( ec->create( type, id) == OK )
        {
            ino_out = id;
        }
        //and modify the parent dir;
        string parent_block;
        
        //force the cache to flush, in order to get right disk data when 'get'
        elr->dorelease(parent);
        // lc->acquire(parent);
        //  lc->acquire(parent);
        ec->get( parent, parent_block );
        string file_name = name;
        parent_block.append("|^"+file_name+"^");
        //convert an integer to string
        string temp_inum;
        std::stringstream out;
        out<< ino_out;
        temp_inum = out.str();
        //-------end converting-------
        string temp_inum_s = temp_inum;
        parent_block.append("*"+temp_inum_s+"*|");
        ec->put( parent, parent_block );   
  //  lc->release(parent);
    }
        elr->dorelease(parent);
    lc->release(parent);
    std::cout<<"***************end of yfs_client::creat"<<endl;
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;
cout<<"in yfs_client::lookup"<<endl;
    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    //1. check if the file is exist in the directory.
    found = false;
    string* file_name = new string(name);
    std::list<dirent> file_list;
    this->readdir( parent, file_list );
    ino_out = parent;
    list<dirent>::iterator iter;
    for( iter = file_list.begin(); iter != file_list.end(); iter++)
    {
        if( iter->name.compare(*file_name) == 0)
        {
           ino_out = iter->inum;
            found = true;
            r = EXIST;
            break;
        }
    }
    return r;
}

//this function trunc the pass-in string
vector<string> yfs_client::split(string &str, string trunc)
{
    vector<string> strs;
    int length = trunc.size();
    int lastPos=0, index = -1;
    int index_2;
    while( -1!= (index = str.find(trunc, lastPos)))
    {
        string temp = (str.substr(lastPos, index - lastPos));
        strs.push_back(temp);
        lastPos = index + length;
    }
    string lastStr = str.substr(lastPos);
    if( ! lastStr.empty() )
        strs.push_back(lastStr);

    return strs;
}

//this function is used to split the pass-in string using certain character.
//this function shall be used to cooperate to parse the file name and file inode in the inode context of directory type.
string yfs_client::split_str(string &str, string trunc)
{
    string str_return;
    int length = trunc.size();
    int lastPos = 0, index = -1;
    int index_2;
    while( -1 != (index = str.find(trunc, lastPos)))
    {
        str_return = str.substr(lastPos, index - lastPos);
        lastPos = index + length;
    }
       
   return str_return; 
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    
    //1. read the block's contents out.
    //2. parse the contents into pairs.
    //3. set the list and return.
    std::string buf;

    //lc->acquire(dir);
    ec->get(dir, buf);

    //the format is as follows:
    //the file name is seperated by ^
    //the inode number is seperated by *
    //and the pair of filename and inode is seperated by |
    //example:
    //| ^[$filename1]^*[$inum]* | ^[$filename2]^*[$inum]* |  ...
    
    string trunc = "|";
    string trunc_2 = "^";
    string trunc_3 = "*";
    vector<string> pairs = split( buf, trunc);
    vector<string>::iterator iter;
    int i=0;
    for( iter = pairs.begin(); iter != pairs.end(); iter++ )
    {
       string name = split_str( *iter, trunc_2 );
       string ino = split_str( *iter, trunc_3 );
       if( (!name.empty()) && (!ino.empty()))
       {
       struct dirent dir_temp;
       dir_temp.name = name;
       dir_temp.inum = n2i(ino);
       list.push_back(dir_temp);
       }
       i++;
    }

    //lc->release(dir);
    //std::cout<<"in yfs_client::lookup, releasing lock:"<<dir<<endl;
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;
    //std::cout<<"require lock:"<<ino<<endl;
lc->acquire(ino);
//std::cout<<"in yfs_client::read, holding the lock:"<<ino<<endl;
    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    string buf;
    //lc->acquire(ino);
    ec->get( ino, buf );
elr->dorelease(ino);
    lc->release(ino);
  //  std::cout<<"releasing the lock:"<<ino<<endl;
    data = buf.substr( off, size );
    //data = buf;
    //lc->release(ino);
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    //std::cout<<"require lock:"<<ino<<endl;
 lc->acquire(ino);  
 //std::cout<<"in yfs_client::write, holding the lock:"<<ino<<endl; 
    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    //first deal with the situation that the off < length.
    string buf;

//    lc->acquire(ino);
    ec->get( ino, buf );
    if( off == 0 )
    {
        buf.clear();
        buf.append( data, size);
    }
    else if( off < buf.size() )
    {
        //make the syntax clear
        string buf_temp;
        buf_temp.append( buf.substr(0, off));
        buf_temp.append( data, size );
        buf_temp.append( buf.substr( off+size, buf.size()-off-size) );
        buf.swap(buf_temp);
    }
    else
    {
        //buf.resize(off+size);
        if( off > buf.size() )
        {
            buf.resize( off + size );
            string s(data);
            buf.replace( off, size, s, 0, size);
            buf.resize( off+size);
        }    
        else buf.append( data, size);
    }
    //write the buffer back.
 
    ec->put( ino, buf );

    //first assume the value is correct.
    bytes_written = size;
   elr->dorelease(ino); 
    lc->release(ino);
   // std::cout<<"releasing the lock:"<<ino<<endl;
    
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;
    std::cout<<"******************** in yfs_client::unlink"<<endl;
    std::cout<<"in parent:"<<parent<<" unlink:"<<name<<endl;
    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
 
    //std::cout<<"require lock:"<<parent<<endl;
    lc->acquire(parent);
   // std::cout<<"yfs_client::unlink, holding the lock:"<<parent<<endl;
    
    bool found;
    inum ino_out=0;
    this->lookup( parent, name, found, ino_out);
    if( ino_out == parent) return NOENT;
    //if(ino_out == parent) return OK;
    //std::cout<<"require lock:"<<ino_out<<endl;
    lc->acquire(ino_out);
    //std::cout<<"in yfs_client::unlink, removing the file, holding lock:"<<ino_out<<endl;
    
    ec->remove(ino_out);
   elr->dorelease(ino_out); 
    lc->release(ino_out);
   // std::cout<<"releasing the lock:"<<ino_out<<endl;
    
    //and modify the parent block
    string buf;
    
    
    ec->get( parent, buf);
    string del_buf;
    
    string trunc = "|";
    string trunc_2 = "^";
    string trunc_3 = "*";
    vector<string> pairs = split( buf, trunc);
    vector<string>::iterator iter;
    for( iter = pairs.begin(); iter != pairs.end(); iter++ )
    {
       string name_1 = split_str( *iter, trunc_2 );
       if( name_1.compare(name) == 0) continue;
       else
       {
        string ino = split_str( *iter, trunc_3 );
        if( (!name_1.empty()) && (!ino.empty()))
        {
           del_buf.append("|^" + name_1+ "^");
           del_buf.append("*" + ino + "*|");
        }
       }
    }
    buf.swap(del_buf);
   // buf.resize(BLOCK_SIZE);
   
    ec->put( parent, buf);
    elr->dorelease(parent);
    lc->release(parent);
    //std::cout<<"releasing the lock:"<<parent<<endl;
    std::cout<<"*********************** finish yfs_client::unlink"<<endl;
    return r;
}

