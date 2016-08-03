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

#include "chromaticnodes.hpp"
#include <iostream>

using namespace std;


polynode* delete_sub::build(){

	chromatic_interior_node* top = dynamic_cast<chromatic_interior_node*>(prev);
	if(top == NULL){assert(false);}

	// copy old
	chromatic_node* left = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[0])).ptr());
	chromatic_node* right = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[1])).ptr());

	// compute new weights
	int64_t new_weight;
	bool delete_left;
	if(left==victim){ delete_left = true;}
	else if(right==victim){ delete_left = false;}
	// TODO correct casting to get this check to work...
	//else if(left!=NULL && left->k()->equals(k)){ delete_left = true;}
	//else if(right!=NULL && right->k()->equals(k)){ delete_left = false;}
	else{return NULL;}

	polynode* new_peak;
	if(delete_left){
		new_weight = right->weight()+top->weight();
		new_peak = right->duplicate(new_weight);
	}
	else{
		new_weight = left->weight()+top->weight();
		new_peak = left->duplicate(new_weight);
	}
	
	return new_peak;
}
