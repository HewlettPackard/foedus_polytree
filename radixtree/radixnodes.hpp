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

#ifndef RADIXNODES_HPP
#define RADIXNODES_HPP

#include <string>
#include <atomic>
#include <string.h>
#include "fptr.hpp"
#include "polynodes.hpp"
#include "polysub.hpp"
#include "bitcomboutils.hpp"



class prefix_node : public polyinterior{
public:
	int32_t bit_offset;
	int16_t num_bits;
	int32_t num_slots;
	int8_t a,b,c;
	int bucket_sz;

	std::atomic<size_t> num_children;

private:
	prefix_node(size_t bit_offset, size_t num_bits){
		this->bit_offset = bit_offset;
		this->num_bits = num_bits;

		set_bitcombo_params(bit_offset,num_bits, a,b,c);
		num_slots = compute_num_bitcombos(a,b,c);
		bucket_sz = compute_bucket_sz(a,b,c);

		num_children.store(0,std::memory_order::memory_order_release);
	}

	inline fptr<polynode>* _slots(){
		char* ptr = (char*)this;
		fptr<polynode>* slots_ptr = (fptr<polynode>*)(ptr+sizeof(prefix_node));
		return slots_ptr;
	}

public:

	static inline size_t size_of(size_t bit_offset, size_t num_bits){
		int8_t a,b,c;
		set_bitcombo_params(bit_offset,num_bits, a,b,c);
		int32_t num_slots = compute_num_bitcombos(a,b,c);
		return sizeof(prefix_node)+num_slots*sizeof(fptr<polynode>);
	}

	inline size_t size_of() const {
		return prefix_node::size_of(bit_offset, num_bits);
	}

	static inline prefix_node* alloc(size_t bit_offset, size_t num_bits){
		size_t sz = prefix_node::size_of(bit_offset, num_bits);
		void* ptr = malloc(sz);
		memset(ptr,0,sz);
		prefix_node *o = new (ptr) prefix_node(bit_offset,num_bits);
		
		return o;
	}

	std::string to_string() const{
		return std::string("prefix_node < ")+"begin="+std::to_string(bit_offset)
		 +" "+"end="+std::to_string(bit_offset+num_bits)
		 +" "+"num_children="+std::to_string(num_children)+" >";
	}

	fptr<polynode>* slot(key* k){
		int32_t idx = key_to_bitcombo_index(k, bit_offset, num_bits, 
		 bucket_sz, a, b, c);
		assert(idx<num_slots);
		return &(_slots()[idx]);
	}

};


/*
class small_prefix_node : public prefix_node{
public:
	fptr<polynode> slots[257];

	fptr<polynode>* slot(key* k);

	small_prefix_node(size_t depth) : prefix_node(depth){
		memset(this->slots, 0, 257*sizeof(fptr<polynode>));
	}
};


class mega_prefix_node : public prefix_node{
public:
	fptr<polynode> slots[257*257];

	fptr<polynode>* slot(key* k);

	mega_prefix_node(size_t depth) : prefix_node(depth){
		memset(this->slots, 0, 257*257*sizeof(fptr<polynode>));
	}
};
*/


class merge_sub : public polysub{

public:

	static bool scout_location(polynode* location, int max_fill);
	polynode* build();

};


#endif