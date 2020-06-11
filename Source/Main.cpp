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



#include <vector>
#include <fstream>
#include <string.h>
#include "Header/Formula.h"
#include "Header/Formula_LTL.h"
#include "Header/Formula_SLTL.h"

/*
Splits an LTL file into vectors of strings containing the different sections of the file:
	0 all words which should be satisfied
	1 all words which should not be satisfied
	2 a single string containing all usable operators
	3 a single string containing the maximal depth
	4 the grammar if one is used

	input: the path to the input file
*/
std::vector<std::vector<std::string>> split_Input_LTL(char* input) {

	std::ifstream input_File(input);
	std::string line;
	std::vector<std::vector<std::string>> result;


	//read in file

	std::vector<std::string> positive_Sample_Input;
	std::vector<std::string> negative_Sample_Input;
	std::vector<std::string> usable_Operators;
	std::vector<std::string> depth;
	std::vector<std::string> grammar;


	if (input_File.is_open()) {

		//get positive sample

		while (std::getline(input_File, line) && line.compare("---")) {
			positive_Sample_Input.push_back(line);
		}

		//get negative sample

		while (std::getline(input_File, line) && line.compare("---")) {
			negative_Sample_Input.push_back(line);
		}

		//get set of usable formulas

		std::getline(input_File, line);

		std::getline(input_File, line); // skipping next ---

		//get depth

		std::getline(input_File, line);
		depth.push_back(line);

		std::getline(input_File, line); // skipping next ---

		//get grammar if possible
		
		while (std::getline(input_File, line) && line.compare("---")) {
			grammar.push_back(line);
		}

		input_File.close();

		result.push_back(positive_Sample_Input);
		result.push_back(negative_Sample_Input);
		result.push_back(usable_Operators);
		result.push_back(depth);
		result.push_back(grammar);
		

	}
	else {
		std::cout << input << std::endl;
		std::cout << "unable to open file" << std::endl;
	}

	return result;
}

/*
Splits an SLTL file into vectors of strings containing the different sections of the file:
	0 all words which should be satisfied
	1 all words which should not be satisfied
	2 a single string containing all usable operators
	3 a single string containing the maximal depth
	4 the terms which can be used 
	5 the grammar if one is used

	input: the path to the input file
*/
std::vector<std::vector<std::string>> split_Input_SLTL(char* input) {

	std::ifstream input_File(input);
	std::string line;
	std::vector<std::vector<std::string>> result;

	//read in file

	std::vector<std::string> positive_Sample_Input;
	std::vector<std::string> negative_Sample_Input;
	std::vector<std::string> usable_Operators;
	std::vector<std::string> depth;
	std::vector<std::string> input_Terms;
	std::vector<std::string> grammar;

	if (input_File.is_open()) {

		//get positive sample

		while (std::getline(input_File, line) && line.compare("---")) {
			positive_Sample_Input.push_back(line);
		}

		//get negative sample

		while (std::getline(input_File, line) && line.compare("---")) {
			negative_Sample_Input.push_back(line);
		}

		//get set of usable formulas

		std::getline(input_File, line);

		std::getline(input_File, line); // skipping next ---

		//get depth

		std::getline(input_File, line);
		depth.push_back(line);

		std::getline(input_File, line); // skipping next ---

		//get set of usable formulas
		while (std::getline(input_File, line) && line.compare("---")) {
			input_Terms.push_back(line);
		}

		//get grammar if possible

		while (std::getline(input_File, line) && line.compare("---")) {
			grammar.push_back(line);
		}

		input_File.close();

		result.push_back(positive_Sample_Input);
		result.push_back(negative_Sample_Input);
		result.push_back(usable_Operators);
		result.push_back(depth);
		result.push_back(input_Terms);
		result.push_back(grammar);
	}
	else {
		std::cout << input << std::endl;
		std::cout << "unable to open file";
	}

	return result;
}

struct Dataset_Result
{
	int positive_size;
	int negative_size;
	bool success;
	std::string formula;
	std::string ansi_formula;
	std::vector<std::string> tree_formulas;
	int node_count;
	int depth;
	double time;
};
#define ANSI_FORMULA(s) "\e[36m" s "\e[m"

/*
Executes the algorithm for given parameters for a single input.

	using_Grammar: is true if a grammar should be used in all files
	using_Incremental: is true if an incremental solver should be used in all files
	using_SLTL: is true if all files are SLTL files
	optimized_Run: gives the iteration in which max sat is used instead of sat
	input_Split: the different sections of the file
*/
Dataset_Result solve_Single_Dataset(bool using_Grammar, bool using_Incremental, bool using_SLTL, int optimized_Run, std::vector<std::vector<std::string>> &input_Split, int verbose) {
	Formula* formula;
	int grammar_Index;

	std::vector<std::string> positive_Sample_String = input_Split[0];
	std::vector<std::string> negative_Sample_String = input_Split[1];

	if (!using_SLTL) {
		formula = new Formula_LTL(positive_Sample_String, negative_Sample_String);
		grammar_Index = 4;
	} else {
		formula = new Formula_SLTL(positive_Sample_String, negative_Sample_String, input_Split[4]);
		grammar_Index = 5;
	}

	if (using_Grammar) {
		formula->set_Grammar(input_Split[grammar_Index]);
	}

	if (optimized_Run > 0) {
		formula->set_Optimized_Run(optimized_Run);
	}

	if (using_Incremental) {
		formula->set_Using_Incremental();
	}
	formula->set_Vebose(verbose);
	formula->initialize();
	Solver_Result result = formula->find_LTL();
	if (verbose >= 1) std::cout << result.formula << std::endl;

	Dataset_Result nodeResult;
	nodeResult.positive_size = positive_Sample_String.size();
	nodeResult.negative_size = negative_Sample_String.size();
	int positive_dataset_size = positive_Sample_String.size()-result.false_negative.size()+result.false_positive.size();
	int negative_dataset_size = negative_Sample_String.size()-result.false_positive.size()+result.false_negative.size();
	nodeResult.success = positive_dataset_size>0 && negative_dataset_size>0;
	if (nodeResult.success && (result.false_negative.size()==positive_Sample_String.size() || result.false_positive.size()==negative_Sample_String.size())) {
		// swap samples
		unsigned int j;
		std::vector<unsigned int> false_negative;
		j = 0;
		for (unsigned int i = 0; i < positive_Sample_String.size(); i++) {
			if (j < result.false_negative.size() && i == result.false_negative[j]) j++;
			else false_negative.push_back(i);
		}
		std::vector<unsigned int> false_positive;
		j = 0;
		for (unsigned int i = 0; i < negative_Sample_String.size(); i++) {
			if (j < result.false_positive.size() && i == result.false_positive[j]) j++;
			else false_positive.push_back(i);
		}
		result.formula = "!"+result.formula;
		result.false_negative = false_negative;
		result.false_positive = false_positive;
		positive_dataset_size = positive_Sample_String.size()-result.false_negative.size()+result.false_positive.size();
		negative_dataset_size = negative_Sample_String.size()-result.false_positive.size()+result.false_negative.size();
	}

	nodeResult.formula = result.formula;
	if (!nodeResult.success && verbose >= 1)
		nodeResult.ansi_formula = "\e[33m"+result.formula+"\e[m";
	else
		nodeResult.ansi_formula = ""+result.formula+"";
	nodeResult.tree_formulas.push_back(result.formula);
	nodeResult.node_count = 1;
	nodeResult.depth = 1;

	/* print current traces */
	if (verbose >= 3) {
		unsigned int j;
		std::cout << "----------------------------------------" << std::endl;
		j = 0;
		for (unsigned int i = 0; i < positive_Sample_String.size(); i++) {
			std::cout << "\e[01;32m + \e[m";
			if (j < result.false_negative.size() && i == result.false_negative[j]) {
				std::cout << "\e[31m";
				j++;
			} else {
				std::cout << "\e[32m";
			}
			std::cout << positive_Sample_String[i] << "\e[m" << std::endl;
		}
		j = 0;
		for (unsigned int i = 0; i < negative_Sample_String.size(); i++) {
			std::cout << "\e[01;31m - \e[m";
			if (j < result.false_positive.size() && i == result.false_positive[j]) {
				std::cout << "\e[32m";
				j++;
			} else {
				std::cout << "\e[31m";
			}
			std::cout << negative_Sample_String[i] << "\e[m" << std::endl;
		}
	}

	/* sub-branches */

	if (verbose >= 1) {
		// std::cout << "\nPOSITIVE CHILD:" << std::endl;
		std::cout << "\n\e[" << (nodeResult.success?47:43) << ";32m POSITIVE CHILD \e[m";
		std::cout << " (" << result.false_positive.size() << "/" << positive_dataset_size << " misclassified)";
		std::cout << " max=" << optimized_Run;
		std::cout << ":" << std::endl;
	}
	if (result.false_positive.empty()) {
		// positive well sorted
		nodeResult.tree_formulas.push_back("⊤");
		if (verbose >= 1) std::cout << "⊤" << std::endl;
	} else if (!nodeResult.success) {
		nodeResult.tree_formulas.push_back("...");
		if (verbose >= 1) std::cout << "..." << std::endl;
	} else {
		std::vector<std::string> next_negative_Sample_String; // all negative words classified as positive
		for (unsigned int i : result.false_positive) {
			next_negative_Sample_String.push_back(negative_Sample_String[i]);
		}
		std::vector<std::string> next_positive_Sample_String; // all positive words classified as positive
		unsigned int j = 0;
		for (unsigned int i = 0; i < positive_Sample_String.size(); i++) {
			if (j < result.false_negative.size() && i == result.false_negative[j]) j++;
			else next_positive_Sample_String.push_back(positive_Sample_String[i]);
		}
		input_Split[0] = next_positive_Sample_String;
		input_Split[1] = next_negative_Sample_String;
		Dataset_Result posResult = solve_Single_Dataset(using_Grammar, using_Incremental, using_SLTL, optimized_Run, input_Split, verbose);

		nodeResult.formula = "("+result.formula+"&&"+posResult.formula+")";
		nodeResult.ansi_formula = ANSI_FORMULA("(")+result.formula+ANSI_FORMULA("&&")+posResult.ansi_formula+ANSI_FORMULA(")");
		for (std::string node_formula : posResult.tree_formulas) nodeResult.tree_formulas.push_back(node_formula);
		nodeResult.success &= posResult.success;
		nodeResult.node_count += posResult.node_count;
		if (nodeResult.depth < posResult.depth+1) nodeResult.depth = posResult.depth+1;
		if (!nodeResult.success) return nodeResult;
	}

	if (verbose >= 1) {
		// std::cout << "\nNEGATIVE CHILD:" << std::endl;
		std::cout << "\n\e[" << (nodeResult.success?47:43) << ";31m NEGATIVE CHILD \e[m";
		std::cout << " (" << result.false_negative.size() << "/" << negative_dataset_size << " misclassified)";
		std::cout << " max=" << optimized_Run;
		std::cout << ":" << std::endl;
	}
	if (result.false_negative.empty()) {
		// negative well sorted
		nodeResult.tree_formulas.push_back("⊥");
		if (verbose >= 1) std::cout << "⊥" << std::endl;
	} else if (!nodeResult.success) {
		nodeResult.tree_formulas.push_back("...");
		if (verbose >= 1) std::cout << "..." << std::endl;
	} else {
		std::vector<std::string> next_positive_Sample_String; // all positive words classified as negative
		for (unsigned int i : result.false_negative) {
			next_positive_Sample_String.push_back(positive_Sample_String[i]);
		}
		std::vector<std::string> next_negative_Sample_String; // all negative words classified as negative
		unsigned int j = 0;
		for (unsigned int i = 0; i < negative_Sample_String.size(); i++) {
			if (j < result.false_positive.size() && i == result.false_positive[j]) j++;
			else next_negative_Sample_String.push_back(negative_Sample_String[i]);
		}
		input_Split[0] = next_positive_Sample_String;
		input_Split[1] = next_negative_Sample_String;
		Dataset_Result negResult = solve_Single_Dataset(using_Grammar, using_Incremental, using_SLTL, optimized_Run, input_Split, verbose);

		nodeResult.formula = "("+nodeResult.formula+"||(!"+result.formula+"&&"+negResult.formula+"))";
		nodeResult.ansi_formula = ANSI_FORMULA("(")+nodeResult.ansi_formula+ANSI_FORMULA("||(!")+result.formula+ANSI_FORMULA("&&")+negResult.ansi_formula+ANSI_FORMULA("))");
		for (std::string node_formula : negResult.tree_formulas) nodeResult.tree_formulas.push_back(node_formula);
		nodeResult.success &= negResult.success;
		nodeResult.node_count += negResult.node_count;
		if (nodeResult.depth < negResult.depth+1) nodeResult.depth = negResult.depth+1;
		if (!nodeResult.success) return nodeResult;
	}

	input_Split[0] = positive_Sample_String;
	input_Split[1] = negative_Sample_String;
	return nodeResult;
}

/*
Executes the algorithm for given parameters for a single input file.

	using_Grammar: is true if a grammar should be used in all files
	using_Incremental: is true if an incremental solver should be used in all files
	using_SLTL: is true if all files are SLTL files
	optimized_Run: gives the iteration in which max sat is used instead of sat
	input: the paths to the input file
*/
Dataset_Result solve_Single_File(bool using_Grammar, bool using_Incremental, bool using_SLTL, int optimized_Run, char * input, int verbose)
{
	std::vector<std::vector<std::string>> input_Split;

	Dataset_Result failResult;
	failResult.success = false;
	failResult.formula = "";
	failResult.ansi_formula = "";
	failResult.node_count = 0;
	failResult.depth = 0;
	failResult.time = 0;

	if (!using_SLTL) {
		input_Split = split_Input_LTL(input);
	}
	else {
		input_Split = split_Input_SLTL(input);
	}
	if (input_Split.size() < 1) return failResult;

	clock_t start = clock();

	if (verbose >= 1) {
		std::cout << "\n\e[47;33m ROOT NODE \e[m";
		std::cout << " (" << input_Split[0].size() << "/" << input_Split[0].size()+input_Split[1].size() << " positive)";
		std::cout << " max=" << optimized_Run;
		std::cout << ":" << std::endl;
	}
	Dataset_Result rootResult = solve_Single_Dataset(using_Grammar, using_Incremental, using_SLTL, optimized_Run, input_Split, verbose);

	clock_t end = clock();
	rootResult.time = (end - start) / (double)CLOCKS_PER_SEC;

	if (verbose >= 1) {
		std::cout << "========================================" << std::endl;
		std::cout << (rootResult.success?"success":"failure") << std::endl;
		std::cout << rootResult.ansi_formula << std::endl;
		std::cout << "total execution time: " << rootResult.time << std::endl;
		std::cout << "node count: " << rootResult.node_count << std::endl;
		std::cout << "depth: " << rootResult.depth << std::endl;
	} else {
		if (rootResult.success)
			std::cout << rootResult.formula << std::endl;
		else
			std::cout << "failure" << std::endl;
	}

	return rootResult;
}

/*
Executes the algorithm for given parameters for a set of inputs.

	using_Grammar: is true if a grammar should be used in all files
	using_Incremental: is true if an incremental solver should be used in all files
	using_SLTL: is true if all files are SLTL files
	optimized_Run: gives the iteration in which max sat is used instead of sat
	input_Files: a vector of paths to the input files
*/
int solve_Multiple_Files(bool using_Grammar, bool using_Incremental, bool using_SLTL, int optimized_Run, std::vector<char*> input_Files, int verbose) {

	std::ofstream csv_result;
	csv_result.open("results.csv");

	csv_result << "File Name";
	csv_result << ",Traces";
	csv_result << ",Positive Traces";
	csv_result << ",Negative Traces";
	csv_result << ",Method";
	csv_result << ",Success";
	csv_result << ",Time passed";
	csv_result << ",Formula";
	csv_result << ",Tree Nodes";
	csv_result << ",Tree Depth";
	csv_result << ",Formula Tree";
	csv_result << "\n";
	csv_result.flush();

	int status = 0;
	for (char* file : input_Files) {

		/* "File Name" */
		csv_result << file;
		csv_result.flush();

		if (input_Files.size() > 1) std::cout << "\nSOLVING " << file << std::endl;

		Dataset_Result result = solve_Single_File(using_Grammar, using_Incremental, using_SLTL, optimized_Run, file, verbose);
		if (!result.success) status = 1;

		/* "Traces" */
		csv_result << "," << result.positive_size+result.negative_size;
		/* "Positive Traces" */
		csv_result << "," << result.positive_size;
		/* "Negative Traces" */
		csv_result << "," << result.negative_size;
		/* "Method" */
		csv_result << "," << "DecisionTree[max=" << optimized_Run << "]";
		/* "Success" */
		csv_result << "," << result.success;
		/* "Time passed" */
		csv_result << "," << result.time;
		/* "Formula" */
		csv_result << "," << result.formula;
		/* "Tree Nodes" */
		csv_result << "," << result.node_count;
		/* "Tree Depth" */
		csv_result << "," << result.depth;
		/* "Formula Tree" */
		csv_result << ",";
		std::string last_formula = result.tree_formulas.back();
		result.tree_formulas.pop_back();
		for (std::string formula : result.tree_formulas) csv_result << formula << ";";
		csv_result << last_formula ;
		result.tree_formulas.push_back(last_formula);

		csv_result << "\n";
		csv_result.flush();
	}
	csv_result.close();

	return status;
}

void print_Help() {
	std::cout << "Command Line Arguments:\n " << std::endl;
	std::cout << "-f <path>:	Specifies the path to a single trace file which should be examined.\n" << std::endl;
	std::cout << "-max <iteration>:	Specifies the iteration in which MAX-SAT solver is used instead of a SAT Solver. If this argument is not used MAX-SAT will not be used.\n" << std::endl;
	std::cout << "-i:	Specifies whether an incremental solver should be used.\n" << std::endl;
	std::cout << "-g:	Specifies whether a grammar is used to limit the output formulas.\n" << std::endl;
	std::cout << "-sltl:	Specifies whether the input file is a SLTL example.\n" << std::endl;
	std::cout << "Example Files can be found in the Trace directory and in the README file.\n" << std::endl;
}

int main(int argc, char* argv[]) {

	// Input Parameters:

	bool using_Grammar = false;
	bool using_Incremental = false;
	bool using_SLTL = false;
	int verbose = 0;
	int optimized_Run = 0;
	std::vector<char*> input_Files;


	std::vector<std::string> strings;

	// setting the input parameters

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-g")) using_Grammar = true;
		if (!strcmp(argv[i], "-i")) using_Incremental = true;
		if (!strcmp(argv[i], "-f")&& (i + 1) < argc) input_Files.push_back(argv[i + 1]);
		if (!strcmp(argv[i], "-max") && (i + 1) < argc) optimized_Run = std::stoi(argv[i + 1]);
		if (!strcmp(argv[i], "-sltl")) using_SLTL = true;
		if (!strcmp(argv[i], "-range") && (i + 2) < argc) {

			int initial_Number = std::stoi(argv[i + 1]);
			int last_Number = std::stoi(argv[i + 2]);

			for (int j = initial_Number; j <= last_Number; j++) {

				std::stringstream ss;
				if (j < 10) {
					ss << "traces/000";
				}
				else if (j < 100) {
					ss << "traces/00";
				}
				else if (j < 1000) {
					ss << "traces/0";
				}
				else {
					ss << "traces/";
				}
				ss << j;
				ss << ".trace";
				strings.push_back(ss.str());
				char* input_File = const_cast<char*> (strings.back().c_str());
				std::ifstream infile(input_File);
				if (infile.is_open()) {
					input_Files.push_back(input_File);
				}
				infile.close();
			}
		}
		if (!strcmp(argv[i], "-h") || argc  == 1) {
			print_Help();
			return 0;
		}
		if (!strcmp(argv[i], "-v")) verbose = 1;
		if (!strcmp(argv[i], "-vv")) verbose = 2;
		if (!strcmp(argv[i], "-vvv")) verbose = 3;
		if (!strcmp(argv[i], "-vvvv")) verbose = 4;
		if (!strcmp(argv[i], "-vvvvv")) verbose = 5;
	}


	if (input_Files.size() > 0) {

		// execute for all input files

		return solve_Multiple_Files(using_Grammar, using_Incremental, using_SLTL, optimized_Run, input_Files, verbose);
	}
	else {
		std::cout << "No input File\n" << std::endl;
		print_Help();
	}

	return 0;
}
