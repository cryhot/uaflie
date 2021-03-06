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
#include "Header/Utils.h"
#include <iostream>


void Formula::add_Variables(){

	dag->add_Variables(iteration);

	positive_Sample->add_Variables(iteration);

	negative_Sample->add_Variables(iteration);

	if (using_Grammar) context_Free_Grammar->add_Variables(iteration);
}

std::pair<bool, std::string> Formula::solve_Iteration_Incrementally(){

	if (optimized_Run == 0) {
		//TODO
	}

	Sat_Solver solver_Optimizer = Sat_Solver(*solver);

	solver->push();

	// Only these Y Variables will be removed from the solver after the iteration
	for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
		solver_Optimizer.add(positive_Sample->variables_Y_Word_i_t_all[i][iteration][0] > 0);
	}
	for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
		solver_Optimizer.add(-negative_Sample->variables_Y_Word_i_t_any[i][iteration][0] > 0);
	}



	std::pair<bool, std::string> result = std::make_pair(true, "");
	if (solver->check() == z3::sat) {
		finish = clock();
		z3_Time += (finish - start) /(double) CLOCKS_PER_SEC;
		start = clock();
		if(verbose >= 2) std::cout << "satisfiable" << std::endl;
		z3::model model = solver->get_model();
		make_Result(model, result);
	}
	else {

		solver->pop();
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();

		if(verbose >= 2)std::cout << "not satisfiable" << std::endl;
	}

	return result;
}

std::pair<bool, std::string> Formula::solve_Iteration()
{
	if (using_Incremental) {
		return solve_Iteration_Incrementally();
	}

	if (optimized_Run == 0) {
		return solve_Iteration_Optimize();
	}

	z3::optimize optimize(context);
	Max_Sat_Solver solver_Optimizer = Max_Sat_Solver(optimize);
	solver_Optimizer.using_Optimize = false;

	add_Formulas(solver_Optimizer);

	std::pair<bool, std::string> result = std::make_pair(true, "");
	if (optimize.check() == z3::sat) {
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();
		z3::model model = optimize.get_model();
		double robustness;
		model.eval(*objective).is_numeral(robustness);
		if (verbose >= 2) std::cout << "satisfiable (robustness=" << robustness << ")" << std::endl;
		make_Result(model, result);
	}
	else {
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();

		if (verbose >= 2) std::cout << "not satisfiable" << std::endl;
	}
	return result;
	
}

std::pair<bool, std::string> Formula::solve_Iteration_Optimize() {

	z3::optimize optimize(context);
	Max_Sat_Solver solver_Optimizer = Max_Sat_Solver(optimize);

	add_Formulas(solver_Optimizer);

	std::pair<bool, std::string> result = std::make_pair(true, "");
	if (optimize.check() == z3::sat) {
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();
		z3::model model = optimize.get_model();
		double robustness;
		model.eval(*objective).is_numeral(robustness);
		if (verbose >= 2) std::cout << "satisfiable (robustness=" << robustness << ")" << std::endl;
		make_Result(model, result);
	}
	else {
		finish = clock();
		z3_Time += (finish - start) / (double)CLOCKS_PER_SEC;
		start = clock();

		if (verbose >= 2) std::cout << "not satisfiable" << std::endl;
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
	for (int p = 0; p < 2; p++) {
		z3::expr param = model.eval(dag->variables_parameter_i_p[root][p]);
		if (param.is_numeral())
			root_Node->int_parameter[p] = param.get_numeral_int();
		else
			root_Node->int_parameter[p] = -1;
	}

	touched[root] = true;
	node_Vector[root] = root_Node;
	return root_Node;
}

Formula::Formula()
{
}

std::string Formula::find_LTL()
{
	// the result contains a bool stating wether the iteration was satisfiable and a string containing the representation of the correct formula
	std::pair<bool, std::string> result;

	// is decreased one already since the first iteration has index 0
	optimized_Run--; 



	result = solve_Iteration();

	// as long as no result was found prepare a new iteration and use sat solver
	while (result.first) {
		optimized_Run--;
		prepare_New_Iteration();


		result = solve_Iteration();

	}	

	finish = clock();
	own_Execution_Time += (finish - start) / (double)CLOCKS_PER_SEC;


	if (verbose >= 1) std::cout << "\nOwn execution time: " << own_Execution_Time << std::endl;
	if (verbose >= 1) std::cout << "Z3 execution time: " << z3_Time << std::endl;
	if (verbose >= 1) std::cout << "total execution time: " << own_Execution_Time + z3_Time << std::endl;

	return result.second;	
}

void Formula::print_bounds(std::ostream& stream, int a, int b) {
	int lowerbound = a;
	int upperbound = a + max_Word_Size - b;
	if (a != 0 || b != 0) {
		stream << "[";
		stream << lowerbound;
		stream << ",";
		if (b > 0) {
			stream << upperbound;
		} else {
			stream << "inf";
		}
		stream << ")";
	}
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
		print_bounds(result, root->int_parameter[0], root->int_parameter[1]);
		result << print_Tree(root->left);
	}else if (root->formula == number_Of_Variables + 6) {
		result << "G";
		print_bounds(result, root->int_parameter[0], root->int_parameter[1]);
		result << print_Tree(root->left);
	}else if (root->formula == number_Of_Variables + 7) {
		result << "(";
		result << print_Tree(root->left);
		result << ")U";
		print_bounds(result, root->int_parameter[0], root->int_parameter[1]);
		result << "(";
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
	}
	else {

		for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
			solver_Optimizer.add(positive_Sample->variables_Y_Word_i_t_all[i][iteration][0] > 0);
		}

		for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
			solver_Optimizer.add(-negative_Sample->variables_Y_Word_i_t_any[i][iteration][0] > 0);
		}
	}

	z3::expr_vector objectives(context);
	for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
		objectives.push_back(positive_Sample->variables_Y_Word_i_t_all[i][iteration][0]);
	}

	for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
		objectives.push_back(-negative_Sample->variables_Y_Word_i_t_any[i][iteration][0]);
	}
	objective = std::unique_ptr<z3::expr>(new z3::expr(context));
	*objective = mk_chain(&z3::min, objectives, objectives[0]);
	solver_Optimizer.add_maximize(*objective);

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

void Formula::make_Result(z3::model& model, std::pair<bool, std::string>& result)
{
	std::vector<bool> touched;
	for (int i = 0; i <= iteration; i++) {
		touched.push_back(false);
	}
	Node** node_Vector;
	node_Vector = (Node**)malloc((iteration + 1) * sizeof(Node*));
	Node* root = build_Tree(model, iteration, node_Vector, touched);
	result.second = print_Tree(root);
	for (int i = 0; i <= iteration; i++) {
		delete node_Vector[i];
	}
	delete[] node_Vector;
	result.first = false;

	if (verbose >= 2) {
		double robustness;
		double robustness2;
		std::cout << "----------------------------------------\n";
		std::cout << "Individual robustness\n";
		std::cout << "---\n";
		for (unsigned int i = 0; i < positive_Sample->sample_Metadatas.size(); i++) {
			model.eval(positive_Sample->variables_Y_Word_i_t_all[i][iteration][0]).is_numeral(robustness);
			model.eval(positive_Sample->variables_Y_Word_i_t_any[i][iteration][0]).is_numeral(robustness2);
			std::cout << "min=" << robustness << " max=" << robustness2 << "\n";
		}
		std::cout << "---\n";
		for (unsigned int i = 0; i < negative_Sample->sample_Metadatas.size(); i++) {
			model.eval(-negative_Sample->variables_Y_Word_i_t_any[i][iteration][0]).is_numeral(robustness);
			model.eval(-negative_Sample->variables_Y_Word_i_t_all[i][iteration][0]).is_numeral(robustness2);
			std::cout << "min=" << robustness << " max=" << robustness2 << "\n";
		}
		std::cout << "----------------------------------------\n";
	}

	std::cout << result.second << std::endl;
}

void Formula::set_Grammar(std::vector<std::string>& grammar)
{

	context_Free_Grammar = std::unique_ptr<Grammar>(new Grammar(context, *dag.get(), number_Of_Variables, grammar));

	using_Grammar = true;
}

static int gcd(int a, int b)
{
	if (b == 0) return a;
    return gcd(b, a % b);
}

static int lcm(int a, int b)
{
	return (a*b) / gcd(a, b);
}

void Formula::initialize()
{
	max_Word_Size = 0;
	int max_Word_Prefix = 0;
	int word_Period_LCM = 1;
	for (Trace_Metadata word : positive_Sample->sample_Metadatas) {
		max_Word_Size = std::max(max_Word_Size, word.size);
		max_Word_Prefix = std::max(max_Word_Prefix, word.repetition);
		word_Period_LCM = lcm(word_Period_LCM, word.size-word.repetition);
	}
	for (Trace_Metadata word : negative_Sample->sample_Metadatas) {
		max_Word_Size = std::max(max_Word_Size, word.size);
		max_Word_Prefix = std::max(max_Word_Prefix, word.repetition);
		word_Period_LCM = lcm(word_Period_LCM, word.size-word.repetition);
	}
	max_Word_Period = max_Word_Prefix + word_Period_LCM;
	positive_Sample->max_Word_Size = max_Word_Size;
	negative_Sample->max_Word_Size = max_Word_Size;
	positive_Sample->max_Word_Period = max_Word_Period;
	negative_Sample->max_Word_Period = max_Word_Period;

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


