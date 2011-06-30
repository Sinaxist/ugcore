/*
 * reference_hexahedron.h
 *
 *  Created on: 05.08.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_HEXAHEDRON__
#define __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_HEXAHEDRON__

#include "common/math/ugmath.h"
#include "lib_grid/grid/geometric_base_objects.h"
#include "reference_element_mapping.h"

namespace ug{

///	reference element for a hexahedron
class ReferenceHexahedron{
	public:
	///	type of reference element
		static const ReferenceObjectID REFERENCE_OBJECT_ID = ROID_HEXAHEDRON;

	///	dimension of reference element
		static const int dim = 3;

	///	number of corners
		static const int num_corners = 8;

	///	number of eges
		static const int num_edges = 12;

	///	number of faces
		static const int num_faces = 6;

	///	number of volumes
		static const int num_volumes = 1;

	public:
	///	Constructor filling the arrays
		ReferenceHexahedron();

	/// \copydoc ug::ReferenceElement::reference_object_id()
		ReferenceObjectID reference_object_id() const {return REFERENCE_OBJECT_ID;}

	/// \copydoc ug::ReferenceElement::dimension()
		int dimension() const {return dim;}

	/// \copydoc ug::ReferenceElement::size()
		number size() const	{return 1.0;}

	/// \copydoc ug::ReferenceElement::num(int)
		size_t num(int dim) const	{return m_vNum[dim];}

	/// \copydoc ug::ReferenceElement::num(int, size_t, int)
		size_t num(int dim_i, size_t i, int dim_j) const
			{return m_vSubNum[dim_i][i][dim_j];}

	/// \copydoc ug::ReferenceElement::id()
		int id(int dim_i, size_t i, int dim_j, size_t j) const
			{return m_id[dim_i][i][dim_j][j];}

	/// \copydoc ug::ReferenceElement::num_ref_elem()
		size_t num_ref_elem(ReferenceObjectID type) const {return m_vRefElem[type];}

	/// \copydoc ug::ReferenceElement::ref_elem_type()
		ReferenceObjectID ref_elem_type(int dim_i, size_t i) const{	return m_vRefElemType[dim_i][i];}

	/// \copydoc ug::DimReferenceElement::corner()
		const MathVector<dim>& corner(size_t i) const {return m_vCorner[i];}

	private:
	/// to make it more readable
		enum{POINT = 0, EDGE = 1, FACE = 2, VOLUME= 3};
		enum{MAXOBJECTS = 12};

	/// number of Geometric Objects of a dimension
	
		size_t m_vNum[dim+1];

	/// number of Geometric Objects contained in a (Sub-)Geometric Object of the Element
		size_t m_vSubNum[dim+1][MAXOBJECTS][dim+1];

	///	coordinates of Reference Corner
		MathVector<dim> m_vCorner[num_corners];

	/// indices of GeomObjects
		int m_id[dim+1][MAXOBJECTS][dim+1][MAXOBJECTS];

	///	number of reference elements
		size_t m_vRefElem[NUM_REFERENCE_OBJECTS];

	///	type of reference elements
		ReferenceObjectID m_vRefElemType[dim+1][MAXOBJECTS];
};

template <>
template <int TWorldDim>
class ReferenceMapping<ReferenceHexahedron, TWorldDim>
{
	public:
	///	world dimension
		static const int worldDim = TWorldDim;

	///	reference dimension
		static const int dim = ReferenceHexahedron::dim;

	public:
		ReferenceMapping() : m_vCo(NULL)
		{}

		void update(const MathVector<worldDim>* corners)
		{
			m_vCo = corners;
		}

		void local_to_global(	const MathVector<dim>& local,
								MathVector<worldDim>& global) const
		{

			number a,b,c,a0,a1,a2,a3,a4,a5,a6,a7;
			const MathVector<worldDim>* x = m_vCo;
			a = 1.0 - (local)[0];
			b = 1.0 - (local)[1];
			c = 1.0 - (local)[2];
			a0 = a * b * c;
			a1 = (local)[0] * b * c;
			a2 = (local)[0] * (local)[1] * c;
			a3 = a * (local)[1] * c;
			a4 = a * b * (local)[2];
			a5 = (local)[0] * b * (local)[2];
			a6 = (local)[0] * (local)[1] * (local)[2];
			a7 = a * (local)[1] * (local)[2];
			(global)[0] =
			        a0*(x)[0][0]+a1*(x)[1][0]+a2*(x)[2][0]+a3*(x)[3][0]+
			        a4*(x)[4][0]+a5*(x)[5][0]+a6*(x)[6][0]+a7*(x)[7][0];
			(global)[1] =
			        a0*(x)[0][1]+a1*(x)[1][1]+a2*(x)[2][1]+a3*(x)[3][1]+
			        a4*(x)[4][1]+a5*(x)[5][1]+a6*(x)[6][1]+a7*(x)[7][1];
			(global)[2] =
			        a0*(x)[0][2]+a1*(x)[1][2]+a2*(x)[2][2]+a3*(x)[3][2]+
			        a4*(x)[4][2]+a5*(x)[5][2]+a6*(x)[6][2]+a7*(x)[7][2];
		}

		void jacobian_transposed(	const MathVector<dim>& local,
									MathMatrix<dim, worldDim>& JT) const
	   {
			number a,b,c,a0,a1,a2,a3;
			const MathVector<worldDim>* x = m_vCo;
			a = 1.0 - (local)[0];
			b = 1.0 - (local)[1];
			c = 1.0 - (local)[2];
			a0 = b * c;
			a1 = (local)[1] * c;
			a2 = (local)[1] * (local)[2];
			a3 = b * (local)[2];
			JT(0,0) = a0*((x)[1][0]-x[0][0])+a1*((x)[2][0]-(x)[3][0])
						+ a2*((x)[6][0]-x[7][0])+a3*((x)[5][0]-(x)[4][0]);
	        JT(0,1) = a0*((x)[1][1]-x[0][1])+a1*((x)[2][1]-(x)[3][1])
	                    + a2*((x)[6][1]-x[7][1])+a3*((x)[5][1]-(x)[4][1]);
	        JT(0,2) = a0*((x)[1][2]-x[0][2])+a1*((x)[2][2]-(x)[3][2])
	                    + a2*((x)[6][2]-x[7][2])+a3*((x)[5][2]-(x)[4][2]);
	        a0 = a * c;
	        a1 = (local)[0] * c;
	        a2 = (local)[0] * (local)[2];
	        a3 = a * (local)[2];
	        JT(1,0) = a0*((x)[3][0]-x[0][0])+a1*((x)[2][0]-(x)[1][0])
	                    + a2*((x)[6][0]-x[5][0])+a3*((x)[7][0]-(x)[4][0]);
	        JT(1,1) = a0*((x)[3][1]-x[0][1])+a1*((x)[2][1]-(x)[1][1])
	                    + a2*((x)[6][1]-x[5][1])+a3*((x)[7][1]-(x)[4][1]);
	        JT(1,2) = a0*((x)[3][2]-x[0][2])+a1*((x)[2][2]-(x)[1][2])
	                    + a2*((x)[6][2]-x[5][2])+a3*((x)[7][2]-(x)[4][2]);
	        a0 = a * b;
	        a1 = (local)[0] * b;
	        a2 = (local)[0] * (local)[1];
	        a3 = a * (local)[1];
	        JT(2,0) = a0*((x)[4][0]-x[0][0])+a1*((x)[5][0]-(x)[1][0])
	                    + a2*((x)[6][0]-x[2][0])+a3*((x)[7][0]-(x)[3][0]);
	        JT(2,1) = a0*((x)[4][1]-x[0][1])+a1*((x)[5][1]-(x)[1][1])
	                    + a2*((x)[6][1]-x[2][1])+a3*((x)[7][1]-(x)[3][1]);
	        JT(2,2) = a0*((x)[4][2]-x[0][2])+a1*((x)[5][2]-(x)[1][2])
	                    + a2*((x)[6][2]-x[2][2])+a3*((x)[7][2]-(x)[3][2]);
		}

		void jacobian_transposed_inverse(	const MathVector<dim>& locPos,
											MathMatrix<worldDim, dim>& JTInv) const
		{
			MathMatrix<dim, worldDim> JT;

			jacobian_transposed(locPos, JT);

			// compute right inverse
			RightInverse(JTInv, JT);
		}

		number jacobian_det(const MathVector<dim>& locPos) const
		{
			MathMatrix<dim, worldDim> JT;
			jacobian_transposed(locPos, JT);
			if((dim==3) && (worldDim==3))
			{
				const number det
				= JT(0,0)*JT(1,1)*JT(2,2)
				+ JT(0,1)*JT(1,2)*JT(2,0)
				+ JT(0,2)*JT(1,0)*JT(2,1)
				- JT(0,0)*JT(1,2)*JT(2,1)
				- JT(0,1)*JT(1,0)*JT(2,2)
				- JT(0,2)*JT(1,1)*JT(2,0);
				return det;
			}

			UG_ASSERT(0, "Not implemented");
			return 1.0;
		}

	private:
		const MathVector<worldDim>* m_vCo;
};

}

#endif /* __H__LIBDISCRETIZATION__REFERENCE_ELEMENT__REFERENCE_HEXAHEDRON__ */
