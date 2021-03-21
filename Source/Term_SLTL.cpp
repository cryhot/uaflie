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

#include "Header/Term_SLTL.h"

int Term_SLTL::max_Constant = -1;



Term_SLTL::Term_SLTL(std::string & input_Line)
{
	std::stringstream string_stream(input_Line);
	std::string op;
	std::string left;
	std::string right;

	std::getline(string_stream, op, '(');
	oper = op.at(0);
	if (oper == 's') {
		op.erase(op.begin());
		var = std::stoi(op);
	}
	else if (oper == 'c') {
		op.erase(op.begin());
		var = std::stoi(op);
		if (var > max_Constant) max_Constant = var;
	}
	else {

		std::string remaining;
		std::getline(string_stream, remaining);

		int brackets = 0;
		for (unsigned int i = 0; i < remaining.size(); i++) {

			char c = remaining.at(i);

			switch (c) {
			case '(':
				brackets++;
				break;
			case ')':
				brackets--;
				break;
			case ',':
				if (brackets == 0) {
					left = remaining.substr(0, i);
					remaining.erase(remaining.begin(), remaining.begin() + i + 1);
					goto make_Right;
				}
			default:
				break;
			}
		}

	make_Right:

		remaining.erase(remaining.end() - 1);
		right = remaining;


		left_Term = new Term_SLTL(left);
		right_Term = new Term_SLTL(right);

	}
}


z3::expr Term_SLTL::compute_Term_boolean(std::vector<signal_t>& signals, z3::expr_vector& constants, z3::context& context, bool forall) {

	std::pair<z3::expr,z3::expr> left = left_Term->compute_Term_arithmetic(signals, constants, context);
	std::pair<z3::expr,z3::expr> right = right_Term->compute_Term_arithmetic(signals, constants, context);

	if (forall) { // for all possible pathes

		switch (oper) {
		case '<':
			return (right.first - left.second);
		case '>':
			return (left.first - right.second);
		case '=':
			return z3::min(right.first - left.second, left.first - right.second);
		case '!':
			return z3::max(right.first - left.second, left.first - right.second);
		}

	} else { // for at least one possible path

		switch (oper) {
		case '<':
			return (right.second - left.first);
		case '>':
			return (left.second - right.first);
		case '=':
			return z3::min(right.second - left.first, left.second - right.first);
		case '!':
			return z3::max(right.second - left.first, left.second - right.first);
		}

	}

	throw "Unknown boolean term";
}

std::pair<z3::expr,z3::expr> Term_SLTL::compute_Term_arithmetic(std::vector<signal_t>& signals, z3::expr_vector& constants, z3::context& context) {

	// in a pair, first: lower bound, second: upper bound
	// we assume (lower <= upper) always true
	z3::expr lower(context);
	z3::expr upper(context);

	switch (oper) {
		case 'c':
			lower = constants[var];
			upper = constants[var];
			return std::make_pair(lower,upper);
		case 's':
			lower = context.real_val(signals[var].first.c_str());
			upper = context.real_val(signals[var].second.c_str());
			return std::make_pair(lower,upper);
	}

	std::pair<z3::expr,z3::expr> left = left_Term->compute_Term_arithmetic(signals, constants, context);
	std::pair<z3::expr,z3::expr> right = right_Term->compute_Term_arithmetic(signals, constants, context);

	switch (oper) {
	case '+':
		lower = (left.first + right.first);
		upper = (left.second + right.second);
		return std::make_pair(lower,upper);
	case '-':
		lower = (left.first - right.second);
		upper = (left.second - right.first);
		return std::make_pair(lower,upper);
	case '*': {
		z3::expr l_times_l = left.first * right.first;
		z3::expr l_times_u = left.first * right.second;
		z3::expr u_times_l = left.second * right.first;
		z3::expr u_times_u = left.second * right.second;
		lower = z3::min(z3::min(l_times_l, l_times_u), z3::min(u_times_l, u_times_u));
		upper = z3::max(z3::max(l_times_l, l_times_u), z3::max(u_times_l, u_times_u));
		return std::make_pair(lower,upper); }
	}

	throw "Unknown arithmetic term";
}


std::string Term_SLTL::print_Term(z3::model& model, z3::expr_vector& constants) {
	std::stringstream result;

	if (oper != 'c' && oper != 's') {
		result << "(";
		result << left_Term->print_Term(model, constants);
		result << oper;
		result << right_Term->print_Term(model, constants);
		result << ")";
	}
	else {


		result << oper;
		result << var;
		if (oper == 'c') {
			result << "(";
			result << model.eval(constants[var]);

			result << ")";
		}
	}

	formula = result.str();
	return formula;
}
