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

#include "Header/Functor_Operators.h"

void make_bounds(z3::expr& lowerbound, z3::expr& upperbound, z3::expr& bounds_formula,
	const z3::expr& param_a, const z3::expr& param_b, int max_Word_Size, int max_Word_Period)
{
	lowerbound = param_a;
	upperbound = lowerbound + max_Word_Size - param_b;
	bounds_formula = (0 <= param_a) && (0 <= param_b) && (lowerbound < upperbound) && (upperbound <= max_Word_Period);
}

z3::expr is_timestamp_in_bounds(int t, const z3::expr& lowerbound, const z3::expr& upperbound,
	z3::context& context, int word_Size, int repetition, int max_Word_Period)
{
	// return context.bool_val(true); /* no bounds */

	// if (t<repetition) {
	// 	return (lowerbound <= t) && (t < upperbound);
	// } else {
	// 	z3::expr l = z3::ite(lowerbound>repetition, lowerbound, context.int_val(repetition)); /* l = max(lowerbound,repetition) */
	// 	// z3::expr u = z3::ite(upperbound>repetition, upperbound, context.int_val(repetition)); /* u = max(upperbound,repetition) */
	// 	return (t-l)%(word_Size-repetition) < (upperbound-l);
	// 	/* exists k s.t.  l <= t+k(word_Size-repetition) < u */
	// 	/* exists k s.t.  0 <= (t-l)+k(word_Size-repetition) < (u-l) */
	// }

	z3::expr_vector disjunction_In_Bounds(context);
	for (int t2 = t; t2 < max_Word_Period; t2 += word_Size-repetition) {
		disjunction_In_Bounds.push_back((lowerbound <= t2) && (t2 < upperbound));
		if (t2 < repetition) break;
	}
	return z3::mk_or(disjunction_In_Bounds);
}
