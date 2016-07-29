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

#ifndef RADIXTREE_HPP
#define RADIXTREE_HPP

#include "polytree.hpp"

class radix_tree : public polytree {
	
	polyoptions default_options;
	polyoptions insert_options;
	polyoptions put_options;
	polyoptions remove_options;
	polyoptions get_options;
	polyoptions replace_options;

public:
	
	radix_tree();

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