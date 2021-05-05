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

class Operator_Unary {

public:
	Operator_Unary() {};
	virtual z3::expr make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{
		return z3::expr(context);
	};
};

class Operator_Not : public Operator_Unary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
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
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{

		z3::expr_vector inner_Formula(context);

		for (int t = 0; t < word_Size - 1; t++) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] ==
				variables_Y_Word_i_t_any[word_Index][j][t + 1]);
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] ==
				variables_Y_Word_i_t_all[word_Index][j][t + 1]);
		}
		if (repetition<word_Size) {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][word_Size - 1] ==
				variables_Y_Word_i_t_any[word_Index][j][repetition]);
			inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][word_Size - 1] ==
				variables_Y_Word_i_t_all[word_Index][j][repetition]);
		} else {
			inner_Formula.push_back(variables_Y_Word_i_t_any[word_Index][iteration][word_Size - 1] == INT_MIN); // TODO: minus infinity
            inner_Formula.push_back(variables_Y_Word_i_t_all[word_Index][iteration][word_Size - 1] == INT_MIN); // TODO: minus infinity
		}

		return z3::mk_and(inner_Formula);
	}
};

class Operator_Finally : public Operator_Unary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{


		z3::expr_vector conjunction_Pre_Loop(context);

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector disjunction_any(context),  disjunction_all(context);
			int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				disjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][s]);
				disjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][s]); // too strong
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_any, disjunction_any[0]));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_all, disjunction_all[0]));
		}
		z3::expr inner_Formula = z3::mk_and(conjunction_Outer);

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

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{
		z3::expr_vector conjunction_Pre_Loop(context);

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector conjunction_any(context), conjunction_all(context);
			int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				conjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][s]); // too weak
				conjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][s]);
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::min, conjunction_any, conjunction_any[0]));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::min, conjunction_all, conjunction_all[0]));
		}
		z3::expr inner_Formula = z3::mk_and(conjunction_Outer);




		/*
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
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{
		return z3::expr(context);
	};
};

class Operator_Or : public Operator_Binary {

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
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
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
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
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
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

public:
	z3::expr  make_Inner_Formula(int iteration, int j, int k, int word_Index,
		z3::context& context, int word_Size, int repetition,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_any,
		std::vector<std::vector<z3::expr_vector>>& variables_Y_Word_i_t_all)
	{

		z3::expr_vector conjunction_Outer(context);
		for (int t = 0; t < word_Size; t++) {
			int s = t;
			z3::expr_vector disjunction_any(context), disjunction_all(context);
			int stopping_Time = (t <= repetition) ? word_Size - 1 : t - 1;
			while (true) {
				z3::expr_vector conjunction_any(context), conjunction_all(context);
				int q = t;
				while (q != s) {
					conjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][j][q]);
					conjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][j][q]);
					q = (q < word_Size - 1) ? q + 1 : repetition;
				}
				conjunction_any.push_back(variables_Y_Word_i_t_any[word_Index][k][s]);
				conjunction_all.push_back(variables_Y_Word_i_t_all[word_Index][k][s]);
				disjunction_any.push_back(mk_chain(&z3::min, conjunction_any, conjunction_any[0]));
				disjunction_all.push_back(mk_chain(&z3::min, conjunction_all, conjunction_all[0]));
				if (s == stopping_Time) break;
				s = (s < word_Size - 1) ? s + 1 : repetition;
			}
			conjunction_Outer.push_back(variables_Y_Word_i_t_any[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_any, disjunction_any[0]));
			conjunction_Outer.push_back(variables_Y_Word_i_t_all[word_Index][iteration][t] == mk_chain(&z3::max, disjunction_all, disjunction_all[0]));
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
