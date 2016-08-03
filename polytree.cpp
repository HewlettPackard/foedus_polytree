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

#include "polynodes.hpp"
#include "polysub.hpp"
#include "polytree.hpp"
#include "simplevector.hpp"


#include <iostream>
#include <typeinfo>
#include <pthread.h>

using namespace std;

static thread_local simple_vector<fptr_val<polynode>>* my_vector = NULL; 

inline simple_vector<fptr_val<polynode>>* get_my_vector(){
	if(my_vector == NULL){
		my_vector = new simple_vector<fptr_val<polynode>>(20);
	}
	my_vector->clear();
	return my_vector;
}

/*
Simple search for a key. Never triggers a substitution.
key k : the key to find
path : an empty list; 
	-To be populated with the path, 
	sorted from top (front) to bottom (back).
	-The dummy root node is always the front most object
	-The data node (if present) is the back most object
Returns success (TRUE) or failure (FALSE) wrt finding the data
*/
bool polytree::search(key* k, simple_vector<fptr_val<polynode>>& path, 
 polyoptions opts){

	fptr<polynode>* here = &root;
	polynode* current = NULL;
	uint64_t flag = 0;

	path.clear();

	fptr_local<polynode> here_cpy;
	here_cpy.init(here->all());	
	while(true){
		current = here_cpy.ptr();
		flag = here_cpy.flag();
		if(unlikely(current == NULL)){
			fptr_val<polynode> fv;
			fv.init(here,here_cpy);
			path.push_back(fv);
			return true;
		}
		else if(likely(flag == FP_VALID)){
			if(likely(dynamic_cast<polyinterior*>(current))){
				polyinterior* pi = (polyinterior*)current;
				fptr_val<polynode> fv;
				fv.init(here,here_cpy);
				path.push_back(fv);
				here = pi->slot(k);
				if(here==DNE){return true;}
				here_cpy.init(here->all());	
				continue;
			}
			else if(polydata* pd = dynamic_cast<polydata*>(current)){
				fptr_val<polynode> fv;
				fv.init(here,here_cpy);
				path.push_back(fv);
				return true;
			}
			assert(false);
		}
		else if(flag == FP_ABORTED){
			if(opts.SEARCH_HELP_CLEAN_ABORTS){
				clean_aborted(here,here_cpy.ptr());
				here_cpy.init(here->all());	
				continue;
			}
			else{
				/* effectively speculate on the previous value*/
				here_cpy.init(current->prev,FP_VALID);
				continue;
			}
		}
		else{ // if(flag == FP_PENDING)
			if(opts.SEARCH_HELP_RESOLVE_PENDING){
				help_pending(here,here_cpy,path,opts);
				here_cpy.init(here->all());	
				continue;
			}
			else{
				if(opts.SEARCH_SPECULATE_VIA_PENDING){
					if (polyinterior* pi = dynamic_cast<polyinterior*>(current)){
						fptr_val<polynode> fv;
						fv.init(here,here_cpy);
						path.push_back(fv);
						here = pi->slot(k);
						if(here==DNE){return true;}
						here_cpy.init(here->all());	
						assert(false);
						continue;
					}
				}	
				/* effectively speculate on the previous value*/
				here_cpy.init(current->prev,FP_VALID);
				continue;
			}
		}
	}
	assert(false);
}

bool polytree::preapprove(simple_vector<fptr_val<polynode>>& path, 
 polyoptions opts){

	fptr<polynode>* here;
	fptr_local<polynode> old_cpy;
	fptr_local<polynode> here_cpy;
	fptr_val<polynode> pv;
	polynode* current = NULL;
	uint64_t flag = 0;
	int depth = 0;

	assert(path.size()>0);

	int64_t rit=path.rbegin();
	
	pv = path.get(rit);
	here = pv.fp();
	old_cpy = pv.val();
	here_cpy.init(here->all());	

	// TODO: this needs more thought - we have problems
	// caused by swapping out the target on polysub success.
	while(true){

		current = here_cpy.ptr();
		flag = here_cpy.flag();

		if(flag==FP_ABORTED && current==old_cpy.ptr()){
			/* we speculated by taking a pending node, but it backfired */
			return false;
		} 
		else if(flag==FP_ABORTED && current!=old_cpy.ptr()){
			/* our path might have been covered by an aborted operation, 
				so let's clean up and hope for the best. */
			assert(current!=NULL);
			if(opts.PREAPPROVE_HELP_CLEAN_ABORTS){
				clean_aborted(here,here_cpy.ptr());
				here_cpy.init(here->all());	
				continue;
			}
			else{
				here_cpy.init(current->prev,FP_VALID);	
				continue;
			}
		}
		else if(flag==FP_PENDING && current==old_cpy.ptr()
		 && opts.PREAPPROVE_HELP_RESOLVE_PENDING){
			/* there's a pending node above us which might interfere with us,
				so we decide to help it. */
			help_pending(here,here_cpy,path,opts); 
			here_cpy.init(here->all());	
			continue;
		}
		else if(flag==FP_PENDING && current!=old_cpy.ptr()){
			/* there's a pending node above us which might interfere with us,
				but if we help it, we WILL fail to validate. */
			assert(current!=NULL);
			here_cpy.init(current->prev,FP_VALID);	
			continue;
		}
		else if(here_cpy.ptr()!=old_cpy.ptr()){
			return false;
		} 
		else if(likely(here_cpy.ptr()==old_cpy.ptr() &&
			(flag == FP_VALID || flag == FP_PENDING) )){
			depth++;
			rit--;
			if(rit<0 || depth>=opts.MAX_DEPTH){break;}
			pv = path.get(rit);
			here = pv.fp();
			fptr_local<polynode> old_cpy = pv.val();
			here_cpy.init(here->all());	
			continue;
		}
		assert(false);
	}

	return true;
}

/*
Validates a pending operation
pending_ptr : the pending fptr to validate (points 
	to pending operation / node)
pending_val : the value of the pending fptr
path : the path from 'search' which was used to find
	the operation entry point (includes old value of pending_ptr).
opts : whether or not to ignore ongoing range queries
1) examines the path to determine what needs to be validated
		(last N pointers, where N is max substitution size).
2) snapshots these values
		(verifies they either haven't changed, or ongoing substitution
		is still pending)
3) sets the pending operation to VALID
Returns true if I did the actual validation, false otherwise
*/
// TODO : we need to abort UIP on certain validation failures.
bool polytree::validate_common(fptr<polynode>* pending_ptr, 
 polynode* pending_val,
 simple_vector<fptr_val<polynode>>& path, 
 polyoptions opts, bool helping, bool is_read){

	fptr<polynode>* here;
	fptr_val<polynode> pv;
	fptr_local<polynode> here_cpy;
	fptr_local<polynode> old_cpy;
	polynode* current = NULL;
	uint64_t flag = 0;
	int depth = 0;

	assert(path.size()>0);

	bool CLEAN_ABORTS;
	bool IGNORE_RANGE_QUERY;
	if(helping){
		CLEAN_ABORTS = opts.VALIDATE_OTHERS_HELP_CLEAN_ABORTS;
		IGNORE_RANGE_QUERY = opts.VALIDATE_OTHERS_IGNORE_RANGE_QUERY;
	}
	else{
		CLEAN_ABORTS = opts.VALIDATE_ME_HELP_CLEAN_ABORTS;
		IGNORE_RANGE_QUERY = opts.VALIDATE_ME_IGNORE_RANGE_QUERY;
	}

	int64_t rit=path.rbegin();
	if(!is_read){
		rit--; // skip end because it's what we're validating.
		if(rit<0){return pending_ptr->validate(pending_val);}
	}
	pv = path.get(rit);
	here = pv.fp();
	old_cpy = pv.val();
	here_cpy.init(here->all());	

	// TODO: this needs more thought - we have problems
	// caused by swapping out the actual node on polysub success.
	while(true){

		current = here_cpy.ptr();
		flag = here_cpy.flag();

		if(flag==FP_ABORTED && current!=old_cpy.ptr()){
			/* our path might have been covered by an aborted operation, 
				so let's clean up and hope for the best. */
			assert(current!=NULL);
			if(CLEAN_ABORTS){
				clean_aborted(here,here_cpy.ptr());
				here_cpy.init(here->all());	
				continue;
			}
			else{
				here_cpy.init(here_cpy.ptr()->prev,FP_VALID);	
				continue;
			}
		}
		else if(flag==FP_ABORTED && current==old_cpy.ptr()){
			/* we speculated by taking a pending node, but it backfired */
			return false;
		} 
		else if(flag==FP_PENDING){
			assert(current!=NULL);
			if(here_cpy.ptr()->prev==old_cpy.ptr()){
				/* our speculation hasn't completed */
				help_pending(here,here_cpy,path,opts);
				here_cpy.init(here->all());	
				continue;
			}
			else{
				here_cpy.init(here_cpy.ptr()->prev,FP_VALID);	
				continue;
			}
			/* else: our path has been covered by a pending node, but it's ok */
		}
		else if(here_cpy.ptr()!=old_cpy.ptr()){
			return false;
		} 
		else if(likely(flag==FP_VALID && current==old_cpy.ptr())){
			depth++;
			rit--;
			if(rit<0 || depth>=opts.MAX_DEPTH){break;}
			pv = path.get(rit);
			here = pv.fp();
			old_cpy = pv.val();
			continue;
		}
		else{assert(false);}
	}

	if(!is_read){return pending_ptr->validate(pending_val);}
	else{return true;}
}


bool polytree::insert_update_in_place_at(fptr<polynode>* here,
 polynode* new_node,
 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

	assert(here == path.back().fp());
	fptr_local<polynode> here_cpy;
	while(true){
		/* Get a good copy of the slot, helping anyone in my way */
		here_cpy.init(here->all());
		if(!help_to_valid(here,here_cpy,path,opts)){
			continue;
		}

		/* Verify that the valid node we're replacing is an
		acceptable predecessor (NULL or a polydata) */
		polydata* prev = dynamic_cast<polydata*>(here_cpy.ptr());
		if(here_cpy.ptr()==NULL || prev != NULL){
			/* Actually do the insertion of the new polydata
				(resembles a treiber stack push) */
			new_node->prev = prev;
			if(here->CAS(here_cpy,new_node, FP_PENDING)){
				/* insertion successful */
				return true;}
			else{
				/* slot value changed, retry getting a good copy */
				continue;}
		}
		else{
			/* unacceptable predecessor, so we can't use this slot
			 (valid interior node) */
			return false;
		}
	}

}

bool polytree::insert_substitution_at(fptr<polynode>* here, polysub* sub, 
	simple_vector<fptr_val<polynode>>& path, polyoptions opts){

	assert(here == path.back().fp());
	fptr_local<polynode> here_cpy;
	while(true){
		here_cpy.init(here->all());
		if(!help_to_valid(here,here_cpy,path,opts)){
			continue;
		}

		/* 
		prelink (unbuilt) polysub object at location.
		at this point the polysub is empty and its
		replacement subtree needs to be built
		*/
		sub->to_here = here;
		sub->prev = here_cpy.ptr();
		if(sub->to_here->CAS(sub->prev, FP_VALID, sub, FP_PENDING)){
			return true;
		}
	}
}


bool polytree::construct_substitution(polysub* sub,
 simple_vector<fptr_val<polynode>>& path, polyoptions opts){
	/*
	Since the object is attached, we need 
	to build the replacement subtree.  We call build,
	a per subclass method to create the new subtree
	and abort any ongoing operations in the danger zone.
	*/
	polynode* my_new_peak = sub->build();

	/*
	If my_new_peak is NULL, this indicates that the
	danger zone never contained a subtree which
	met the precondition for this substitution.
	*/
	if(my_new_peak == NULL){
		sub->to_here->CAS(sub, FP_PENDING, sub, FP_ABORTED);
		return false;
	}
	my_new_peak->prev = sub->prev;

	/*
	Attach my_new_peak as a completed result to the
	polysub object
	*/
	if(!attach_new_peak(sub,my_new_peak)){
		// TODO: GC my_new_peak and everything attached
	}

	/*
	Given the new subtree, we link it in
	to replace this polysub object.
	*/
	sub->to_here->CAS(sub, FP_PENDING, 
	 sub->new_peak.load(std::memory_order::memory_order_acquire), FP_PENDING);

	return true;
}

bool polytree::update(key* k, value* v, polyoptions opts, 
 value*& ans_value_out, void* params){
	assert(opts.OP==OP_UPDATE);
	simple_vector<fptr_val<polynode>>& path_out = *get_my_vector();
	polynode* old_node_out, *new_node_out;

	return update(k,v,opts,path_out,old_node_out,
	 new_node_out,ans_value_out,params);
}


bool polytree::update(key* k, value* v, polyoptions opts, 
 simple_vector<fptr_val<polynode>>& path, polynode*& old_node_out, 
 polynode*& new_node_out, value*& ans_value_out, void* params){
	
	fptr<polynode>* here;
	bool callback;

	while(true){

		path.clear();

		/* Find insertion location */
		this->search(k, path,opts);

		/* Preapprove path and help as necessary */
		if(!opts.SKIP_PREAPPROVE && !this->preapprove(path,opts)){
			continue;
		}

		/* callback to get new_node */
		bool return_old = true;
		polynode* new_node = 
		 this->get_update_node(k,v,path,opts,ans_value_out,callback,params);
		if(new_node==NULL){return false;}
		here = path.back().fp();

		/* Do the pending insertion for substitution*/
		if(polysub* sub = dynamic_cast<polysub*>(new_node)){
			if(insert_substitution_at(here,sub,path,opts)){
				if(construct_substitution(sub,path,opts)){
					polynode* peak = 
					 sub->new_peak.load(std::memory_order::memory_order_acquire);
					if(validate_my_op(here, 
					 peak,sub->prev,path,opts)){
						old_node_out = sub->prev;
						new_node_out = peak;
						//cout<<"sub success: "<<(((polyinterior*)peak)->slot(k))->ptr()->to_string()<<endl;
						break;
					}
				}
				// TODO: collect unbuildable sub
			}
			// TODO: GC unvalidated node
		}	
		/* Do the pending insertion for UIP*/
		else{
			if(insert_update_in_place_at(here,new_node,path,opts)){
				if(validate_my_op(here, new_node, new_node->prev, path, opts)){
					old_node_out = new_node->prev;
					new_node_out = new_node;
					break;
				}
			}
			// TODO: abort and GC unvalidated node
		}

	}

	if(callback){
		on_update_success(k, v,
		 path, new_node_out, old_node_out, opts);
	}
	return true;

}




value* polytree::point_read(key* k, polyoptions opts){
	
	simple_vector<fptr_val<polynode>>& path = *get_my_vector();

	bool found;
	fptr<polynode>* here;
	fptr_local<polynode> here_cpy;
	fptr_val<polynode> fv;

	assert(opts.OP==OP_READ);	

	while(true){
		/* Find read location */
		found = this->search(k, path,opts);
		fv = path.back();
		here = fv.fp();
		here_cpy = fv.val();

		/* handle any pending or aborted nodes */
		if(unlikely(here_cpy.flag() == FP_ABORTED)){
			if(opts.READ_HELP_CLEAN_ABORTS){
				clean_aborted(here,here_cpy.ptr());
				here_cpy.init(here->all());	
				path.pop_back();
				fv.init(here,here_cpy);
				path.push_back(fv);
			}
		}
		else if(unlikely(here_cpy.flag() == FP_PENDING)){
			if(opts.READ_HELP_RESOLVE_PENDING){
				help_pending(here,here_cpy,path,opts);
				here_cpy.init(here->all());	
				path.pop_back();
				fv.init(here,here_cpy);
				path.push_back(fv);
			}
		}

		/* callback to get value */
		value* val = this->get_point_read_value(k,path,opts);

		/* optimization - if we never read a pending value in search,
		 we don't need to validate our read - TODO: verify correct*/
		if(!opts.SEARCH_SPECULATE_VIA_PENDING){return val;}

		/* Validate pending value */
		if(validate_my_read(here, here_cpy.ptr(), path, opts)){
			/* successful validation of our own operation, our polynode is VALID */
			return val;
		}

		path.clear();
	}

	assert(false);
}


pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
void print_UIP(key* k, polynode* new_node, 
 simple_vector<fptr_val<polynode>>& path){

	pthread_mutex_lock(&print_lock);

	cout<<"=="<<to_string(k)<<"=="<<endl;
	for(int i = 0; i<path.size(); i++){
		polynode* current = path.get(i).val().ptr();
		if(current!=NULL){cout<<current->to_string()<<endl;}
		else{cout<<"NULL"<<endl;};
	}
	if(new_node!=NULL){cout<<"+++"<<new_node->to_string()<<endl;}
	else{cout<<"NULL"<<endl;};

	pthread_mutex_unlock(&print_lock);
}

