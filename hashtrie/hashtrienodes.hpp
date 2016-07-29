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

#ifndef HASHTRIENODES_HPP
#define HASHTRIENODES_HPP

#include <string>
#include <atomic>
#include <string.h>
#include <endian.h>
#include "fptr.hpp"
#include "polynodes.hpp"
#include "polysub.hpp"
#include "radixtree/bitcomboutils.hpp"


class hash_trie_node : public polyinterior{
public:
	int32_t bit_offset;
	int16_t num_bits;
	size_t num_slots;
	size_t num_marks;
	size_t sz_marks_array;
	int8_t a,b,c;
	int bucket_sz;

	std::atomic<size_t> num_children;

	// always followed by:
	// char* marks;
	// fptr<polynode>** slots;


private:
	hash_trie_node(size_t bit_offset, size_t num_bits, size_t num_slots){
		assert(num_bits<=16); // or you're gonna have a bad time...

		this->bit_offset = bit_offset;
		this->num_bits = num_bits;
		this->num_slots = num_slots;

		set_bitcombo_params(bit_offset,num_bits,a,b,c);
		this->num_marks = compute_num_bitcombos(a,b,c);
		this->sz_marks_array = ((num_marks+63)/64)*8;
		this->bucket_sz = compute_bucket_sz(a,b,c);

		num_children.store(0,std::memory_order::memory_order_release);
	}

public:

	inline uint64_t* _marks(){
		char* ptr = (char*)this;
		char* marks_ptr = (ptr+sizeof(hash_trie_node));
		return (uint64_t*)marks_ptr;
	}

	inline fptr<polynode>* _slots(){
		char* ptr = (char*)this;
		fptr<polynode>* slots_ptr = (fptr<polynode>*)
		 (ptr+sizeof(hash_trie_node)+sz_marks_array);
		return slots_ptr;
	}



	static inline size_t size_of(size_t bit_offset, 
	 size_t num_bits, size_t num_slots){

		int8_t a,b,c;
		set_bitcombo_params(bit_offset,num_bits,a,b,c);
		size_t num_marks = compute_num_bitcombos(a,b,c);
		size_t sz_marks_array = ((num_marks+63)/64)*8;
		assert(sz_marks_array>=num_marks/8);
		assert(sz_marks_array%8==0);
		return sizeof(hash_trie_node)+sz_marks_array+
		 num_slots*sizeof(fptr<polynode>);
	}

	inline size_t size_of() const {
		return hash_trie_node::size_of(bit_offset, num_bits, num_slots);
	}

	static inline hash_trie_node* alloc(size_t bit_offset, size_t num_bits, 
	 size_t num_slots){
		size_t sz = hash_trie_node::size_of(bit_offset,num_bits,num_slots);
		void* ptr = malloc(sz);
		memset(ptr,0,sz);
		hash_trie_node *o = new (ptr) hash_trie_node(bit_offset,num_bits,num_slots);
		return o;
	}

	static inline hash_trie_node* alloc_max(size_t bit_offset, size_t num_bits){
		int8_t a,b,c;
		set_bitcombo_params(bit_offset, num_bits,a,b,c);
		size_t num_slots = compute_num_bitcombos(a,b,c);
		hash_trie_node* o = alloc(bit_offset, num_bits, num_slots);
		memset(o->_marks(),0xff,o->sz_marks_array);
		return o;
	}

	static inline hash_trie_node* alloc_max(size_t bit_offset, size_t num_bits, 
	 size_t num_slots){
		return alloc_max(bit_offset, num_bits);
	}

	inline void init_with(key* k1, polynode* n1, key* k2, polynode* n2){
		mark(k1);
		if(k2!=NULL){mark(k2);}
		local_insert(k1,n1);
		if(k2!=NULL){
			local_insert(k2,n2);
			num_children+=2;
		}
		else{num_children++;}
	}

	inline void init_with(key* k1, polynode* n1){
		init_with(k1,n1,NULL,NULL);
	}

	std::string to_string() const{
		return std::string("hash_trie_node < ")+"begin="+std::to_string(bit_offset)
		 +" "+"end="+std::to_string(bit_offset+num_bits)
		 +" "+"num_children="+std::to_string(num_children)+" >";
	}

	inline size_t slot_index_of(key* k){
		size_t mark_idx = key_to_bitcombo_index(k, bit_offset, num_bits, 
		 bucket_sz, a, b, c);

		assert(mark_idx<num_marks);

		if(!have_slot_for(mark_idx)){assert(false);return -1;}

		// now do mark dereferencing to get real index
		size_t idx = num_marks_up_to(mark_idx)-1;

		assert(idx<num_slots);
		return idx;
	}

	inline void mark(key* k){
		size_t mark_idx = key_to_bitcombo_index(k, bit_offset, num_bits, 
		 bucket_sz, a, b, c);
		mark_index(mark_idx);
	}

	inline void mark_index(size_t idx){
		uint64_t* marks = _marks();
		size_t end = idx/64;
		size_t remainder = idx%64;
		uint64_t mask = htobe64(0x1ull<<(63-remainder));
		assert(__builtin_popcountll(mask)==1);
		uint64_t final_int = marks[end];
		//cout<<"mask: "<<hex<<mask<<dec<<endl;
		//cout<<"marks[end]: "<<hex<<final_int<<dec<<endl;
		//cout<<"masked: "<<to_bitset(final_int&mask)<<endl;
		marks[end] = final_int | mask;
	}

	bool inline have_slot_for(size_t idx){
		uint64_t* marks = _marks();
		size_t end = idx/64;
		size_t remainder = idx%64;
		uint64_t mask = htobe64(0x1ull<<(63-remainder));
		uint64_t final_int = marks[end];
		//cout<<"mask: "<<hex<<mask<<dec<<endl;
		//cout<<"marks[end]: "<<hex<<final_int<<dec<<endl;
		//cout<<"masked: "<<to_bitset(final_int&mask)<<endl;
		return (final_int & mask) != 0;
	}

	size_t inline num_marks_up_to(size_t idx){
		uint64_t* marks = _marks();
		size_t end = idx/64;
		size_t remainder = idx%64;
		size_t dist = 0;
		assert(end<sz_marks_array*8);
		for(size_t i = 0; i<end; i++){
			dist +=  __builtin_popcountll(marks[i]);
			assert(dist<=num_slots);
			//cout<<"dist= "<<dist<<endl;
		}

		uint64_t mask = htobe64(~((0x1ull<<(63-remainder))-1));
		uint64_t final_int = marks[end];
		/*std::cout<<"mask: "<<to_bitset(mask)<<std::endl;
		std::cout<<"marks[end]: "<<to_bitset(final_int)<<std::endl;
		std::cout<<"masked: "<<to_bitset(final_int&mask)<<std::endl;*/
		dist += __builtin_popcountll(final_int&mask);
		assert(dist<=num_slots);

		return dist;
	}


	fptr<polynode>* slot(key* k){

		size_t mark_idx = key_to_bitcombo_index(k, bit_offset, num_bits, 
		 bucket_sz, a, b, c);

		assert(mark_idx<num_marks);

		if(!have_slot_for(mark_idx)){return (fptr<polynode>*)DNE;}

		// now do mark dereferencing to get real index
		size_t idx = num_marks_up_to(mark_idx)-1;

		fptr<polynode>* retval = &(_slots()[idx]);

		assert(idx<num_slots);
		assert((char*)retval<((char*)this)+this->size_of());

		return retval;
	}

};



class resize_sub : public polysub{
public:
	key* k;
	value* v;
	resize_sub(key* k, value* v):k(k),v(v){}

	polynode* build();

};



#endif