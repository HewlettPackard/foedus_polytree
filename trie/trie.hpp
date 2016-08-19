#ifndef TRIE_HPP
#define TRIE_HPP

#include "polytree.hpp"

class trie : public polytree {
	
	polyoptions default_options;
	polyoptions insert_options;
	polyoptions put_options;
	polyoptions remove_options;
	polyoptions get_options;
	polyoptions replace_options;

public:
	
	trie();

	value* get(key* k);
	value* put(key* k, value* v);
	bool insert(key* k, value* v);
	value* remove(key* k);
	value* replace(key* k, value* v);

	polynode* get_update_node(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts,
	 value*& ans_value_out, bool& callback_out, void* params);

	value* get_point_read_value(key* k, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts);

	void on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts);

};


#endif