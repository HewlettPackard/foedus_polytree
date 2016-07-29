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

#ifndef SIMPLEVECTOR_HPP
#define SIMPLEVECTOR_HPP

template <class T> 
class simple_vector {
	
	T* array = NULL;
	int64_t capacity = 0;
	int64_t sz = 0;

public:
	
	simple_vector<T>(int64_t capacity) : capacity(capacity){
		sz = 0;
		assert(capacity>0);
		array = (T*)malloc(capacity*sizeof(T));
	}

	inline void push_back(T val){
		assert(array!=NULL);
		if(sz>=capacity){
			T* new_array = (T*)malloc(capacity*2*sizeof(T));
			memcpy(new_array,array,capacity);
			free(array);
			capacity = capacity*2;
			array = new_array;
		}
		assert(sz<capacity);
		array[sz] = val;
		sz++;
	}

	inline void pop_back(){
		if(sz>0){sz--;}
	}

	inline void clear(){sz=0;}
	inline T back(){
		assert(sz>0);
		return get(rbegin());
	}
	inline T get(int idx){
		assert(idx>=0);
		assert(idx<sz);
		assert(idx<capacity);
		return array[idx];
	}
	inline int64_t size(){return sz;}
	inline int64_t rbegin(){return sz-1;}

};


#endif