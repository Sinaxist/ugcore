/*
 * finite_volume_geometry_impl.h
 *
 *  Created on: 04.09.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIB_DISCRETIZATION__SPACIAL_DISCRETIZATION__DISC_HELPER__FINITE_VOLUME_GEOMETRY_IMPL__
#define __H__LIB_DISCRETIZATION__SPACIAL_DISCRETIZATION__DISC_HELPER__FINITE_VOLUME_GEOMETRY_IMPL__

#include "./finite_volume_geometry.h"

namespace ug{


template <typename TElem, int TWorldDim>
FV1Geometry<TElem, TWorldDim>::
FV1Geometry() : m_pElem(NULL)
{
	// set corners of element as local centers of nodes
	for(size_t i = 0; i < m_rRefElem.num_obj(0); ++i)
		m_locMid[0][i] = m_rRefElem.corner(i);

	// compute local midpoints for all geometric objects with  0 < d <= dim
	for(int d = 1; d <= dim; ++d)
	{
		// loop geometric objects of dimension d
		for(size_t i = 0; i < m_rRefElem.num_obj(d); ++i)
		{
			// set first node
			const size_t coID0 = m_rRefElem.id(d, i, 0, 0);
			m_locMid[d][i] = m_locMid[0][coID0];

			// add corner coordinates of the corners of the geometric object
			for(size_t j = 1; j < m_rRefElem.num_obj_of_obj(d, i, 0); ++j)
			{
				const size_t coID = m_rRefElem.id(d, i, 0, j);
				m_locMid[d][i] += m_locMid[0][coID];
			}

			// scale for correct averaging
			m_locMid[d][i] *= 1./(m_rRefElem.num_obj_of_obj(d, i, 0));
		}
	}

	// set up local informations for SubControlVolumeFaces (scvf)
	// each scvf is associated to one edge of the element
	for(size_t i = 0; i < num_scvf(); ++i)
	{
		m_vSCVF[i].m_from = m_rRefElem.id(1, i, 0, 0);
		m_vSCVF[i].m_to = m_rRefElem.id(1, i, 0, 1);

		// set mid ids
		{
			// start at edge midpoint
			m_vSCVF[i].midId[0] = MidID(1,i);

			// loop up dimension
			if(dim == 2)
			{
				m_vSCVF[i].midId[1] = MidID(dim, 0); // center of element
			}
			else if (dim == 3)
			{
				m_vSCVF[i].midId[1] = MidID(2, m_rRefElem.id(1, i, 2, 0)); // side 0
				m_vSCVF[i].midId[2] = MidID(dim, 0); // center of element
				m_vSCVF[i].midId[3] = MidID(2, m_rRefElem.id(1, i, 2, 1)); // side 1
			}
		}

		// copy local corners of scvf
		copy_local_corners(m_vSCVF[i]);

		// integration point
		AveragePositions(m_vSCVF[i].localIP, m_vSCVF[i].m_vLocPos, SCVF::m_numCorners);
	}

	// set up local informations for SubControlVolumes (scv)
	// each scv is associated to one corner of the element
	for(size_t i = 0; i < num_scv(); ++i)
	{
		m_vSCV[i].nodeId = i;

		if(dim == 1)
		{
			m_vSCV[i].midId[0] = MidID(0, i); // set node as corner of scv
			m_vSCV[i].midId[1] = MidID(dim, 0);	// center of element
		}
		else if(dim == 2)
		{
			m_vSCV[i].midId[0] = MidID(0, i); // set node as corner of scv
			m_vSCV[i].midId[1] = MidID(1, m_rRefElem.id(0, i, 1, 0)); // edge 1
			m_vSCV[i].midId[2] = MidID(dim, 0);	// center of element
			m_vSCV[i].midId[3] = MidID(1, m_rRefElem.id(0, i, 1, 1)); // edge 2
		}
		else if(dim == 3 && (ref_elem_type::REFERENCE_OBJECT_ID != ROID_PYRAMID || i != num_scv()-1))
		{
			m_vSCV[i].midId[0] = MidID(0, i); // set node as corner of scv
			m_vSCV[i].midId[1] = MidID(1, m_rRefElem.id(0, i, 1, 1)); // edge 1
			m_vSCV[i].midId[2] = MidID(2, m_rRefElem.id(0, i, 2, 0)); // face 0
			m_vSCV[i].midId[3] = MidID(1, m_rRefElem.id(0, i, 1, 0)); // edge 0
			m_vSCV[i].midId[4] = MidID(1, m_rRefElem.id(0, i, 1, 2)); // edge 2
			m_vSCV[i].midId[5] = MidID(2, m_rRefElem.id(0, i, 2, 2)); // face 2
			m_vSCV[i].midId[6] = MidID(dim, 0);	// center of element
			m_vSCV[i].midId[7] = MidID(2, m_rRefElem.id(0, i, 2, 1)); // face 1
		}
		// TODO: Implement last ControlVolume for Pyramid
		else if(dim == 3 && ref_elem_type::REFERENCE_OBJECT_ID == ROID_PYRAMID && i == num_scv()-1)
		{
			// this scv has 10 corners
			m_vSCV[i].m_numCorners = 10;
			UG_ASSERT(0, "Last SCV for Pyramid must be implemented");
		}
		else {UG_ASSERT(0, "Dimension higher that 3 not implemented.");}

		// copy local corners of scv
		copy_local_corners(m_vSCV[i]);
	}

	/////////////////////////
	// Shapes and Derivatives
	/////////////////////////
	for(size_t i = 0; i < num_scvf(); ++i)
	{
		const LocalShapeFunctionSet<ref_elem_type>& TrialSpace =
				LocalShapeFunctionSetProvider::
					get_local_shape_function_set<ref_elem_type>
					(LocalShapeFunctionSetID(LocalShapeFunctionSetID::LAGRANGE, 1));

		const size_t num_sh = m_numSCV;
		m_vSCVF[i].vShape.resize(num_sh);
		m_vSCVF[i].localGrad.resize(num_sh);
		m_vSCVF[i].globalGrad.resize(num_sh);

		TrialSpace.shapes(&(m_vSCVF[i].vShape[0]), m_vSCVF[i].localIP);
		TrialSpace.grads(&(m_vSCVF[i].localGrad[0]), m_vSCVF[i].localIP);
	}
}


/// update data for given element
template <typename TElem, int TWorldDim>
bool
FV1Geometry<TElem, TWorldDim>::
update(TElem* elem, const ISubsetHandler& ish, const MathVector<world_dim>* vCornerCoords)
{
// 	If already update for this element, do nothing
	if(m_pElem == elem) return true;
	else m_pElem = elem;

// 	remember global position of nodes
	for(size_t i = 0; i < m_rRefElem.num_obj(0); ++i)
		m_gloMid[0][i] = vCornerCoords[i];

// 	compute global midpoints for all geometric objects with  0 < d <= dim
	for(int d = 1; d <= dim; ++d)
	{
	// 	loop geometric objects of dimension d
		for(size_t i = 0; i < m_rRefElem.num_obj(d); ++i)
		{
		// 	set first node
			const size_t coID0 = m_rRefElem.id(d, i, 0, 0);
			m_gloMid[d][i] = m_gloMid[0][coID0];

		// 	add corner coordinates of the corners of the geometric object
			for(size_t j = 1; j < m_rRefElem.num_obj_of_obj(d, i, 0); ++j)
			{
				const size_t coID = m_rRefElem.id(d, i, 0, j);
				m_gloMid[d][i] += m_gloMid[0][coID];
			}

		// 	scale for correct averaging
			m_gloMid[d][i] *= 1./(m_rRefElem.num_obj_of_obj(d, i, 0));
		}
	}

// 	compute global informations for scvf
	for(size_t i = 0; i < num_scvf(); ++i)
	{
	// 	copy local corners of scvf
		copy_global_corners(m_vSCVF[i]);

	// 	integration point
		AveragePositions(m_vSCVF[i].globalIP, m_vSCVF[i].m_vGloPos, SCVF::m_numCorners);

	// 	normal on scvf
		if(ref_elem_type::dim == world_dim)
		{
			NormalOnSCVF<ref_elem_type, world_dim>(m_vSCVF[i].Normal, m_vSCVF[i].m_vGloPos);
		}
		else
		{
			if(ref_elem_type::dim == 1)
				NormalOnSCVF<ref_elem_type, world_dim>(m_vSCVF[i].Normal, vCornerCoords);
			else
				throw(UGError("Not Implemented"));
		}
	}

// 	compute size of scv
	for(size_t i = 0; i < num_scv(); ++i)
	{
	// 	copy global corners
		copy_global_corners(m_vSCV[i]);

	// 	compute volume of scv
		if(m_vSCV[i].m_numCorners != 10)
		{
			m_vSCV[i].vol = ElementSize<scv_type, world_dim>(m_vSCV[i].m_vGloPos);
		}
		else
		{
			// special case for pyramid, last scv
		}
	}

	/////////////////////////
	// Shapes and Derivatives
	/////////////////////////


	m_rMapping.update(vCornerCoords);

	for(size_t i = 0; i < num_scvf(); ++i)
	{
		if(!m_rMapping.jacobian_transposed_inverse(m_vSCVF[i].localIP, m_vSCVF[i].JtInv))
			{UG_LOG("Cannot compute jacobian transposed.\n"); return false;}
		if(!m_rMapping.jacobian_det(m_vSCVF[i].localIP, m_vSCVF[i].detj))
			{UG_LOG("Cannot compute jacobian determinate.\n"); return false;}

		for(size_t sh = 0 ; sh < num_scv(); ++sh)
			MatVecMult((m_vSCVF[i].globalGrad)[sh], m_vSCVF[i].JtInv, (m_vSCVF[i].localGrad)[sh]);
	}


	/////////////////////////
	// Boundary Faces
	/////////////////////////

//	if no boundary subsets required, return
	if(num_boundary_subsets() == 0) return true;

//	get grid
	Grid& grid = *ish.get_assigned_grid();

//	vector of subset indices of side
	std::vector<int> vSubsetIndex;

//	get subset indices for sides (i.e. edge in 2d, faces in 3d)
	if(dim == 2)
	{
		std::vector<EdgeBase*> vEdges;
		CollectEdgesSorted(vEdges, grid, elem);

		vSubsetIndex.resize(vEdges.size());
		for(size_t i = 0; i < vEdges.size(); ++i)
		{
			vSubsetIndex[i] = ish.get_subset_index(vEdges[i]);
		}
	}

	if(dim == 3)
	{
		std::vector<Face*> vFaces;
		CollectFacesSorted(vFaces, grid, elem);

		vSubsetIndex.resize(vFaces.size());
		for(size_t i = 0; i < vFaces.size(); ++i)
		{
			vSubsetIndex[i] = ish.get_subset_index(vFaces[i]);
		}
	}

//	loop requested subset
	typename std::map<int, std::vector<BF> >::iterator it;
	for (it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); ++it)
	{
	//	get subset index
		const int bndIndex = (*it).first;

	//	get vector of BF for element
		std::vector<BF>& vBF = (*it).second;

	//	clear vector
		vBF.clear();

	//	current number of bf
		size_t curr_bf = 0;

	//	loop sides of element
		for(size_t side = 0; side < vSubsetIndex.size(); ++side)
		{
		//	skip non boundary sides
			if(vSubsetIndex[side] != bndIndex) continue;

		///////////////////////////
		//	add Boundary faces
		///////////////////////////

		//	number of corners of side
			const int coOfSide = m_rRefElem.num_obj_of_obj(dim-1, side, 0);

		//	resize vector
			vBF.resize(curr_bf + coOfSide);

		//	loop corners
			for(int co = 0; co < coOfSide; ++co)
			{
			//	get current bf
				BF& bf = vBF[curr_bf];

			//	set node id == scv this bf belongs to
				bf.m_nodeId = m_rRefElem.id(dim-1, side, 0, co);

			// 	set mid ids
				if(dim == 2)
				{
					bf.midId[co%2] = MidID(0, m_rRefElem.id(1, side, 0, co)); // corner of side
					bf.midId[(co+1)%2] = MidID(1, side); // side midpoint
				}
				else if (dim == 3)
				{
					bf.midId[0] = MidID(0, m_rRefElem.id(2, side, 0, co)); // corner of side
					bf.midId[1] = MidID(1, m_rRefElem.id(2, side, 1, co)); // edge co
					bf.midId[2] = MidID(2, side); // side midpoint
					bf.midId[3] = MidID(1, m_rRefElem.id(2, side, 1, (co -1 + coOfSide)%coOfSide)); // edge co-1
				}

			// 	copy corners of bf
				copy_local_corners(bf);
				copy_global_corners(bf);

			// 	integration point
				AveragePositions(bf.localIP, bf.m_vLocPos, SCVF::m_numCorners);
				AveragePositions(bf.globalIP, bf.m_vGloPos, SCVF::m_numCorners);

			// 	normal on scvf
				NormalOnSCVF<ref_elem_type, world_dim>(bf.Normal, bf.m_vGloPos);

			//	compute volume
				bf.m_volume = VecTwoNorm(bf.Normal);

			/////////////////////////
			// Shapes and Derivatives
			/////////////////////////
				const LocalShapeFunctionSet<ref_elem_type>& TrialSpace =
						LocalShapeFunctionSetProvider::
							get_local_shape_function_set<ref_elem_type>
								(LocalShapeFunctionSetID(LocalShapeFunctionSetID::LAGRANGE, 1));

				const size_t num_sh = m_numSCV;
				bf.vShape.resize(num_sh);
				bf.localGrad.resize(num_sh);
				bf.globalGrad.resize(num_sh);

				TrialSpace.shapes(&(bf.vShape[0]), bf.localIP);
				TrialSpace.grads(&(bf.localGrad[0]), bf.localIP);

				if(!m_rMapping.jacobian_transposed_inverse(bf.localIP, bf.JtInv))
					{UG_LOG("Cannot compute jacobian transposed.\n"); return false;}
				if(!m_rMapping.jacobian_det(bf.localIP, bf.detj))
					{UG_LOG("Cannot compute jacobian determinate.\n"); return false;}

				for(size_t sh = 0 ; sh < num_scv(); ++sh)
					MatVecMult((bf.globalGrad)[sh], bf.JtInv, (bf.localGrad)[sh]);

			//	increase curr_bf
				++curr_bf;
			}
		}
	}

	return true;
}





} // end namespace ug

#endif /* __H__LIB_DISCRETIZATION__SPACIAL_DISCRETIZATION__DISC_HELPER__FINITE_VOLUME_GEOMETRY_IMPL__ */
