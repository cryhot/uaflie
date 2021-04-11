#include "Header/Utils.h"

z3::expr mk_chain(
    std::function<z3::expr(z3::expr, z3::expr)> merge,
    z3::expr_vector const &args,
    z3::expr start)
{
	for (z3::expr arg: args) {
		start = merge(start, arg);
	}
	return start;
}

z3::expr mk_chain(
    std::function<z3::expr(z3::expr, z3::expr)> merge,
    z3::expr_vector const &args,
    z3::expr start,
    z3::expr_vector const &filter,
    z3::expr started)
{
	for (unsigned int i=0; i<args.size(); i++) {
		start = z3::ite(filter[i],
			z3::ite(started,
				merge(start, args[i]),
				args[i]),
			start);
		started = started || filter[i];
	}
	return start;
}
