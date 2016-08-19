#include "trie.hpp"
#include "trienodes.hpp"
#include <iostream>

using namespace std;

trie::trie(){

	default_options.all_flags = 0;
	default_options.all_flags = default_options.all_flags | 
	 always_help_clean_abort();
	default_options.all_flags = default_options.all_flags | 
	 always_help_resolve_pending();
	default_options.MAX_DEPTH = 1;


	get_options.all_flags = default_options.all_flags;
	get_options.FUNC = FUNC_GET;
	get_options.OP = OP_READ;

	put_options.all_flags = default_options.all_flags;
	put_options.FUNC = FUNC_PUT;
	put_options.OP = OP_UPDATE;

	insert_options.all_flags = default_options.all_flags;
	insert_options.FUNC = FUNC_INSERT;
	insert_options.OP = OP_UPDATE;

	replace_options.all_flags = default_options.all_flags;
	replace_options.FUNC = FUNC_REPLACE;
	replace_options.OP = OP_UPDATE;

	remove_options.all_flags = default_options.all_flags;
	remove_options.FUNC = FUNC_REMOVE;
	remove_options.OP = OP_UPDATE;

}

static int inline calculate_depth(simple_vector<fptr_val<polynode>>& path){
	int depth = 0;
	for(int i = 0; i<path.size(); i++){
		fptr_val<polynode> fv = path.get(i);
		polynode* current = fv.val().ptr();
		if(trie_node* pn = dynamic_cast<trie_node*>(current)){
			depth++;
		}
		else if(dynamic_cast<polydata*>(current)){}
		else if(current==NULL){}
		else{assert(false);}
	}
	return depth;
}




polynode* trie::get_update_node(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts,
	 value*& ans_value_out, bool& callback_out, void* params){
	
	assert(opts.OP==OP_UPDATE);

	polynode* current = get_terminal_node(path);
	polynode* n;

	callback_out = true;

	/* leaf slot isn't occupied */
	if(current == NULL){
		return get_update_node_at_NULL(k, v, opts, ans_value_out);
	}
	/* found a removed node */
	else if(removed_node* pd = dynamic_cast<removed_node*>(current)){
		return get_update_node_at_removed(k, v, pd, opts, ans_value_out);
	}
	/* leaf slot is already occupied*/
	else if(kv_node* pd = dynamic_cast<kv_node*>(current)){
		if(pd->k()->equals(k)){
			/* already existing value for this key */
			return get_update_node_at_equals(k, v, pd, opts, ans_value_out);
		}
		else{
			/* key conflict on this slot */
			ans_value_out = NULL;
			switch(opts.FUNC){
			case FUNC_REPLACE:
			case FUNC_REMOVE:
				return NULL;		
			case FUNC_PUT:
			case FUNC_INSERT:	
				/* sprouting insert */
				int64_t depth = calculate_depth(path);
				assert(depth>=0);
				trie_node* top = trie_node::alloc(depth);
				trie_node* node = top;
				while(node->slot(k)==node->slot(pd->k())){	
					/* add nodes until keys differ */
					depth++;
					trie_node* down = trie_node::alloc(depth);
					node->local_insert(k,down);
					node->num_children++;
					node=down;
				}
				node->local_insert(k,kv_node::alloc(k, v));
				node->local_insert(pd->k(),kv_node::alloc(pd->k(), pd->v()));
				node->num_children+=2;
				return top;
			}
		}
	}
	assert(false);return NULL;
}


void trie::on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts){

	//print_UIP(k,new_node,path);

	trie_node* parent;

	// update parent node size
	parent = (trie_node*)get_penultimate_node(path);
	if(likely(parent!=NULL)){
		kv_node* n = dynamic_cast<kv_node*>(new_node);
		kv_node* o = dynamic_cast<kv_node*>(old_node);
		if(n != NULL && o != NULL && !n->k()->equals(o->k()) ){
			parent->num_children++; 
		}
		else if(old_node == NULL){
			parent->num_children++; 
		}
	}

	return;
}

value* trie::get_point_read_value(key* k, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

	assert(opts.OP==OP_READ);
	polynode* current = get_terminal_node(path);

	if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		assert(false);
	}
	else if(polysub* pi = dynamic_cast<polysub*>(current)){
		assert(false);
	}
	else{
		return read_data_node(k,(polydata*)current);
	}
}

value* trie::get(key* k){
	value* v = point_read(k,get_options);
	return v;
}

value* trie::put(key* k, value* v){
	value* ans;
	update(k,v,put_options,ans,NULL);
	return v;
}

bool trie::insert(key* k, value* v){
	value* ans;
	return update(k,v,insert_options,ans,NULL);
}

value* trie::replace(key* k, value* v){
	value* ans;
	update(k,v,replace_options,ans,NULL);
	return ans;
}

value* trie::remove(key* k){
	value* ans;
	update(k,NULL,remove_options,ans,NULL);
	return ans;
}


