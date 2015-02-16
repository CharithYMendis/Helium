bool Abs_Tree::are_abs_trees_similar(std::vector<Abs_Node *> abs_nodes){
	if (abs_nodes.size() == 0) return true;

	Abs_Node * first_node = abs_nodes[0];

	for (int i = 1; i < abs_nodes.size(); i++){
		if ((first_node->type != abs_nodes[i]->type) || (first_node->operation != abs_nodes[i]->operation)){
			return false;
		}
	}

	/*check whether all the nodes have same number of sources*/
	uint no_srcs = abs_nodes[0]->srcs.size();

	for (int i = 1; i < abs_nodes.size(); i++){
		if (abs_nodes[i]->srcs.size() != no_srcs){
			return false;
		}
	}

	/* recursively check whether the src nodes are similar*/
	for (int i = 0; i < no_srcs; i++){
		vector<Abs_Node *> nodes;
		for (int j = 0; j < abs_nodes.size(); j++){
			nodes.push_back(static_cast<Abs_Node *>(abs_nodes[j]->srcs[i]));
		}
		bool ret = are_abs_trees_similar(nodes);
		if (!ret) return false;
	}

	return true;
}

bool Abs_Tree::are_abs_trees_similar(std::vector<Abs_Tree *> abs_trees)
{
	
	vector<Abs_Node *> abs_nodes;
	for (int i = 0; i < abs_trees.size(); i++){
		abs_nodes.push_back(static_cast<Abs_Node *>(abs_trees[i]->get_head()));
	}

	return are_abs_trees_similar(abs_nodes);

}

bool Abs_Tree::are_abs_trees_similar(Abs_Tree * tree){

	vector<Abs_Node *> abs_nodes;
	abs_nodes.push_back(static_cast<Abs_Node *>(this->get_head()));
	abs_nodes.push_back(static_cast<Abs_Node *>(tree->get_head()));

	return are_abs_trees_similar(abs_nodes); 
}
