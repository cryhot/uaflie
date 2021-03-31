#include "Header/Utils.h"

z3::expr mk_max(z3::expr_vector const &args, z3::expr start)
{
	for (z3::expr arg: args){
		start = z3::max(start, arg);
	}
	return start;
};

z3::expr mk_min(z3::expr_vector const &args, z3::expr start)
{
	for (z3::expr arg: args){
		start = z3::min(start, arg);
	}
	return start;
};

z3::expr mk_max(z3::expr_vector const &args)
{
	assert (args.size() > 0);
    return mk_max(args, args[0]);
};

z3::expr mk_min(z3::expr_vector const &args)
{
	assert (args.size() > 0);
    return mk_min(args, args[0]);
};
