// ---------------------------------------------------------------------
//
// Copyright (C) 2001 - 2016 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------

#include <deal.II/base/utilities.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/parallel_vector.h>
#include <deal.II/lac/parallel_block_vector.h>
#include <deal.II/lac/petsc_vector.h>
#include <deal.II/lac/petsc_block_vector.h>
#include <deal.II/lac/trilinos_vector.h>
#include <deal.II/lac/trilinos_block_vector.h>
#include <deal.II/lac/trilinos_parallel_block_vector.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/fe/fe.h>
#include <deal.II/fe/fe_tools.h>
#include <deal.II/fe/mapping_q_eulerian.h>
#include <deal.II/fe/mapping_q1_eulerian.h>

DEAL_II_NAMESPACE_OPEN



// .... MAPPING Q EULERIAN CONSTRUCTOR

template <int dim, class VectorType, int spacedim>
MappingQEulerian<dim, VectorType, spacedim>::MappingQEulerianGeneric::
MappingQEulerianGeneric (const unsigned int                                 degree,
                         const MappingQEulerian<dim, VectorType, spacedim> &mapping_q_eulerian)
  :
  MappingQGeneric<dim,spacedim>(degree),
  mapping_q_eulerian (mapping_q_eulerian),
  support_quadrature(degree),
  fe_values(mapping_q_eulerian.euler_dof_handler->get_fe(),
            support_quadrature,
            update_values | update_q_points)
{}

template <int dim, class VectorType, int spacedim>
MappingQEulerian<dim, VectorType, spacedim>::
MappingQEulerian (const unsigned int              degree,
                  const VectorType               &euler_vector,
                  const DoFHandler<dim,spacedim> &euler_dof_handler)
  :
  MappingQ<dim,spacedim>(degree, true),
  euler_vector(&euler_vector),
  euler_dof_handler(&euler_dof_handler)
{
  // reset the q1 mapping we use for interior cells (and previously
  // set by the MappingQ constructor) to a MappingQ1Eulerian with the
  // current vector
  this->q1_mapping.reset (new MappingQ1Eulerian<dim,VectorType,spacedim>(euler_vector,
                          euler_dof_handler));

  // also reset the qp mapping pointer with our own class
  this->qp_mapping.reset (new MappingQEulerianGeneric(degree,*this));
}



template <int dim, class VectorType, int spacedim>
MappingQEulerian<dim, VectorType, spacedim>::
MappingQEulerian (const unsigned int              degree,
                  const DoFHandler<dim,spacedim> &euler_dof_handler,
                  const VectorType               &euler_vector)
  :
  MappingQ<dim,spacedim>(degree, true),
  euler_vector(&euler_vector),
  euler_dof_handler(&euler_dof_handler)
{
  // reset the q1 mapping we use for interior cells (and previously
  // set by the MappingQ constructor) to a MappingQ1Eulerian with the
  // current vector
  this->q1_mapping.reset (new MappingQ1Eulerian<dim,VectorType,spacedim>(euler_vector,
                          euler_dof_handler));

  // also reset the qp mapping pointer with our own class
  this->qp_mapping.reset (new MappingQEulerianGeneric(degree,*this));
}



template <int dim, class VectorType, int spacedim>
Mapping<dim,spacedim> *
MappingQEulerian<dim, VectorType, spacedim>::clone () const
{
  return new MappingQEulerian<dim,VectorType,spacedim>(this->get_degree(),
                                                       *euler_vector,
                                                       *euler_dof_handler);
}



// .... SUPPORT QUADRATURE CONSTRUCTOR

template <int dim, class VectorType, int spacedim>
MappingQEulerian<dim,VectorType,spacedim>::MappingQEulerianGeneric::
SupportQuadrature::
SupportQuadrature (const unsigned int map_degree)
  :
  Quadrature<dim>(Utilities::fixed_power<dim>(map_degree+1))
{
  // first we determine the support points on the unit cell in lexicographic
  // order, which are (in accordance with MappingQ) the support points of
  // QGaussLobatto.
  const QGaussLobatto<dim> q_iterated(map_degree+1);
  const unsigned int n_q_points = q_iterated.size();

  // we then need to define a renumbering vector that allows us to go from a
  // lexicographic numbering scheme to a hierarchic one.  this fragment is
  // taking almost verbatim from the MappingQ class.
  std::vector<unsigned int> renumber(n_q_points);
  std::vector<unsigned int> dpo(dim+1, 1U);
  for (unsigned int i=1; i<dpo.size(); ++i)
    dpo[i]=dpo[i-1]*(map_degree-1);

  FETools::lexicographic_to_hierarchic_numbering (FiniteElementData<dim> (dpo, 1, map_degree),
                                                  renumber);

  // finally we assign the quadrature points in the required order.
  for (unsigned int q=0; q<n_q_points; ++q)
    this->quadrature_points[renumber[q]] = q_iterated.point(q);
}



// .... COMPUTE MAPPING SUPPORT POINTS

template <int dim, class VectorType, int spacedim>
std_cxx11::array<Point<spacedim>, GeometryInfo<dim>::vertices_per_cell>
MappingQEulerian<dim, VectorType, spacedim>::
get_vertices
(const typename Triangulation<dim,spacedim>::cell_iterator &cell) const
{
  // get the vertices as the first 2^dim mapping support points
  const std::vector<Point<spacedim> > a
    = dynamic_cast<const MappingQEulerianGeneric &>(*this->qp_mapping).compute_mapping_support_points(cell);

  std_cxx11::array<Point<spacedim>, GeometryInfo<dim>::vertices_per_cell> vertex_locations;
  std::copy (a.begin(),
             a.begin()+GeometryInfo<dim>::vertices_per_cell,
             vertex_locations.begin());

  return vertex_locations;
}



template <int dim, class VectorType, int spacedim>
std_cxx11::array<Point<spacedim>, GeometryInfo<dim>::vertices_per_cell>
MappingQEulerian<dim, VectorType, spacedim>::MappingQEulerianGeneric::
get_vertices
(const typename Triangulation<dim,spacedim>::cell_iterator &cell) const
{
  return mapping_q_eulerian.get_vertices (cell);
}




template <int dim, class VectorType, int spacedim>
std::vector<Point<spacedim> >
MappingQEulerian<dim, VectorType, spacedim>::MappingQEulerianGeneric::
compute_mapping_support_points (const typename Triangulation<dim,spacedim>::cell_iterator &cell) const
{
  // first, basic assertion with respect to vector size,

  const types::global_dof_index n_dofs  = mapping_q_eulerian.euler_dof_handler->n_dofs();
  const types::global_dof_index vector_size = mapping_q_eulerian.euler_vector->size();
  (void)n_dofs;
  (void)vector_size;

  AssertDimension(vector_size,n_dofs);

  // we then transform our tria iterator into a dof iterator so we can access
  // data not associated with triangulations
  typename DoFHandler<dim,spacedim>::cell_iterator dof_cell(*cell,
                                                            mapping_q_eulerian.euler_dof_handler);

  Assert (dof_cell->active() == true, ExcInactiveCell());

  // our quadrature rule is chosen so that each quadrature point corresponds
  // to a support point in the undeformed configuration.  we can then query
  // the given displacement field at these points to determine the shift
  // vector that maps the support points to the deformed configuration.

  // we assume that the given field contains dim displacement components, but
  // that there may be other solution components as well (e.g. pressures).
  // this class therefore assumes that the first dim components represent the
  // actual shift vector we need, and simply ignores any components after
  // that.  this implies that the user should order components appropriately,
  // or create a separate dof handler for the displacements.

  const unsigned int n_support_pts = support_quadrature.size();
  const unsigned int n_components  = mapping_q_eulerian.euler_dof_handler->get_fe().n_components();

  Assert (n_components >= spacedim, ExcDimensionMismatch(n_components, spacedim) );

  std::vector<Vector<typename VectorType::value_type> >
  shift_vector(n_support_pts,
               Vector<typename VectorType::value_type>(n_components));

  // fill shift vector for each support point using an fe_values object. make
  // sure that the fe_values variable isn't used simultaneously from different
  // threads
  Threads::Mutex::ScopedLock lock(fe_values_mutex);
  fe_values.reinit(dof_cell);
  fe_values.get_function_values(*mapping_q_eulerian.euler_vector, shift_vector);

  // and finally compute the positions of the support points in the deformed
  // configuration.
  std::vector<Point<spacedim> > a(n_support_pts);
  for (unsigned int q=0; q<n_support_pts; ++q)
    {
      a[q] = fe_values.quadrature_point(q);
      for (unsigned int d=0; d<spacedim; ++d)
        a[q](d) += shift_vector[q](d);
    }

  return a;
}



template<int dim, class VectorType, int spacedim>
CellSimilarity::Similarity
MappingQEulerian<dim,VectorType,spacedim>::
fill_fe_values (const typename Triangulation<dim,spacedim>::cell_iterator &cell,
                const CellSimilarity::Similarity                           ,
                const Quadrature<dim>                                     &quadrature,
                const typename Mapping<dim,spacedim>::InternalDataBase    &internal_data,
                internal::FEValues::MappingRelatedData<dim,spacedim>      &output_data) const
{
  // call the function of the base class, but ignoring
  // any potentially detected cell similarity between
  // the current and the previous cell
  MappingQ<dim,spacedim>::fill_fe_values (cell,
                                          CellSimilarity::invalid_next_cell,
                                          quadrature,
                                          internal_data,
                                          output_data);
  // also return the updated flag since any detected
  // similarity wasn't based on the mapped field, but
  // the original vertices which are meaningless
  return CellSimilarity::invalid_next_cell;
}



// explicit instantiations
#include "mapping_q_eulerian.inst"


DEAL_II_NAMESPACE_CLOSE
