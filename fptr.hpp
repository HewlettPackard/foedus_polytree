/*
 * Copyright (c) 2014-2015, Hewlett-Packard Development Company, LP.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details. You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * HP designates this particular file as subject to the "Classpath" exception
 * as provided by HP in the LICENSE.txt file that accompanied this code.
 *
 */

/* 
fptr.hpp

This file defines the flagged pointers needed for the polytree data 
structure. These pointers are roughly based on the traditional marked 
pointers of Harris's ordered list.  

A flagged pointer (fptr) uses the high 16 bits to maintain state regarding 
the target of the pointer.  The state can be pending, valid, or aborted. 
Pending values are optimistic, and reflect possible future states of the 
pointer, and in general should not be communicated publically.  Valid states 
are completed values of the pointer, and can be returned to the user. Aborted 
states are entirely invalid, they indicate a value that has never completed 
and will soon be removed and (eventually) freed.  The pointer state
follows this state machine:

FP_PENDING -> FP_VALID
           \
            --> FP_ABORTED

These pointers have two variants, a fptr and fptr_local.  An fptr uses 
atomics to ensure a consistent state in shared memory.  An fptr_local is 
appropriate for a local copy and does not maintain a consistent state wrt 
parallel readers (and can consequently ignore memory fences).  Thus, the rule 
is:

"Any pointer which is shared should be a fptr.  Any pointer which is not 
shared should be a fptr_local."

*/

#ifndef FPTR_HPP
#define FPTR_HPP

#include <atomic>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>



#define FP_PENDING 0x0001000000000000
#define FP_VALID   0x0000000000000000
#define FP_ABORTED 0x0010000000000000


template <class T>
class fptr;

// Flagged pointer, local copy.  Non atomic, for use
// to create values for flagged pointers.
template <class T>
class fptr_local{

	uint64_t contents
		__attribute__(( aligned(8) )) =0;

public:
	void inline init(const T* ptr, const uint64_t flag){
		uint64_t a;
		a = (uint64_t)ptr;
		a += flag;
		assert(flag == FP_VALID || flag==FP_ABORTED || flag==FP_PENDING);
		contents=a;
	}
	void inline init(const uint64_t initer){
		uint64_t flag = (initer&0xffff000000000000);
		assert(flag == FP_VALID || flag==FP_ABORTED || flag==FP_PENDING);
		contents=initer;
	}
	void inline init(const fptr<T> ptr){
		contents=ptr.all();
	}
	void inline init(const fptr_local<T> ptr){
		contents=ptr.all();
	}
	uint64_t inline all() const{
		return contents;
	}

	fptr_local<T>(){
		init(NULL,0);
	}
	fptr_local<T>(const uint64_t initer){
		init(initer);
	}
	fptr_local<T>(const T* ptr, const uint64_t sn){
		init(ptr,sn);
	}
	fptr_local<T>(const fptr_local<T> &fp){
		init(fp.all());
	}
	fptr_local<T>(const fptr<T>& fp){
		init(fp.all());
	}
	fptr_local<T>& operator= (const fptr<T>& fp){
		init(fp.all()); 
		return *this;
	}
	fptr_local<T>& operator= (const fptr_local<T>& fp){
		init(fp.all()); 
		return *this;
	}


	inline T& operator *(){return *this->ptr();}
	inline T* operator ->(){return this->ptr();}
	fptr_local<T> (const T*& val) {init(val,0);}
	inline operator T*(){return this->ptr();}

	void inline set_null(){contents=0;}

	inline T* ptr(){return (T*)(contents&0x0000ffffffffffff);}
	uint64_t inline flag(){
		uint64_t retval = (contents&0xffff000000000000);
		assert(retval == FP_VALID || retval==FP_ABORTED || retval==FP_PENDING);
		return retval;
	}
};


// Flagged pointer
template <class T>
class fptr{

	std::atomic<uint64_t> contents
		__attribute__(( aligned(8) ));

public:
	void inline init(const T* ptr, const uint64_t flag){
		uint64_t a;
		a = (uint64_t)ptr;
		a += flag;
		assert(flag == FP_VALID || flag==FP_ABORTED || flag==FP_PENDING);
		contents.store(a,std::memory_order::memory_order_release);
	}
	void inline init(const uint64_t initer){
		uint64_t flag = (initer&0xffff000000000000);
		assert(flag == FP_VALID || flag==FP_ABORTED || flag==FP_PENDING);
		contents.store(initer,std::memory_order::memory_order_release);
	}
	void inline init_relaxed(const uint64_t initer){
		uint64_t flag = (initer&0xffff000000000000);
		assert(flag == FP_VALID || flag==FP_ABORTED || flag==FP_PENDING);
		contents.store(initer,std::memory_order::memory_order_relaxed);
	}
	void inline init_relaxed(const fptr_local<T> initer){
		init_relaxed(initer.all());
	}
	fptr<T>(){
		init(NULL,0);
	}
	fptr<T>(const fptr<T>& fp){
		init(fp.all());
	}
	fptr<T>(const fptr_local<T>& fp){
		init(fp.all());
	}
	fptr<T>(const uint64_t initer){
		init(initer);
	}
	fptr<T>(const T* ptr, const uint64_t flag){
		init(ptr,flag);
	}



	inline T& operator *(){return *this->ptr();}
	inline T* operator ->(){return this->ptr();}
  fptr<T> (const T*& val) {init(val,0);}
  operator T*() {return this->ptr();}

	inline T* ptr(){
		return (T*)((contents.load(std::memory_order::memory_order_acquire))
		 &0x0000ffffffffffff);
	}
	inline uint64_t flag(){
		uint64_t retval = 
		 contents.load(std::memory_order::memory_order_acquire)
		 &0xffff000000000000;
		assert(retval == FP_VALID || retval==FP_ABORTED || retval==FP_PENDING);
		return retval;
	}
	uint64_t inline all() const{
		return contents;
	}	

	bool inline CAS(fptr_local<T> &oldval,fptr_local<T> &newval){
		uint64_t old = oldval.all();
		return contents.compare_exchange_strong(old,newval.all(),std::memory_order::memory_order_acq_rel);
	}
	bool inline CAS(fptr_local<T> &oldval,T* newval, uint64_t newflag){
		fptr_local<T> replacement;
		replacement.init(newval,newflag);
		uint64_t old = oldval.all();
		return contents.compare_exchange_strong(old,replacement.all(),std::memory_order::memory_order_acq_rel);
	}
	bool inline CAS(T* oldval, uint64_t oldflag, T* newval, uint64_t newflag){
		fptr_local<T> replacement;
		replacement.init(newval,newflag);
		fptr_local<T> old;
		old.init(oldval,oldflag);
		uint64_t u = old.all();
		return contents.compare_exchange_strong(u,replacement.all(),std::memory_order::memory_order_acq_rel);
	}

	bool inline validate(T* ptr){
		fptr_local<T> previous;
		previous.init(ptr,FP_PENDING);
		fptr_local<T> replacement;
		replacement.init(ptr,FP_VALID);
		uint64_t old = previous.all();
		return contents.compare_exchange_strong(old,replacement.all(),std::memory_order::memory_order_acq_rel);
	}
	bool inline validate(fptr_local<T> &oldval){
		return validate(oldval.ptr());
	}
	bool inline abort(T* ptr){
		fptr_local<T> previous;
		previous.init(ptr,FP_PENDING);
		fptr_local<T> replacement;
		replacement.init(ptr,FP_ABORTED);
		uint64_t old = previous.all();
		return contents.compare_exchange_strong(old,replacement.all(),std::memory_order::memory_order_acq_rel);
	}
	bool inline abort(fptr_local<T> &oldval){
		return abort(oldval.ptr());
	}

	void inline set_null_relaxed(){
		init(NULL,0);
	}
};


template <class T>
class fptr_val{
public:
	fptr<T>* ptr;
	fptr_local<T> value;

	inline void init(fptr<T>* ptr,	fptr_local<T> val){
		this->ptr = ptr; this->value = val;
	}

	inline fptr<T>* fp(){ return ptr; }
	inline fptr_local<T> val(){ return value; }	

};



#endif















