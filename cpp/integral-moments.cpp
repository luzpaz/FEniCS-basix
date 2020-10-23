// Copyright (c) 2020 Chris Richardson & Matthew Scroggs
// FEniCS Project
// SPDX-License-Identifier:    MIT

#include "integral-moments.h"
#include "cell.h"
#include "polynomial-set.h"
#include "quadrature.h"

using namespace libtab;

Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
IntegralMoments::make_integral_moments(const FiniteElement& moment_space,
                                       const Cell::Type celltype,
                                       const int value_size, const int poly_deg,
                                       const int q_deg)
{
  const int psize = PolynomialSet::size(celltype, poly_deg);

  const Cell::Type sub_celltype = moment_space.cell_type();
  const int sub_entity_dim = Cell::topological_dimension(sub_celltype);
  const int sub_entity_count = Cell::sub_entity_count(celltype, sub_entity_dim);

  auto [Qpts, Qwts] = Quadrature::make_quadrature(sub_entity_dim, q_deg);
  const int tdim = Cell::topological_dimension(celltype);

  // It this is always true, value_size input can be removed
  assert(tdim == value_size);

  // Evaluate moment space at quadrature points
  Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      moment_space_at_Qpts = moment_space.tabulate(0, Qpts)[0];

  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      dualmat(moment_space_at_Qpts.cols() * sub_entity_dim * sub_entity_count,
              psize * value_size);

  int c = 0;
  // Iterate over sub entities
  for (int i = 0; i < sub_entity_count; ++i)
  {

    // FIXME: get entity tangent from the cell class
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> entity
        = Cell::sub_entity_geometry(celltype, sub_entity_dim, i);

    // Map quadrature points onto entity
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        Qpts_scaled(Qpts.rows(), tdim);
    // Parametrise entity coordinates
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> axes(
        sub_entity_dim, tdim);
    axes.setZero();

    double integral_jacobian;

    if (sub_entity_dim == 0)
    {
      throw std::runtime_error("Cannot integrate over a dimension 0 entity.");
    }
    else if (sub_entity_dim == tdim)
    {
      for (int j = 0; j < Qpts.rows(); ++j)
        Qpts_scaled.row(j) = Qpts.row(j);
      for (int j = 0; j < tdim; ++j)
        axes(j, j) = 1;
    }
    else if (sub_entity_dim == 1)
    {
      axes.row(0) = entity.row(1) - entity.row(0);
      for (int j = 0; j < Qpts.rows(); ++j)
        Qpts_scaled.row(j) = entity.row(0) + Qpts(j, 0) * axes.row(0);
    }
    else if (sub_entity_dim == 2)
    {
      axes.row(0) = entity.row(1) - entity.row(0);
      axes.row(1) = entity.row(2) - entity.row(0);
      for (int j = 0; j < Qpts.rows(); ++j)
        Qpts_scaled.row(j) = entity.row(0) + Qpts(j, 0) * axes.row(0)
                             + Qpts(j, 1) * axes.row(1);
    }
    if (sub_entity_dim == 1)
    {
      Eigen::VectorXd a0 = axes.row(0);
      integral_jacobian = a0.norm();
    }
    else if (sub_entity_dim == 2)
    {
      if (tdim == 2)
      {
        Eigen::Vector3d a0 = {axes(0, 0), axes(0, 1), 0};
        Eigen::Vector3d a1 = {axes(1, 0), axes(1, 1), 0};
        integral_jacobian = a0.cross(a1).norm();
      }
      else
      {
        Eigen::Vector3d a0 = axes.row(0);
        Eigen::Vector3d a1 = axes.row(1);
        integral_jacobian = a0.cross(a1).norm();
      }
    }
    else if (sub_entity_dim == 3)
    {
      Eigen::Vector3d a0 = axes.row(0);
      Eigen::Vector3d a1 = axes.row(1);
      Eigen::Vector3d a2 = axes.row(2);
      integral_jacobian = a0.dot(a1.cross(a2));
    }

    // Tabulate polynomial set at entity quadrature points
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        poly_set_at_Qpts
        = PolynomialSet::tabulate(celltype, poly_deg, 0, Qpts_scaled)[0].transpose();

    // Compute entity integral moments
    for (int j = 0; j < moment_space_at_Qpts.cols(); ++j)
    {
      Eigen::ArrayXd phi = moment_space_at_Qpts.col(j);
      for (int d = 0; d < sub_entity_dim; ++d)
      {
        Eigen::VectorXd axis = axes.row(d);
        for (int k = 0; k < value_size; ++k)
        {
          Eigen::VectorXd q
              = phi * Qwts * (integral_jacobian * axis(k) / axis.norm());
          Eigen::RowVectorXd qcoeffs = poly_set_at_Qpts * q;
          assert(qcoeffs.size() == psize);
          dualmat.block(c, psize * k, 1, psize) = qcoeffs;
        }
        ++c;
      }
    }
  }
  return dualmat;
}

Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
IntegralMoments::make_tangent_integral_moments(
    const FiniteElement& moment_space, const Cell::Type celltype,
    const int value_size, const int poly_deg, const int q_deg)
{
  const int psize = PolynomialSet::size(celltype, poly_deg);
  const Cell::Type sub_celltype = moment_space.cell_type();
  const int sub_entity_dim = Cell::topological_dimension(sub_celltype);
  const int sub_entity_count = Cell::sub_entity_count(celltype, 1);

  auto [Qpts, Qwts] = Quadrature::make_quadrature(1, q_deg);

  const int tdim = Cell::topological_dimension(celltype);

  // It this is always true, value_size input can be removed
  assert(tdim == value_size);

  if (sub_entity_dim != 1)
    throw std::runtime_error("Tangent is only well-defined on an edge.");

  // Evaluate moment space at quadrature points
  Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      moment_space_at_Qpts = moment_space.tabulate(0, Qpts)[0];

  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      dualmat(moment_space_at_Qpts.cols() * sub_entity_count,
              psize * value_size);

  int c = 0;

  // Iterate over sub entities
  for (int i = 0; i < sub_entity_count; ++i)
  {

    // FIXME: get edge tangent from the cell class
    Eigen::Array<double, 2, Eigen::Dynamic, Eigen::RowMajor> edge
        = Cell::sub_entity_geometry(celltype, 1, i);
    Eigen::VectorXd tangent = edge.row(1) - edge.row(0);
    // Map quadrature points onto triangle edge
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        Qpts_scaled(Qpts.rows(), tdim);
    for (int j = 0; j < Qpts.rows(); ++j)
      Qpts_scaled.row(j)
          = edge.row(0) + Qpts(j, 0) * (edge.row(1) - edge.row(0));

    // Tabulate polynomial set at edge quadrature points
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        poly_set_at_Qpts = PolynomialSet::tabulate(celltype,
                                                   poly_deg, 0, Qpts_scaled)[0]
                               .transpose();
    // Compute edge tangent integral moments
    for (int j = 0; j < moment_space_at_Qpts.cols(); ++j)
    {
      Eigen::ArrayXd phi = moment_space_at_Qpts.col(j);
      for (int k = 0; k < value_size; ++k)
      {
        Eigen::VectorXd q = phi * Qwts * tangent[k];
        Eigen::RowVectorXd qcoeffs = poly_set_at_Qpts * q;
        dualmat.block(c, psize * k, 1, psize) = qcoeffs;
      }
      ++c;
    }
  }
  return dualmat;
}
