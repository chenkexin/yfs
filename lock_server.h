// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include <deque>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
using namespace std;
class MutexLock
{
    public:
    MutexLock()
    {
        pthread_mutex_init(&mutex_, NULL);
    }
    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex_);
    }
    public:
    pthread_mutex_t* getPthreadMutex()
    {
        return &mutex_;
    }
    void lock()
    {
        pthread_mutex_lock(&mutex_);
    }
    void unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }
    private:
    pthread_mutex_t mutex_;
};

class MutexLockGuard
{
    public:
        MutexLockGuard(MutexLock& mutex): mutex_(mutex)
         {mutex_.lock(); }
        void unlock()
        {
            //basically should not use this function
            mutex_.unlock();
        };
        ~MutexLockGuard()
        {mutex_.unlock();}
    private:
        MutexLock& mutex_;
};

#define MutexLockGuard(x) static_assert(false, "missing mutex guard var name")

class Condition
{
    public:
        Condition( MutexLock& mutex ): mutex_(mutex)
        {
            pthread_cond_init(&pcond_, NULL);
        }

        ~Condition()
        {
            pthread_cond_destroy(&pcond_);
        }
        void wait()
        {
            pthread_cond_wait( &pcond_, mutex_.getPthreadMutex());
        }
        void notify()
        {
            pthread_cond_signal(&pcond_);
        }
        void notifyAll()
        {
            pthread_cond_broadcast(&pcond_);
        }
    private:
        pthread_cond_t pcond_;
        MutexLock mutex_;
};
class state
{
    public:
enum l_state
{
    free = 0x9003,
    used,
    retry
};
};
class s_lockInfo
{
    private:

        MutexLock mutex_;
    public:
        string client_id;//this is used to find the client rpc connection
        int lock_state;
        std::deque<std::string> wait_queue;//the int is the client's id.        
        s_lockInfo():lock_state(state::used){};
        void setbit();
        void clearbit();
        bool getbit();
        void set(int);
        void set_client(string id)
        {
            MutexLockGuard lock(mutex_);
            this->client_id = id;
        }
};

class lock_server {
    private:
        MutexLock mutex_;

 protected:
  int nacquire;
  //this structure maintains the infomation about the locks.
  std::map<lock_protocol::lockid_t, s_lockInfo> lockinfo_map;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
  
};

#endif 







