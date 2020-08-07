/*
MIT License

Copyright (c) [2019] [Joshua Blickensd�rfer]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Header/Formula.h"
#include <iostream>


void Formula::add_Variables(){

	dag->add_Variables(iteration);

	positive_Sample->add_Variables(iteration);

	negative_Sample->add_Variables(iteration);

	if (using_Grammar) context_Free_Grammar->add_Variables(iteration);
}

Solver_Result Formula::solve_Iteration_Incrementally(){

	if (optimized_Run == 0) {
		score_goal = 0.0;
	}
	if (score_goal < 1.0) {
		//TODO
	}

	Solve_And_Optimize *solver_Optimizer;
	z3::solver solver(context);
	solver_Optimizer = new Sat_Solver(solver);

	solver_Optimizer->push();

	// Only these Y Variables will be removed from the solver after the iteration
	for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
		solver_Optimizer->add(positive_Sample->variables_Y_Word_i_t[i][iteration][0]);
	}
	for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
		solver_Optimizer->add(!negative_Sample->variables_Y_Word_i_t[i][iteration][0]);
	}



	Solver_Result result;
	if (solver_Optimizer->check() == z3::sat) {
		finish = clock();
		z3_Time += (finish - start) /(double) CLOCKS_PER_SEC;
		start = clock();
		make_Result(*solver_Optimizer, result);
		if (verbose >= 2) std::cout << "satisfiable" << std::endl;
		if (verbose >= 0) std::cout << result.formula << std::endl;
	}
	else {

		solver_Optimizer->pop();
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();

		if (verbose >= 2) std::cout << "not satisfiable" << std::endl;
		result.satisfiable = false;
	}

	return result;
}

Solver_Result Formula::solve_Iteration()
{
	if (using_Incremental) {
		return solve_Iteration_Incrementally();
	}

	Solve_And_Optimize *solver_Optimizer;
	z3::optimize optimize(context);
	z3::solver solver(context);
	if (optimized_Run == 0) {
		score_goal = 0.0;
	}
	if (score_goal < 1.0) {
		solver_Optimizer = new Max_Sat_Solver(optimize);
	} else {
		solver_Optimizer = new Sat_Solver(solver);
	}
	
	add_Formulas(*solver_Optimizer);

	Solver_Result result;
	if (solver_Optimizer->check() == z3::sat) {
		make_Result(*solver_Optimizer, result);
		if (result.score < score_goal) {
			result.satisfiable = false;
		}
	} else {
		result.satisfiable = false;
	}

	finish = clock();
	z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
	start = clock();

	if (verbose >= 2) {
		if (result.satisfiable)
			std::cout << "satisfiable";
		else
			std::cout << "not satisfiable";
		if (solver_Optimizer->using_Optimize)
			std::cout << ": " << result.score;
		std::cout << std::endl;
	}
	if (verbose >= 0) {
		if (result.satisfiable)
			std::cout << result.formula << std::endl;
	}
	return result;

}

void Formula::prepare_New_Iteration()
{
	iteration++;
	add_Variables();

	dag->add_Formulas(iteration);
	positive_Sample->add_Formulas(iteration);
	negative_Sample->add_Formulas(iteration);

	if(using_Grammar) context_Free_Grammar->add_Formulas(iteration);
}

Node* Formula::build_Tree(z3::model &model, int root, Node** node_Vector, std::vector<bool>& touched){
	Node *root_Node = (Node*)std::malloc(sizeof(Node));
	if (root == 0) {
		root_Node->left = nullptr;
		root_Node->right = nullptr;
	}
	else {
		for (int i = 0; i < root; i++) {
			if (model.eval(dag->variables_left_i_j[root][i]).is_true()) {
				if (!touched[i]) {
					root_Node->left = build_Tree(model, i, node_Vector, touched);
					break;
				}
				else {
					root_Node->left = node_Vector[i];
					break;
				}
			}
		}
		for (int i = 0; i < root; i++) {
			if (model.eval(dag->variables_right_i_j[root][i]).is_true()) {
				if (!touched[i]) {
					root_Node->right= build_Tree(model, i, node_Vector, touched);
					break;
				}
				else {
					root_Node->right = node_Vector[i];
					break;
				}
			}
		}
	}
	for (int i = 0; i < (number_Of_Variables + 8); i++ ) {
		if (model.eval(dag->variables_x_lambda_i[i][root]).is_true()) {
			root_Node->formula = i;
			break;
		}
	}

	touched[root] = true;
	node_Vector[root] = root_Node;
	return root_Node;
}

Formula::Formula()
{
}

Solver_Result Formula::find_LTL()
{
	Solver_Result result;

	// is decreased one already since the first iteration has index 0
	optimized_Run--;



	result = solve_Iteration();

	// as long as no result was found prepare a new iteration and use sat solver
	while (!result.satisfiable) {
		optimized_Run--;
		prepare_New_Iteration();


		result = solve_Iteration();

	}	

	finish = clock();
	own_Execution_Time += (finish - start) / (double)CLOCKS_PER_SEC;
	result.time = own_Execution_Time + z3_Time;

	if (verbose >= 1) std::cout << "\nOwn execution time: " << own_Execution_Time << std::endl;
	if (verbose >= 1) std::cout << "Z3 execution time: " << z3_Time << std::endl;
	if (verbose >= 1) std::cout << "total execution time: " << own_Execution_Time + z3_Time << std::endl;

	return result;
}

std::string Formula::print_Tree(Node *root)
{
	std::stringstream result;

	if (root->formula == number_Of_Variables) {
		result << "!";
		result << print_Tree(root->left);
	}else if(root->formula == number_Of_Variables +1) {
		result << "(";
		result << print_Tree(root->left);
		result << "||";
		result << print_Tree(root->right);
		result << ")";
	}else if (root->formula == number_Of_Variables + 2) {
		result << "(";
		result << print_Tree(root->left);
		result << "&&";
		result << print_Tree(root->right);
		result << ")";
	}else if (root->formula == number_Of_Variables + 3) {
		result << "(";
		result << print_Tree(root->left);
		result << "=>";
		result << print_Tree(root->right);
		result << ")";
	}else if (root->formula == number_Of_Variables + 4) {
		result << "X";
		result << print_Tree(root->left);
	}else if (root->formula == number_Of_Variables + 5) {
		result << "F";
		result << print_Tree(root->left);
	}else if (root->formula == number_Of_Variables + 6) {
		result << "G";
		result << print_Tree(root->left);
	}else if (root->formula == number_Of_Variables + 7) {
		result << "(";
		result << print_Tree(root->left);
		result << ")U(";
		result << print_Tree(root->right);
		result << ")";
	}

	return result.str();
}

void Formula::add_Formulas(Solve_And_Optimize& solver_Optimizer)
{
	if (verbose >= 2) std::cout << "----------------------------------------" << std::endl;
	if (verbose >= 2) std::cout << "Solve new Iteration with number: " << (iteration + 1) << std::endl;


	for (const z3::expr_vector& list : dag->formulas_Dag) {
		for (const z3::expr& formula : list) {
			solver_Optimizer.add(formula);
		}
	}


	for (const z3::expr_vector& parents : dag->formulas_Parent_Exists) {
		solver_Optimizer.add(z3::atleast(parents, 1));
	}

	for (const z3::expr& formula : positive_Sample->all_Formulas) {
		solver_Optimizer.add(formula);
	}

	for (const z3::expr& formula : negative_Sample->all_Formulas) {
		solver_Optimizer.add(formula);
	}

	if (using_Grammar) {
		for (const z3::expr& expr : context_Free_Grammar->formulas_Grammar) {
			solver_Optimizer.add(expr);
		}
		solver_Optimizer.add(context_Free_Grammar->make_Start(iteration));
	}

	if (solver_Optimizer.using_Optimize) {
		int pos_weights = 0;
		for (Trace_Metadata word : positive_Sample->sample_Metadatas) pos_weights += word.weight;
		int neg_weights = 0;
		for (Trace_Metadata word : negative_Sample->sample_Metadatas) neg_weights += word.weight;
		if (score==Score::Count || score==Score::Ratio) {
			int pos_scale, neg_scale;
			switch (score) {
				case Score::Count: pos_scale=1; neg_scale=1; break;
				case Score::Ratio: pos_scale=neg_weights; neg_scale=pos_weights; break;
			}

			for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
				solver_Optimizer.add(positive_Sample->variables_Y_Word_i_t[i][iteration][0], positive_Sample->sample_Metadatas[i].weight * pos_scale);
			}

			for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
				solver_Optimizer.add(!negative_Sample->variables_Y_Word_i_t[i][iteration][0], negative_Sample->sample_Metadatas[i].weight * neg_scale);
			}
		} else if (score==Score::Linear || score==Score::Quadra) {
			z3::expr_vector positive_Sample_Classified(context);
			for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
				positive_Sample_Classified.push_back(z3::ite(positive_Sample->variables_Y_Word_i_t[i][iteration][0],
					context.int_val(positive_Sample->sample_Metadatas[i].weight),
					context.int_val(0)
				));
			}
			z3::expr pos_s_c_size = z3::sum(positive_Sample_Classified);

			z3::expr_vector negative_Sample_Classified(context);
			for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
				negative_Sample_Classified.push_back(z3::ite(negative_Sample->variables_Y_Word_i_t[i][iteration][0],
					context.int_val(0),
					context.int_val(negative_Sample->sample_Metadatas[i].weight)
				));
			}
			z3::expr neg_s_c_size = z3::sum(negative_Sample_Classified);

			switch (score) {
				case Score::Linear: solver_Optimizer.add_maximize(pos_s_c_size*neg_weights + neg_s_c_size*pos_weights); break;
				case Score::Quadra: solver_Optimizer.add_maximize(pos_s_c_size * neg_s_c_size); break;
			}
		}
	}
	else {

		for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
			solver_Optimizer.add(positive_Sample->variables_Y_Word_i_t[i][iteration][0]);
		}

		for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
			solver_Optimizer.add(!negative_Sample->variables_Y_Word_i_t[i][iteration][0]);
		}
	}


	finish = clock();
	own_Execution_Time += (finish - start) / (double)CLOCKS_PER_SEC;
	start = clock();

	if (solver_Optimizer.using_Optimize) {

		if (verbose >= 2) std::cout << "finished making optimizer" << std::endl;
	}
	else {

		if (verbose >= 2) std::cout << "finished making solver" << std::endl;
	}
}

void Formula::make_Result(Solve_And_Optimize& solver_Optimizer, Solver_Result& result)
{
	z3::model model = solver_Optimizer.get_model();
	std::vector<bool> touched;
	for (int i = 0; i <= iteration; i++) {
		touched.push_back(false);
	}
	Node** node_Vector;
	node_Vector = (Node**)malloc((iteration + 1) * sizeof(Node*));
	Node* root = build_Tree(model, iteration, node_Vector, touched);
	result.formula = print_Tree(root);
	result.size = (iteration + 1);
	for (int i = 0; i <= iteration; i++) {
		delete node_Vector[i];
	}
	delete[] node_Vector;
	result.satisfiable = true;

	int pos_weights = 0;
	int pos_score = 0;
	for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
		pos_weights += positive_Sample->sample_Metadatas[i].weight;
		if (model.eval(positive_Sample->variables_Y_Word_i_t[i][iteration][0]).is_true()) {
			pos_score += positive_Sample->sample_Metadatas[i].weight;
		} else {
			result.false_negative.push_back(i);
		}
	}

	int neg_weights = 0;
	int neg_score = 0;
	for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
		neg_weights += negative_Sample->sample_Metadatas[i].weight;
		if (model.eval(negative_Sample->variables_Y_Word_i_t[i][iteration][0]).is_false()) {
			neg_score += negative_Sample->sample_Metadatas[i].weight;
		} else {
			result.false_positive.push_back(i);
		}
	}

	switch (score) {
		case Score::Count:
			result.score = (double)(pos_score+neg_score) / (double)(pos_weights+neg_weights); break;
		case Score::Ratio:
		case Score::Linear:
			result.score = 0.5l*pos_score/pos_weights + 0.5l*neg_score/neg_weights; break;
		case Score::Quadra:
			result.score = (double)(pos_score*neg_score) / (double)(pos_weights*neg_weights); break;
	}
}

void Formula::set_Grammar(std::vector<std::string>& grammar)
{

	context_Free_Grammar = std::unique_ptr<Grammar>(new Grammar(context, *dag.get(), number_Of_Variables, grammar));

	using_Grammar = true;
}

void Formula::initialize()
{
	dag->initialize();
	positive_Sample->initialize();
	negative_Sample->initialize();
	if (using_Grammar) context_Free_Grammar->initialize();

}

void Formula::set_Using_Incremental()
{
	using_Incremental = true;
	solver = new z3::solver(context);
	if (using_Grammar) context_Free_Grammar->set_Incremental(solver);
	positive_Sample->set_Incremental(solver);
	negative_Sample->set_Incremental(solver);
	dag->set_Incremental(solver);
}

Formula::~Formula()
{
	delete solver;
}
