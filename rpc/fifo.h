#ifndef fifo_h
#define fifo_h

// fifo template
// blocks enq() and deq() when queue is FULL or EMPTY
//
// 用来实现生产者消费者模式，对std::list的简单封装

#include <errno.h>
#include <list>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include "slock.h"
#include "lang/verify.h"


// 所有需要加锁访问的数据都使用
// ScopedLock 来实现RAII，初始化时加锁，析构时释放锁
// ScopedLock ml(&m);

template<class T>
class fifo {
	public:
		fifo(int m=0); // 0 表示默认队列大小无限制
		~fifo();
		bool enq(T, bool blocking=true); // 入队， 默认为阻塞
		void deq(T *); // 出队
		bool size(); // 队列大小

	private:
		std::list<T> q_; // 使用 list(双向链表) 实现队列元素存储
		pthread_mutex_t m_; // 访问队列元素时，使用互斥锁进行保护
		pthread_cond_t non_empty_c_; // q went non-empty
		pthread_cond_t has_space_c_; // q is not longer overfull
		unsigned int max_; //maximum capacity of the queue, block enq threads if exceeds this limit
};

template<class T>
fifo<T>::fifo(int limit) : max_(limit)
{
	VERIFY(pthread_mutex_init(&m_, 0) == 0);
	VERIFY(pthread_cond_init(&non_empty_c_, 0) == 0);
	VERIFY(pthread_cond_init(&has_space_c_, 0) == 0);
}

template<class T>
fifo<T>::~fifo()
{
	// fifo is to be deleted only when no threads are using it!
	VERIFY(pthread_mutex_destroy(&m_)==0);
	VERIFY(pthread_cond_destroy(&non_empty_c_) == 0);
	VERIFY(pthread_cond_destroy(&has_space_c_) == 0);
}

template<class T> bool
fifo<T>::size()
{
    // 加锁访问队列大小
	ScopedLock ml(&m_);
	return q_.size();
}

// 入队
template<class T> bool
fifo<T>::enq(T e, bool blocking)
{
	ScopedLock ml(&m_);

    // 使用 while 循环检查等待条件比 if 语句更有优势
    // https://computing.llnl.gov/tutorials/pthreads/ 中有说明
	while (1) {
		if (!max_ || q_.size() < max_) {
			q_.push_back(e);
			break;
		}
        // 阻塞条件下，会一直等到队列有空间添加元素为止
		if (blocking)
			VERIFY(pthread_cond_wait(&has_space_c_, &m_) == 0);
		else
			return false;
	}
    // 通知其他线程当前队列不为空
	VERIFY(pthread_cond_signal(&non_empty_c_) == 0);
	return true;
}

// 出队
template<class T> void
fifo<T>::deq(T *e)
{
	ScopedLock ml(&m_);

	while(1) {
        // 队列如果为空会一直阻塞到有数据为止
		if(q_.empty()){
			VERIFY (pthread_cond_wait(&non_empty_c_, &m_) == 0);
		} else {
			*e = q_.front();
			q_.pop_front();
            // 检查是否还有空间
			if (max_ && q_.size() < max_) {
				VERIFY(pthread_cond_signal(&has_space_c_)==0);
			}
			break;
		}
	}
	return;
}

#endif
