/*
 * reference_triangle.h
 *
 *  Created on: 15.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TRIANGLE__
#define __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TRIANGLE__

#include "common/math/ugmath.h"
#include "lib_grid/grid/geometric_base_objects.h"
#include "reference_element_mapping.h"

namespace ug{

class ReferenceTriangle{
	public:
	///	type of reference element
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_TRIANGLE;

	///	dimension of reference element
		static const int dim = 2;

	///	number of corners
		static const int num_corners = 3;

	///	number of eges
		static const int num_edges = 3;

	///	number of faces
		static const int num_faces = 1;

	///	number of volumes
		static const int num_volumes = 0;

	public:
	///	Constructor filling the arrays
		ReferenceTriangle();

	/// \copydoc ug::ReferenceElement::reference_object_id()
		ReferenceObjectID reference_object_id() const {return REFERENCE_OBJECT_ID;}

	/// \copydoc ug::ReferenceElement::dimension()
		int dimension() const {return dim;}

	/// \copydoc ug::ReferenceElement::size()
		number size() const	{return 0.5;}

	/// \copydoc ug::ReferenceElement::num(int)
		size_t num(int dim) const	{return m_vNum[dim];}

	/// \copydoc ug::ReferenceElement::num(int, size_t, int)
		size_t num(int dim_i, size_t i, int dim_j) const
			{return m_vSubNum[dim_i][i][dim_j];}

	/// \copydoc ug::ReferenceElement::id()
		int id(int dim_i, size_t i, int dim_j, size_t j) const
			{return m_id[dim_i][i][dim_j][j];}

	/// \copydoc ug::ReferenceElement::num_ref_elem()
		size_t num_ref_elem(ReferenceObjectID type) const {return m_vNumRefElem[type];}

	/// \copydoc ug::ReferenceElement::ref_elem_type()
		ReferenceObjectID ref_elem_type(int dim_i, size_t i) const{	return m_vRefElemType[dim_i][i];}

	/// \copydoc ug::DimReferenceElement::corner()
		const MathVector<dim>& corner(size_t i) const {return m_vCorner[i];}

	private:
	/// to make it more readable
		enum{POINT = 0, EDGE = 1, FACE = 2};
		enum{MAXOBJECTS = 3};

	/// number of Geometric Objects of a dimension
	
		size_t m_vNum[dim+1];

	/// number of Geometric Objects contained in a (Sub-)Geometric Object of the Element
		size_t m_vSubNum[dim+1][MAXOBJECTS][dim+1];

	///	coordinates of Reference Corner
		MathVector<dim> m_vCorner[num_corners];

	/// indices of GeomObjects
		int m_id[dim+1][MAXOBJECTS][dim+1][MAXOBJECTS];

	///	number of reference elements
		size_t m_vNumRefElem[NUM_REFERENCE_OBJECTS];

	///	type of reference elements
		ReferenceObjectID m_vRefElemType[dim+1][MAXOBJECTS];
};


template <>
template <int TWorldDim>
class ReferenceMapping<ReferenceTriangle, TWorldDim>
{
	public:
	///	world dimension
		static const int worldDim = TWorldDim;

	///	reference dimension
		static const int dim = ReferenceTriangle::dim;

	public:
		ReferenceMapping() : m_vCo(NULL) {}

		void update(const MathVector<worldDim>* corners)
		{
			m_vCo = corners;
			VecSubtract(a10, m_vCo[1], m_vCo[0]);
			VecSubtract(a20, m_vCo[2], m_vCo[0]);
		}

		void local_to_global(	const MathVector<dim>& locPos,
								MathVector<worldDim>& globPos) const
		{
			globPos = m_vCo[0];
			VecScaleAppend(globPos, locPos[0], a10);
			VecScaleAppend(globPos, locPos[1], a20);
		}

		void jacobian_transposed(	const MathVector<dim>& locPos,
									MathMatrix<dim, worldDim>& JT) const
		{
			for(int i = 0; i < worldDim; ++i)
			{
				JT(0, i) = a10[i];
				JT(1, i) = a20[i];
			}
		}

		void jacobian_transposed_inverse(	const MathVector<dim>& locPos,
											MathMatrix<worldDim, dim>& JTInv) const
		{
			MathMatrix<dim, worldDim> JT;

		//	get jacobian transposed
			jacobian_transposed(locPos, JT);

		// 	compute right inverse
			RightInverse(JTInv, JT);
		}

		number jacobian_det(const MathVector<dim>& locPos) const
		{
			return a10[0]*a20[1] - a10[1]*a20[0];
		}

	private:
		const MathVector<worldDim>* m_vCo;

		MathVector<worldDim> a10, a20;

};

}

#endif /* __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TRIANGLE__ */
