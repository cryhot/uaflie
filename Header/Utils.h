
#pragma once
#include <z3++.h>

/* Return an expression for the maximum of [start, args...]. */
z3::expr mk_max(z3::expr_vector const &args, z3::expr start);

/* Return an expression for the minimum of [start, args...]. */
z3::expr mk_min(z3::expr_vector const &args, z3::expr start);


/* Return an expression for the maximum of [args...].
   args vector should not be empty. */
z3::expr mk_max(z3::expr_vector const &args);

/* Return an expression for the minimum of [args...].
   args vector should not be empty. */
z3::expr mk_min(z3::expr_vector const &args);
