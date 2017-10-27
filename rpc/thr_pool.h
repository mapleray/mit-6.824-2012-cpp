#ifndef __THR_POOL__
#define __THR_POOL__

#include <pthread.h>
#include <vector>

#include "fifo.h"

// 线程池
class ThrPool {
	public:
		struct job_t {
			void *(*f)(void *); //function pointer  函数指针
			void *a; //function arguments  函数参数
		};

		ThrPool(int sz, bool blocking=true);
		~ThrPool();
        // 添加Job
		template<class C, class A> bool addObjJob(C *o, void (C::*m)(A), A a);
		void waitDone();
        // 获得Job
		bool takeJob(job_t *j);

	private:
		pthread_attr_t attr_;
		int nthreads_;  // 线程池大小
		bool blockadd_; // 阻塞状态

		fifo<job_t> jobq_;  // 元素类型为job_t的队列
		std::vector<pthread_t> th_; // 线程容器

		bool addJob(void *(*f)(void *), void *a);
};


// 第一个参数为指向 类C 类型的指针
// 第二个参数为一个类成员函数指针, 其参数是一个类型A
// 第三个参数是传入的函数的参数
// 即第三个参数是第二个参数调用时所需要的参数
template <class C, class A> bool
ThrPool::addObjJob(C *o, void (C::*m)(A), A a)
{

	class objfunc_wrapper {
		public:
			C *o;
			void (C::*m)(A a); // 声明一个类成员指针m
			A a;
			static void *func(void *vvv) {
				objfunc_wrapper *x = (objfunc_wrapper*)vvv;
				C *o = x->o;
				void (C::*m)(A ) = x->m;
				A a = x->a;
				(o->*m)(a); // 类成员函数指针，调用的是局部变量m
				delete x;
				return 0;
			}
	};

	objfunc_wrapper *x = new objfunc_wrapper;
	x->o = o;
	x->m = m;
	x->a = a;
	return addJob(&objfunc_wrapper::func, (void *)x);
}


#endif
