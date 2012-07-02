/*
 * local_shape_function_set_impl.h
 *
 *  Created on: 17.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__LOCAL_FINITE_ELEMENT__LOCAL_SHAPE_FUNCTION_SET_IMPL__
#define __H__UG__LIB_DISC__LOCAL_FINITE_ELEMENT__LOCAL_SHAPE_FUNCTION_SET_IMPL__

#include "common/common.h"
#include "local_shape_function_set.h"

namespace ug{

///////////////////////////////////////////
// LocalShapeFunctionSetProvider
///////////////////////////////////////////

template <int dim>
std::vector<std::map<LFEID, const LocalShapeFunctionSet<dim>* > >&
LocalShapeFunctionSetProvider::get_dim_map()
{
//	get type of map
	typedef std::vector<std::map<LFEID, const LocalShapeFunctionSet<dim>*> > VecMap;

//	create static map
	static VecMap sShapeFunctionSetMap(NUM_REFERENCE_OBJECTS);

//	return map
	return sShapeFunctionSetMap;
};

template <int dim>
std::vector<LocalShapeFunctionSet<dim>*>&
LocalShapeFunctionSetProvider::get_dynamic_allocated_vector()
{
//	get type of map
	typedef std::vector<LocalShapeFunctionSet<dim>*> Vec;

//	create static map
	static Vec sShapeFunctionSetMap;

//	return map
	return sShapeFunctionSetMap;
};

template <int dim>
void LocalShapeFunctionSetProvider::
register_set(LFEID type,
             const ReferenceObjectID roid,
             const LocalShapeFunctionSet<dim>& set)
{
//	get type of map
	typedef std::vector<std::map<LFEID, const LocalShapeFunctionSet<dim>* > > VecMap;
	typedef std::map<LFEID, const LocalShapeFunctionSet<dim>* > DimMap;
	VecMap& vDimMap = get_dim_map<dim>();
	DimMap& dimMap = vDimMap[roid];

//	insert into map
	typedef typename DimMap::value_type DimMapPair;
	if(dimMap.insert(DimMapPair(type, &set)).second == false)
		UG_THROW("LocalShapeFunctionSetProvider::register_set(): "
				"Reference type already registered for trial space: "<<type<<" and "
				" Reference element type "<<roid<<".");
}


template <int dim>
bool LocalShapeFunctionSetProvider::
unregister_set(LFEID id)
{
	typedef std::vector<std::map<LFEID, const LocalShapeFunctionSet<dim>* > > VecMap;
	typedef std::map<LFEID, const LocalShapeFunctionSet<dim>* > DimMap;
	VecMap& vDimMap = get_dim_map<dim>();

	for(int r = 0; r < NUM_REFERENCE_OBJECTS; ++r)
	{
		const ReferenceObjectID roid = (ReferenceObjectID) r;

	//	get map
		DimMap& dimMap = vDimMap[roid];

	//	erase element
		if(!(dimMap.erase(id) == 1)) return false;
	}

	return true;
}

template <int dim>
const LocalShapeFunctionSet<dim>&
LocalShapeFunctionSetProvider::
get(ReferenceObjectID roid, LFEID id, bool bCreate)
{
//	get type of map
	typedef std::vector<std::map<LFEID, const LocalShapeFunctionSet<dim>* > > VecMap;
	typedef std::map<LFEID, const LocalShapeFunctionSet<dim>* > Map;

//	init provider and get map
	static VecMap& vMap = inst().get_dim_map<dim>();
	const Map& map = vMap[roid];

//	search for identifier
	typename Map::const_iterator iter = map.find(id);

//	if not found
	if(iter == map.end())
	{
		if(bCreate)
		{
		//	try to create the set
			dynamically_create_set(roid, id);

		//	next try to return the set
			return get<dim>(roid, id, false);
		}

		throw(UGError_LocalShapeFunctionSetNotRegistered(dim, roid, id));
	}

//	return shape function set
	return *(iter->second);
}

}
#endif /* __H__UG__LIB_DISC__LOCAL_FINITE_ELEMENT__LOCAL_SHAPE_FUNCTION_SET_IMPL__ */
