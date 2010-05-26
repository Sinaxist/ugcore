/*
 * dofpattern.h
 *
 *  Created on: 05.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__P1_DOF_MANAGER__DOF_MANAGER__
#define __H__LIBDISCRETIZATION__P1_DOF_MANAGER__DOF_MANAGER__

#include "common/common.h"
#include "lib_grid/lib_grid.h"
#include "lib_algebra/lib_algebra.h"

// trial spaces
#include "lib_discretization/local_shape_function_set/local_dof_pattern.h"
#include "lib_discretization/local_shape_function_set/local_shape_function_set_factory.h"

// multi indices
#include "lib_algebra/multi_index/multi_indices.h"

// dof manager strategies
#include "lib_discretization/dof_manager/dof_manager.h"

namespace ug{

////////////////////////////////////////////////
// P1ConformDoFManager
/// manages the distribution of degrees of freedom for lagrange p1 functions
/**
 * Manages the distribution of DoFs on a Domain, that has been separated into subsets.
 * Given a SubsetHandler, only Lagrange P1 solutions can be added to selected subsets. The
 * LocalShapeFunctionSetID must be LSFS_LAGRANGEP1.
 * All solutions are grouped automatically.
 * Invoking the finalize command let the P1ConformDoFManager distribute the DoFs.
 */
template <typename TGeomObjContainer>
class P1ConformDoFManager : public GridObserver{
	public:
		// type of multiindex used
		typedef MultiIndex<1> index_type;

		// value container for element local indices
		typedef std::vector<index_type> local_index_type;

		// element container type (i.e. grid or subset_handler)
		typedef TGeomObjContainer geom_obj_container_type;

	public:
		/// constructor
		/**
		 * \param[in] name	Name of this P1ConformDoFManager
		 * \param[in] sh	SubsetHandler
		 */
		P1ConformDoFManager(geom_obj_container_type& objContainer);

		/// add a single solution of LocalShapeFunctionSetID to the entire domain (i.e. all elements of the (Multi-)grid)
		/**
		 * \param[in] name			Name of this Single Solution
		 * \param[in] TrialSpace	Trial Space for this function
		 */
		bool add_discrete_function(std::string name, LocalShapeFunctionSetID id, int dim);

		/// add a single solution of LocalShapeFunctionSetID to selected subsets
		/**
		 * \param[in] name			Name of this Single Solution
		 * \param[in] TrialSpace	Trial Space for this function
		 * \param[in] SubsetIndecex	Std::Vector of subset indeces, where this solution lives
		 */
		bool add_discrete_function(std::string name, LocalShapeFunctionSetID id, std::vector<int>& SubsetIndices, int dim);

		/// group single solutions
		/**
		 * By this function a user can group single solutions to a new one. The single solutions will be
		 * removed from the pattern and a new group solution containing those will be added. It is
		 * also possible to group 'Group Solutions'.
		 * The Grouping has an effect on the dof numbering. While single solutions get an own index with only one component for
		 * each dof, a Grouped Solution has on index with several components.
		 *
		 */
		bool group_discrete_functions(std::string name, std::vector<size_t>& GroupSolutions);

		/// groups selected dof-groups associated with one Geom Object
		template <typename TGeomObj>
		bool group_dof_groups(std::vector<size_t>& dofgroups);

		/// groups all dofgroups on a given geometric object type, where a component of selected functions appear
		template <typename TGeomObj>
		bool group_discrete_functions(std::vector<size_t>& selected_functions);

		/// groups all dof on every geometric object for selected functions
		bool group_discrete_functions(std::vector<size_t>& selected_functions);

		/// resets grouping for a geometric object
		template <typename TGeomObj>
		bool reset_grouping();

		/// clears grouping for all geometric objects
		bool reset_grouping();

		/// gives informations about the current status
		bool print_info();

		/// performs a finalizing step. The Pattern can not be altered after finishing
		virtual bool finalize();

		/// returns the trial space of the discrete function fct
		LocalShapeFunctionSetID get_local_shape_function_set_id(size_t fct) const;

		// returns the number of multi_indices on the Element for the discrete function 'fct'
		template<typename TElem>
		std::size_t num_multi_indices(TElem* elem, size_t fct) const;

		/// returns the indices of the dofs on the Element elem for the discrete function 'fct' and returns num_indices
		template<typename TElem>
		std::size_t get_multi_indices(TElem* elem, size_t fct, local_index_type& ind, std::size_t offset = 0) const;

		/// returns the index of the dofs on the Geom Obj for the discrete function 'fct'
		template<typename TElem>
		std::size_t get_multi_indices_of_geom_obj(TElem* vrt, size_t fct, local_index_type& ind, std::size_t offset = 0) const;

		/// returns the number of dofs on level 'level'
		inline size_t num_dofs(size_t level) const {return m_vLevelInfo[level].numDoFIndex;}

		/// returns the number of levels
		inline size_t num_levels() const {return m_objContainer.num_levels();}

		/// returns the number of levels
		inline int num_subsets() const {return m_objContainer.num_subsets();}

		/// returns the number of discrete functions in this dof pattern
		inline size_t num_fct() const {return m_vFunctionInfo.size();}

		/// returns the name of the discrete function nr_fct
		std::string get_name(size_t fct) const {return m_vFunctionInfo[fct].name;}

		/// returns the dimension in which solution lives
		inline int dim_fct(size_t fct) const
		{
			UG_ASSERT(fct < num_fct(), "Accessing fundamental discrete function, that does not exist.");
			return m_vFunctionInfo[fct].dim;
		}

		/// returns true if the discrete function nr_fct is defined on subset s
		bool fct_is_def_in_subset(size_t fct, int subsetIndex) const
		{
			UG_ASSERT(subsetIndex < num_subsets(), "Wrong subset index.");
			UG_ASSERT(fct < num_fct(), "Accessing fundamental discrete function, that does not exist.");

			return true;
		}

		/// number of elments of type 'TElem' on level and subset
		template <typename TElem>
		size_t num(int subsetIndex, size_t level) const
		{
			UG_ASSERT(subsetIndex < num_subsets(), "Wrong subset index " << subsetIndex << " (only " << num_subsets() << " subset present)");
			UG_ASSERT(level < num_levels(), "Accessing level, that does not exist.");
			return m_objContainer.template num<TElem>(subsetIndex, level);
		}

		/// returns the begin iterator for all elements of type 'TElem' on level 'level' on subset 'subsetIndex'
		template <typename TElem>
		typename geometry_traits<TElem>::iterator begin(int subsetIndex, size_t level) const
		{
			UG_ASSERT(subsetIndex < num_subsets(), "Wrong subset index.");
			UG_ASSERT(level < num_levels(), "Accessing level, that does not exist.");
			return m_objContainer.template begin<TElem>(subsetIndex, level);
		}

		/// returns the end iterator for all elements of type 'TElem' on level 'level' on subset 'subsetIndex'
		template <typename TElem>
		typename geometry_traits<TElem>::iterator end(int subsetIndex, size_t level) const
		{
			UG_ASSERT(subsetIndex < num_subsets(), "Wrong subset index.");
			UG_ASSERT(level < num_levels(), "Accessing level, that does not exist.");
			return m_objContainer.template end<TElem>(subsetIndex, level);
		}

		/// returns the assigned SubsetHandler
		geom_obj_container_type& get_assigned_geom_object_container();

		const IndexInfo& get_index_info(size_t level) const
		{
			UG_ASSERT(level < num_levels(), "Accessing level, that does not exist.");
			return m_vLevelInfo[level].indexInfo;
		}

		/// returns if dof manager has a locked pattern
		inline bool is_locked() const {	return m_bLocked;}

		/// destructor
		~P1ConformDoFManager();

	protected:
		struct FunctionInfo
		{
			std::string name;
			int dim;
		};

		struct LevelInfo
		{
			size_t numDoFIndex;
			IndexInfo indexInfo;
		};

		struct SubsetInfo
		{
			typedef ug::Attachment<size_t> ADoF;
			typedef typename geom_obj_container_type::template AttachmentAccessor<VertexBase, ADoF> attachment_accessor_type;

			attachment_accessor_type aaDoF;

			ADoF aDoF;
		};

	protected:
		// if locked, pattern can not be changed anymore
		bool m_bLocked;

		// informations about Functions
		std::vector<FunctionInfo> m_vFunctionInfo;

		// informations about Levels
		std::vector<LevelInfo> m_vLevelInfo;

		// informations about Subsets
		std::vector<SubsetInfo> m_vSubsetInfo;

		// associated object container
		geom_obj_container_type& m_objContainer;

	private:
		// helper function (since partial template specialization for member function not possible)
		template<typename TElem>
		inline std::size_t get_multi_indices_of_geom_obj_helper(TElem* vrt, size_t fct, local_index_type& ind, std::size_t offset = 0) const;

		inline std::size_t get_multi_indices_of_geom_obj_helper(VertexBase* vrt, size_t fct, local_index_type& ind, std::size_t offset = 0) const;

};




}

#include "p1conform_dof_manager_impl.h"


#endif /* __H__LIBDISCRETIZATION__DOF_MANAGER__DOF_MANAGER__ */
