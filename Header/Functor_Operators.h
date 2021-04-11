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

#pragma once


#include <z3++.h>
#include <vector>
#include <assert.h>
#include "Header/Utils.h"

/*
Compute `lowerbound`, `upperbound` and `bounds_formula` from node parameters.
*/
void make_bounds(z3::expr& lowerbound, z3::expr& upperbound, z3::expr& bounds_formula,
	const z3::expr& param_a, const z3::expr& param_b, int max_Word_Size, int max_Word_Period);

/*
Returns a boolean expression which is true if the letter "word[t]" is visited
at least one time in the given interval.
*/
z3::expr is_timestamp_in_bounds(int t, const z3::expr& lowerbound, const z3::expr& upperbound,
	z3::context& context, int word_Size, int repetition, int max_Word_Period);


class Operator_Unary {

public:
	Operator_Unary() {};
	virtual z3::expr make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{
		return z3::expr(context);
	};
};

class Operator_Not : public Operator_Unary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{

		z3::expr_vector inner_Formula(context);

		for (int t = 0; t < word_Size; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				(-variables_Y_Word_i_t_all[word_Index][j][t]));
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				(-variables_Y_Word_i_t_any[word_Index][j][t]));
		}

		return z3::mk_and(inner_Formula);
	}
};

class Operator_Next : public Operator_Unary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{

		z3::expr_vector inner_Formula(context);

		for (int t = 0; t < word_Size - 1; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				variables_Y_Word_i_t_any[word_Index][j][t + 1]);
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				variables_Y_Word_i_t_all[word_Index][j][t + 1]);
		}
		inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][word_Size - 1] ==
			variables_Y_Word_i_t_any[word_Index][j][repetition]);
		inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][word_Size - 1] ==
			variables_Y_Word_i_t_all[word_Index][j][repetition]);

		return z3::mk_and(inner_Formula);
	}
};

class Operator_Finally : public Operator_Unary {
	int max_Word_Size, max_Word_Period;

public:
	Operator_Finally(int max_Word_Size, int max_Word_Period) {
		this->max_Word_Size = max_Word_Size;
		this->max_Word_Period = max_Word_Period;
	}

	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{
		z3::expr lowerbound(context), upperbound(context), bounds_formula(context);
		make_bounds(lowerbound, upperbound, bounds_formula, parameter_p[0], parameter_p[1], max_Word_Size, max_Word_Period);

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector disjunction_any(context),  disjunction_all(context);
			z3::expr_vector s_In_Bounds(context);
            int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				s_In_Bounds.push_back(is_timestamp_in_bounds(s, t+lowerbound, t+upperbound, context, word_Size, repetition, max_Word_Period));
				disjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][s]);
				disjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][s]); // too strong
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_any, disjunction_any[0], s_In_Bounds, context.bool_val(false)));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_all, disjunction_all[0], s_In_Bounds, context.bool_val(false)));
		}
		z3::expr inner_Formula = z3::mk_and(conjunction_Outer) && bounds_formula;

		/*

		z3::expr_vector conjunction_Pre_Loop(context);
		for (unsigned int t = 0; t < word.second; t++) {
		z3::expr_vector disjunction(context);
		for (int s = t; s < word.first.size(); s++) {
		disjunction.push_back(variables_Y_Word_i_t[word_Index][j][s]);
		}
		conjunction_Pre_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] == z3::mk_or(disjunction));
		}

		z3::expr_vector conjunction_In_Loop(context);
		for (unsigned int t = word.second; t < word.first.size(); t++) {
		z3::expr_vector disjunction(context);
		for (int s = word.second; s < word.first.size(); s++) {
		disjunction.push_back(variables_Y_Word_i_t[word_Index][j][s]);
		}
		conjunction_In_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] == z3::mk_or(disjunction));
		}

		z3::expr inner_Formula = z3::mk_and(conjunction_In_Loop) && z3::mk_and(conjunction_Pre_Loop);
		*/
		return inner_Formula;
	}
};

class Operator_Globally : public Operator_Unary {
	int max_Word_Size, max_Word_Period;

public:
	Operator_Globally(int max_Word_Size, int max_Word_Period) {
		this->max_Word_Size = max_Word_Size;
		this->max_Word_Period = max_Word_Period;
	}

	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{
		z3::expr lowerbound(context), upperbound(context), bounds_formula(context);
		make_bounds(lowerbound, upperbound, bounds_formula, parameter_p[0], parameter_p[1], max_Word_Size, max_Word_Period);

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector conjunction_any(context), conjunction_all(context);
            z3::expr_vector s_In_Bounds(context);
			int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				s_In_Bounds.push_back(is_timestamp_in_bounds(s, t+lowerbound, t+upperbound, context, word_Size, repetition, max_Word_Period));
				conjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][s]); // too weak
				conjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][s]);
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::min, conjunction_any, conjunction_any[0], s_In_Bounds, context.bool_val(false)));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::min, conjunction_all, conjunction_all[0], s_In_Bounds, context.bool_val(false)));
		}
		z3::expr inner_Formula = z3::mk_and(conjunction_Outer) && bounds_formula;




		/*
		z3::expr_vector conjunction_Pre_Loop(context);
		for (unsigned int t = 0; t < word.second; t++) {
		z3::expr_vector conjunction(context);
		for (int s = t; s < word.first.size(); s++) {
		conjunction.push_back(variables_Y_Word_i_t[word_Index][j][s]);
		}
		conjunction_Pre_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] == z3::mk_and(conjunction));
		}

		z3::expr_vector conjunction_In_Loop(context);
		for (unsigned int t = word.second; t < word.first.size(); t++) {
		z3::expr_vector conjunction(context);
		for (int s = word.second; s < word.first.size(); s++) {
		conjunction.push_back(variables_Y_Word_i_t[word_Index][j][s]);
		}
		conjunction_In_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] == z3::mk_and(conjunction));
		}

		z3::expr inner_Formula = z3::mk_and(conjunction_In_Loop) && z3::mk_and(conjunction_Pre_Loop);
		*/
		return inner_Formula;
	}
};



class Operator_Binary {

public:
	Operator_Binary() {};
	virtual z3::expr make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{
		return z3::expr(context);
	};
};

class Operator_Or : public Operator_Binary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{

		z3::expr_vector inner_Formula(context);
		for (int t = 0; t < word_Size; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				z3::max(variables_Y_Word_i_t_any[word_Index][j][t], variables_Y_Word_i_t_any[word_Index][k][t]));
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				z3::max(variables_Y_Word_i_t_all[word_Index][j][t], variables_Y_Word_i_t_all[word_Index][k][t])); // too strong
		}

		return z3::mk_and(inner_Formula);
	}
};

class Operator_And : public Operator_Binary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{

		z3::expr_vector inner_Formula(context);
		for (int t = 0; t < word_Size; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				z3::min(variables_Y_Word_i_t_any[word_Index][j][t], variables_Y_Word_i_t_any[word_Index][k][t])); // too weak
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				z3::min(variables_Y_Word_i_t_all[word_Index][j][t], variables_Y_Word_i_t_all[word_Index][k][t]));
		}

		return z3::mk_and(inner_Formula);
	}
};

class Operator_Implies : public Operator_Binary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{

		z3::expr_vector inner_Formula(context);
		for (int t = 0; t < word_Size; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				z3::max(-variables_Y_Word_i_t_all[word_Index][j][t], variables_Y_Word_i_t_any[word_Index][k][t]));
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				z3::max(-variables_Y_Word_i_t_any[word_Index][j][t], variables_Y_Word_i_t_all[word_Index][k][t])); // too strong
		}

		return z3::mk_and(inner_Formula);
	}
};

class Operator_Until : public Operator_Binary {
	int max_Word_Size, max_Word_Period;

public:
	Operator_Until(int max_Word_Size, int max_Word_Period) {
		this->max_Word_Size = max_Word_Size;
		this->max_Word_Period = max_Word_Period;
	}

	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all,
		z3::expr_vector& parameter_p)
	{
		z3::expr lowerbound(context), upperbound(context), bounds_formula(context);
		make_bounds(lowerbound, upperbound, bounds_formula, parameter_p[0], parameter_p[1], max_Word_Size, max_Word_Period);

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector disjunction_any(context), disjunction_all(context);
            z3::expr_vector s_In_Bounds(context);
			int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				z3::expr_vector conjunction_any(context), conjunction_all(context);
                z3::expr_vector q_In_Bounds(context);
				int q = t;
				while (q != s) {
                    q_In_Bounds.push_back(is_timestamp_in_bounds(q, t+lowerbound, t+upperbound, context, word_Size, repetition, max_Word_Period));
					conjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][q]);
					conjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][q]);
					q = (q < word_Size - 1) ? q + 1 : repetition;
				}
                s_In_Bounds.push_back(is_timestamp_in_bounds(s, t+lowerbound, t+upperbound, context, word_Size, repetition, max_Word_Period));
				disjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][k][s] && mk_chain(&z3::min, conjunction_any, conjunction_any[0], q_In_Bounds, context.bool_val(false)));
				disjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][k][s] && mk_chain(&z3::min, conjunction_all, conjunction_all[0], q_In_Bounds, context.bool_val(false)));
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_any, disjunction_any[0], s_In_Bounds, context.bool_val(false)));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_all, disjunction_all[0], s_In_Bounds, context.bool_val(false)));
		}
		z3::expr inner_Formula = z3::mk_and(conjunction_Outer);

		/*
		z3::expr_vector conjunction_Loop(context);

		for (int t = 0; t < repetition; t++) {
			z3::expr_vector disjunction(context);
			for (int s = t; s < word_Size; s++) {
				z3::expr_vector conjunction(context);
				conjunction.push_back(variables_Y_Word_i_t[word_Index][k][s]);
				for (int q = t; q < s; q++) {
					conjunction.push_back(variables_Y_Word_i_t[word_Index][j][q]);
				}
				disjunction.push_back(z3::mk_and(conjunction));
			}
			conjunction_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] ==
				z3::mk_or(disjunction));
		}

		for (int t = repetition; t < word_Size; t++) {
			z3::expr_vector disjunction(context);
			for (int s = repetition; s < word_Size; s++) {
				z3::expr_vector conjunction(context);
				conjunction.push_back(variables_Y_Word_i_t[word_Index][k][s]);
				int q = t;
				while (q != s) {
					conjunction.push_back(variables_Y_Word_i_t[word_Index][j][q]);

					q++;
					if (q >= word_Size) {
						q = repetition;
					}
				}
				disjunction.push_back(z3::mk_and(conjunction));
			}
			conjunction_Loop.push_back(variables_Y_Word_i_t[word_Index][iteration][t] ==
				z3::mk_or(disjunction));
		}


		z3::expr inner_Formula = z3::mk_and(conjunction_Loop);
		*/

		return inner_Formula;
	}
};
