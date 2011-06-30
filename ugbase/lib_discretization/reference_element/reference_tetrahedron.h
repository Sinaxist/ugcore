/*
 * reference_tetrahedron.h
 *
 *  Created on: 15.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TETRAHEDRON__
#define __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TETRAHEDRON__

#include "common/math/ugmath.h"
#include "lib_grid/grid/geometric_base_objects.h"
#include "reference_element_mapping.h"

namespace ug{

class ReferenceTetrahedron{
	public:
	///	type of reference element
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_TETRAHEDRON;

	///	dimension of reference element
		static const int dim = 3;

	///	number of corners
		static const int num_corners = 4;

	///	number of eges
		static const int num_edges = 6;

	///	number of faces
		static const int num_faces = 4;

	///	number of volumes
		static const int num_volumes = 1;

	public:
	///	Constructor
		ReferenceTetrahedron();

	/// \copydoc ug::ReferenceElement::reference_object_id()
		ReferenceObjectID reference_object_id() const {return REFERENCE_OBJECT_ID;}

	/// \copydoc ug::ReferenceElement::dimension()
		int dimension() const {return dim;}

	/// \copydoc ug::ReferenceElement::size()
		number size() const	{return 1.0/6.0;}

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
		enum{POINT = 0, EDGE = 1, FACE = 2, VOLUME= 3};
		enum{MAXOBJECTS = 6};

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
class ReferenceMapping<ReferenceTetrahedron, TWorldDim>
{
	public:
	///	world dimension
		static const int worldDim = TWorldDim;

	///	reference dimension
		static const int dim = ReferenceTetrahedron::dim;

	public:
		ReferenceMapping() : m_vCo(NULL) {}

		void update(const MathVector<worldDim>* corners)
		{
			m_vCo = corners;
			VecSubtract(a10, m_vCo[1], m_vCo[0]);
			VecSubtract(a20, m_vCo[2], m_vCo[0]);
			VecSubtract(a30, m_vCo[3], m_vCo[0]);
		}

		void local_to_global(	const MathVector<dim>& locPos,
								MathVector<worldDim>& globPos) const
		{
			globPos = m_vCo[0];
			VecScaleAppend(globPos, locPos[0], a10);
			VecScaleAppend(globPos, locPos[1], a20);
			VecScaleAppend(globPos, locPos[1], a30);
		}

		void jacobian_transposed(	const MathVector<dim>& locPos,
									MathMatrix<dim, worldDim>& JT) const
		{
			for(int i = 0; i < worldDim; ++i)
			{
				JT[0][i] = a10[i];
				JT[1][i] = a20[i];
				JT[2][i] = a30[i];
			}
		}

		void jacobian_transposed_inverse(	const MathVector<dim>& locPos,
											MathMatrix<worldDim, dim>& JTInv) const
		{
			MathMatrix<dim, worldDim> JT;

			jacobian_transposed(locPos, JT);

		// 	compute right inverse
			RightInverse(JTInv, JT);
		}

		number jacobian_det(const MathVector<dim>& locPos) const
		{
			MathMatrix<dim, worldDim> JT;

		//	compute jacobian transposed
			jacobian_transposed(locPos, JT);

		//	only in quad case defined
			if((dim==3) && (worldDim==3))
			{
				const number det =
				JT(0,0)*JT(1,1)*JT(2,2)
				+ JT(0,1)*JT(1,2)*JT(2,0)
				+ JT(0,2)*JT(1,0)*JT(2,1)
				- JT(0,0)*JT(1,2)*JT(2,1)
				- JT(0,1)*JT(1,0)*JT(2,2)
				- JT(0,2)*JT(1,1)*JT(2,0);
				return det;
			}

			UG_ASSERT(0, "Not implemented.");
			return 0.0;
		}

	private:
		const MathVector<worldDim>* m_vCo;

		MathVector<worldDim> a10, a20, a30;
};


}

#endif /* __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_TETRAHEDRON__ */
