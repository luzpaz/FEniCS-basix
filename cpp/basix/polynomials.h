// Copyright (c) 2021 Matthew Scroggs
// FEniCS Project
// SPDX-License-Identifier:    MIT

#pragma once

#include "cell.h"
#include "mdspan.hpp"
#include <array>
#include <utility>
#include <vector>

/// Polynomials
namespace basix::polynomials
{
/// Variants of a Lagrange space that can be created
enum class type
{
  legendre = 0,
  bernstein = 1,
};

/// @brief Tabulate a set of polynomials.
/// @param[in] polytype Polynomial type
/// @param[in] celltype Cell type
/// @param[in] d Polynomial degree
/// @param[in] x Points at which to evaluate the basis. The shape is
/// (number of points, geometric dimension).
/// @return Polynomial sets, for each derivative, tabulated at points.
/// The shape is `(basis index, number of points)`.
std::pair<std::vector<double>, std::array<std::size_t, 2>>
tabulate(polynomials::type polytype, cell::type celltype, int d,
         std::experimental::mdspan<const double,
                                   std::experimental::dextents<std::size_t, 2>>
             x);

/// @brief Dimension of a polynomial space.
/// @param[in] polytype The polynomial type
/// @param[in] cell The cell type
/// @param[in] d The polynomial degree
/// @return The number terms in the basis spanning a space of
/// polynomial degree @p d
int dim(polynomials::type polytype, cell::type cell, int d);

} // namespace basix::polynomials
