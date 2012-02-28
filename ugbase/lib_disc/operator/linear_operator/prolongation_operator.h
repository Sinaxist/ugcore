/*
 * prolongation_operator.h
 *
 *  Created on: 04.12.2009
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR__
#define __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR__

// extern headers
#include <iostream>

// other ug4 modules
#include "common/common.h"
#include "lib_grid/lg_base.h"
#include "transfer_interface.h"
#include "lib_disc/spatial_disc/constraints/constraint_interface.h"

#ifdef UG_PARALLEL
#include "lib_disc/parallelization/parallelization_util.h"
#endif

namespace ug{

/**
 * This functions assembles the interpolation matrix between to
 * grid levels using only the Vertex degrees of freedom.
 *
 * \param[out]	mat 			Assembled interpolation matrix that interpolates u -> v
 * \param[in] 	approxSpace		Approximation Space
 * \param[in]	coarseLevel		Coarse Level index
 * \param[in]	fineLevel		Fine Level index
 */
template <typename TDD, typename TAlgebra>
void AssembleVertexProlongation(typename TAlgebra::matrix_type& mat,
                                const TDD& coarseDD, const TDD& fineDD,
								std::vector<bool>& vIsRestricted);


///	Prologation Operator for P1 Approximation Spaces
template <typename TDomain, typename TAlgebra>
class P1Prolongation :
	virtual public IProlongationOperator<TAlgebra>
{
	public:
	///	Type of algebra
		typedef TAlgebra algebra_type;

	///	Type of Vector
		typedef typename TAlgebra::vector_type vector_type;

	///	Type of Vector
		typedef typename TAlgebra::matrix_type matrix_type;

	///	Type of Domain
		typedef TDomain domain_type;

	public:
	/// Default constructor
		P1Prolongation() : m_bInit(false), m_dampRes(1.0) {clear_constraints();};

	///	Constructor setting approximation space
		P1Prolongation(SmartPtr<ApproximationSpace<TDomain> > approxSpace) :
			m_spApproxSpace(approxSpace), m_bInit(false), m_dampRes(1.0)
		{clear_constraints();};

	///	clears dirichlet post processes
		void clear_constraints() {m_vConstraint.clear();}

	///	adds a dirichlet post process (not added if already registered)
		void add_constraint(IConstraint<algebra_type>& pp);

	///	removes a post process
		void remove_constraint(IConstraint<algebra_type>& pp);

	///	Set approximation space
		void set_approximation_space(SmartPtr<ApproximationSpace<TDomain> > approxSpace);

	///	set interpolation damping
		void set_restriction_damping(number damp) {m_dampRes = damp;}

	///	Set levels
		virtual void set_levels(GridLevel coarseLevel, GridLevel fineLevel);

	public:
	///	initialize the operator
		virtual void init(const vector_type& u){init();}

	///	initialize the operator
		virtual void init();

	/// apply Operator, interpolate function
		virtual void apply(vector_type& uFineOut, const vector_type& uCoarseIn);

	/// apply transposed Operator, restrict function
		void apply_transposed(vector_type& uCoarseOut, const vector_type& uFineIn);

	/// apply Operator, i.e. v := v - L(u);
		virtual void apply_sub(vector_type& u, const vector_type& v) {UG_THROW_FATAL("Not Implemented.");}

	///	returns new instance with same setting
		virtual IProlongationOperator<TAlgebra>* clone();

	///	Destructor
		~P1Prolongation() {}

	protected:
	///	matrix to store prolongation
		matrix_type m_matrix;

	///	list of post processes
		std::vector<IConstraint<algebra_type>*> m_vConstraint;

	///	approximation space
		SmartPtr<ApproximationSpace<TDomain> > m_spApproxSpace;

	///	fine grid level
		GridLevel m_fineLevel;

	///	coarse grid level
		GridLevel m_coarseLevel;

	///	restriction flag
		std::vector<bool> m_vIsRestricted;

	///	initialization flag
		bool m_bInit;

	///	damping parameter
		number m_dampRes;
};

} // end namespace ug

#include "prolongation_operator_impl.h"

#endif /* __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__PROLONGATION_OPERATOR__ */
