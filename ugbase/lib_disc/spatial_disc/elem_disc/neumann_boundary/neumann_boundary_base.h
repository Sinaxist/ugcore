/*
 * neumann_boundary_base.h
 *
 *  Created on: 26.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__NEUMANN_BOUNDARY___NEUMANN_BOUNDARY_BASE__
#define __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__NEUMANN_BOUNDARY___NEUMANN_BOUNDARY_BASE__

// other ug4 modules
#include "common/common.h"

// library intern headers
#include "lib_disc/spatial_disc/elem_disc/elem_disc_interface.h"

namespace ug{

template<typename TDomain>
class NeumannBoundaryBase
	: public IDomainElemDisc<TDomain>
{
	private:
	///	Base class type
		typedef IDomainElemDisc<TDomain> base_type;

	///	Base class type
		typedef NeumannBoundaryBase<TDomain> this_type;

	public:
	///	World dimension
		static const int dim = base_type::dim;

	public:
	///	default constructor
		NeumannBoundaryBase(const char* function);

	///	add a boundary value
	///	\{
		virtual void add(SmartPtr<UserData<number, dim> > data, const char* BndSubsets, const char* InnerSubsets) = 0;
		virtual void add(SmartPtr<UserData<number, dim, bool> > user, const char* BndSubsets, const char* InnerSubsets) = 0;
		virtual void add(SmartPtr<UserData<MathVector<dim>, dim> > user, const char* BndSubsets, const char* InnerSubsets) = 0;
		void add(number val, const char* BndSubsets, const char* InnerSubsets);
		void add(const std::vector<number>& val, const char* BndSubsets, const char* InnerSubsets);
#ifdef UG_FOR_LUA
		void add(const char* name, const char* BndSubsets, const char* InnerSubsets);
#endif
	/// \}

	protected:
	///	base class for user data
		struct Data
		{
			Data(std::string BndSubsets_, std::string InnerSubsets_)
							: BndSubsetNames(BndSubsets_), InnerSubsetNames(InnerSubsets_) {}
			SubsetGroup BndSSGrp;
			std::string BndSubsetNames;
			SubsetGroup InnerSSGrp;
			std::string InnerSubsetNames;
		};

	///	method used to extract subsets id
		void update_subset_groups(Data& userData);

	///	adds subsets to the looped inner subsets
		void add_inner_subsets(const char* InnerSubsets);

	public:
	///	 returns the type of elem disc
		virtual int type() const {return EDT_BND;}

	protected:
	///	dummy add methods
	///	\{
		template<typename TElem, typename TFVGeom>
		void add_JA_elem(LocalMatrix& J, const LocalVector& u) {}
		template<typename TElem, typename TFVGeom>
		void add_JM_elem(LocalMatrix& J, const LocalVector& u) {}
		template<typename TElem, typename TFVGeom>
		void add_dA_elem(LocalVector& d, const LocalVector& u) {}
		template<typename TElem, typename TFVGeom>
		void add_dM_elem(LocalVector& d, const LocalVector& u) {}
	/// \}
};

} // end namespac ug

#endif /*__H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__NEUMANN_BOUNDARY___NEUMANN_BOUNDARY_BASE__*/