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

#include "Header/Sample_Tracer_SLTL.h"





Sample_Tracer_SLTL::Sample_Tracer_SLTL(z3::context& context,
	Dag& dag,
	std::vector<std::string>& sample_String,
	std::string sample_Name,
	z3::expr_vector& variables_Constants,
	std::vector<Term_SLTL>& terms) :
	 Sample_Tracer(context, dag, sample_Name), terms(terms), variables_Constants(variables_Constants) 
{


	create_Sample(sample_String);

	number_Of_Variables = terms.size();


}

Sample_Tracer_SLTL::~Sample_Tracer_SLTL()
{
}


void Sample_Tracer_SLTL::initialize()
{
	for (unsigned int i = 0; i < sample.size(); i++) {
		variables_Y_Word_i_t_any.push_back(std::vector<z3::expr_vector>());
		variables_Y_Word_i_t_all.push_back(std::vector<z3::expr_vector>());
	}

	add_Variables(0);
	add_Formulas_Atomic(0);
}

void Sample_Tracer_SLTL::add_Formulas_Atomic(int iteration)
{
	int word_Index = 0;
	for (std::pair<std::vector<std::vector<signal_t>>, int> word : sample) {
		for (int p = 0; p < number_Of_Variables; p++) {

			z3::expr_vector atomic_Vector(context);
			int t = 0;
			for (std::vector<signal_t> letter : word.first) {
				atomic_Vector.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == terms[p].compute_Term_boolean(letter, variables_Constants, context, false));
				atomic_Vector.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == terms[p].compute_Term_boolean(letter, variables_Constants, context, true));
				t++;
			}
			z3::expr atomic = z3::implies(dag.variables_x_lambda_i[p][iteration], z3::mk_and(atomic_Vector));
			if (using_Incremental) {
				solver->add(atomic);
			}
			else {
				all_Formulas.push_back(atomic);
			}
		}
		word_Index++;
	}

}

void Sample_Tracer_SLTL::create_Sample(std::vector<std::string> input_Sample_SLTL)
{

	std::stringstream string_stream;
	std::string line;

	for (std::string word : input_Sample_SLTL) {

		string_stream = std::stringstream(word);

		// get the beginning of the repeting part

		std::getline(string_stream, line, ':');
		std::string remaining_word = line;

		Trace_Metadata trace_Metadata = {.size=0, .repetition=-1, .weight=1};
		if (std::getline(string_stream, line, ':')) {
			if (std::getline(string_stream, line, '[')) {
				if (line.size() > 0) trace_Metadata.repetition = std::stoi(line);
				if (std::getline(string_stream, line, ']')) {
					trace_Metadata.weight = std::stoi(line);
				}
			}
		}

		// make the string to a vector of vector of signal_t

		string_stream = std::stringstream(remaining_word);

		// seperate all letters

		std::vector<std::string> initial_seperation;
		while (std::getline(string_stream, line, ';')) {
			initial_seperation.push_back(line);
		}

		// make string to 'signal_t's

		std::vector<std::vector<signal_t>> letters;
		for (std::string s : initial_seperation) {
			string_stream = std::stringstream(s);
			std::vector<signal_t> letter;
			while (std::getline(string_stream, line, ',')) {
				signal_t sig;
				if (line.at(0) == '[') { // example: "[0.9,1.1]"
					std::string lower_bound, upper_bound;
					lower_bound = line;
					lower_bound.erase(0,1); // erase '['
					std::getline(string_stream, line, ',');
					upper_bound = line;
					upper_bound.pop_back(); // erase ']'
					sig = std::make_pair(lower_bound,upper_bound);
				} else { // example: "1.0"
					sig = std::make_pair(line,line);
				}
				letter.push_back(sig);
			}
			letters.push_back(letter);
		}

		trace_Metadata.size = letters.size();
		if (trace_Metadata.repetition < 0) {
			// index from the end
			trace_Metadata.repetition = trace_Metadata.size + trace_Metadata.repetition;
		}

		sample.push_back(std::make_pair(letters, trace_Metadata.repetition));
		sample_Metadatas.push_back(trace_Metadata);
	}
}
