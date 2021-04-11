
#pragma once
#include <functional>
#include <z3++.h>

/* Return an expression for the maximum of [start, args[i]...].
    merge: operator used to merge expressions from args and start 2 by 2.
    args: vector of expressions to merge
    start: expression to start with
*/
z3::expr mk_chain(
    std::function<z3::expr(z3::expr, z3::expr)> merge,
    z3::expr_vector const &args,
    z3::expr start);

/* Return an expression for the maximum of [start, args[i] where filter[i]...].
`args` and `filter` should have the same size.
    merge: operator used to merge expressions from args and start 2 by 2.
    args: vector of expressions to merge
    start: expression to start with if `started` is true, or to return if there is no index i such that filter[i]
    filter: vector of Boolean expr; take args[i] into account if and only if filter[i] is true
    started: Boolean expr indicating if `start` should or has been taken into account.
*/
z3::expr mk_chain(
    std::function<z3::expr(z3::expr, z3::expr)> merge,
    z3::expr_vector const &args,
    z3::expr start,
    z3::expr_vector const &filter,
    z3::expr started);
