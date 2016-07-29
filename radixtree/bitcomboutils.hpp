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

#ifndef BITCOMBO_UTILS_HPP
#define BITCOMBO_UTILS_HPP

#include <string>
#include <atomic>
#include <string.h>
#include <bitset>
#include <iostream>



inline void set_bitcombo_params(int32_t bit_offset, int32_t num_bits,
 int8_t& a, int8_t& b, int8_t& c){

	assert(num_bits>0); 
	assert(bit_offset>=0);
	assert(num_bits<=16); // or you're gonna have a bad time...

	a = 8-(bit_offset % 8);
	(a==8)?a=0:a;
	b = std::min(8,num_bits-a);
	c = std::min(8,num_bits-(b+a));
	assert(a+b+c == num_bits);
}

inline int32_t compute_num_bitcombos(int8_t a, int8_t b, int8_t c){
	int32_t num_slots = 0;
	num_slots = (0x1<<a);
	num_slots += b==0?0:(0x1<<(a+b));
	num_slots += c==0?0:(0x1<<(a+b+c));
	return num_slots;
}

inline int32_t compute_bucket_sz(int8_t a, int8_t b, int8_t c){
	(void)a;
	int32_t bucket_sz = b==0?0:(0x1<<b)+1;
	bucket_sz += c==0?0:(0x1<<(b+c));
	return bucket_sz;
}


uint32_t inline subbits(uint32_t string, int32_t offset, int32_t len){
	string >>= (32-(len+offset));
	string <<= (32-(len+offset));
	string <<= offset;
	return string;
}


template <size_t num_bits>
std::bitset<num_bits> to_bitset(const char* arr) {
	std::bitset<num_bits> retval;
	assert(num_bits%8==0);
	int num_bytes = num_bits/8;
	for(int i = 0; i < num_bytes; i++) {
		for(int b = 0; b < 8; b++) {
			retval[(num_bytes-(i+1))*8 + b] = ((arr[i] >> b) & 1);
		}
	}
	return retval;
}

std::bitset<32> inline to_bitset(uint32_t arr) {
	return to_bitset<32>((char*)&arr);
}

std::bitset<64> inline to_bitset(uint64_t arr) {
	return to_bitset<64>((char*)&arr);
}

size_t inline bitcombo_to_index(uint32_t substring, int32_t len,
 int32_t bucket_sz, int8_t a, int8_t b, int8_t c){

	uint32_t k_0_a = subbits(substring,0,a)>>(32-a);
	uint32_t k_a_b = subbits(substring,a,b)>>(32-b);
	uint32_t k_b_n = subbits(substring,a+b,c)>>(32-c);
	//cout<<"k_0_a="<<k_0_a<<",k_a_b="<<k_a_b<<",k_b_n="<<k_b_n<<endl;


	int e_a_b = (a<len)?1:0;
	int e_b_n = (a+b<len)?1:0;
	//cout<<"e_a_b="<<e_a_b<<",e_b_n="<<e_b_n<<endl;


	size_t term_a = a==0?0:k_0_a*bucket_sz;
	size_t term_b = b==0?0:k_a_b*(0x1<<c) + e_a_b;
	size_t term_c = c==0?0:k_b_n + e_b_n;

	size_t idx = term_a + term_b + term_c;
	return idx;

}


uint32_t inline key_to_subbits(key* k, int32_t bit_offset, int32_t num_bits){

	uint32_t substring = 0;
	const char* contents = k->contents();
	size_t key_char_len = k->len();
	size_t char_offset = bit_offset / 8;

	// read all chars
	substring = 0;
	
	//std::cout <<"char_offset:"<< char_offset<< " key_char_len: "<< key_char_len<<std::endl;
	assert(char_offset<=key_char_len);
	if(char_offset<key_char_len){substring += ((uint8_t)contents[char_offset]);}
	substring <<= 8;
	if(char_offset+1<key_char_len){substring += ((uint8_t)contents[char_offset+1]);}
	substring <<= 8;
	if(char_offset+2<key_char_len){substring += ((uint8_t)contents[char_offset+2]);}
	substring <<= 8;	

	// truncate string
	substring = subbits(substring,bit_offset%8,num_bits);
	return substring;
}


int32_t inline key_to_bitcombo_index(key* k, int32_t bit_offset, int32_t num_bits,
 int32_t bucket_sz, int8_t a, int8_t b, int8_t c){

	uint32_t substring = key_to_subbits(k, bit_offset, num_bits);
	size_t key_bit_len = k->len()*8;
	size_t idx = bitcombo_to_index(substring, key_bit_len-bit_offset, 
	 bucket_sz, a,b,c);

	//std::cout <<"bit_offset:"<< bit_offset<< " num_bits: "<< num_bits<<std::endl;
	//std::cout <<"substring:"<< to_bitset(substring)<< std::endl;
	//std::cout <<"idx:"<< idx<< std::endl;
	return idx;

}


#endif