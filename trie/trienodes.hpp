#ifndef TRIENODES_HPP
#define TRIENODES_HPP

#include <string>
#include <atomic>
#include <string.h>
#include "fptr.hpp"
#include "polynodes.hpp"
#include "polysub.hpp"


class trie_node : public polyinterior{
public:
	size_t depth;
	std::atomic<size_t> num_children;

	fptr<polynode> slots[257];

	fptr<polynode>* slot(key* k){
		size_t len = k->len();
		size_t hash;
		if(depth<len){
			hash = (unsigned char)(k->contents()[depth]);
			assert(hash<256);
			return &(slots[hash]);
		}
		else if(depth==len){
			hash = 256;
			return &(slots[hash]);
		}
		assert(false);
	}

	trie_node(size_t depth){
		this->depth = depth;
		num_children.store(0,std::memory_order::memory_order_release);
		memset(this->slots, 0, 257*sizeof(fptr<polynode>));
	}

	std::string to_string() const{
		return std::string("trie_node < ")+"depth="+std::to_string(depth)
		 +" "+"num_children="+std::to_string(num_children)+" >";
	}

	static inline trie_node* alloc(size_t depth){
		return new trie_node(depth);
	}

};



#endif