/*
 * cg.h
 *
 *  Created on: 22.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__OPERATOR__LINEAR_SOLVER__CG__
#define __H__UG__LIB_DISC__OPERATOR__LINEAR_SOLVER__CG__

#include <iostream>
#include <sstream>

#include "lib_algebra/operator/operator_interface.h"
#include "common/profiler/profiler.h"
#ifdef UG_PARALLEL
	#include "lib_algebra/parallelization/parallelization.h"
#endif

namespace ug{

///	the CG method as a solver for linear operators
/**
 * This class implements the CG - method for the solution of linear operator
 * problems like A*x = b, where the solution x = A^{-1} b is computed.
 *
 * For detailed description of the algorithm, please refer to:
 *
 * - Barrett, Berry, Chan, Demmel, Donatom Dongarra, Eijkhout, Pozo, Romine,
 * 	 Van der Vorst, "Templates for the Solution of Linear Systems: Building
 * 	 Blocks for Iterative Methods", p.13, Fig, 2.5
 *
 * - Saad, "Iterative Methods For Sparse Linear Systems", p277, Alg. 9.1
 *
 * \tparam 	TAlgebra		algebra type
 */
template <typename TAlgebra>
class CG : public ILinearOperatorInverse<	typename TAlgebra::vector_type,
													typename TAlgebra::vector_type>
{
	public:
	///	Algebra type
		typedef TAlgebra algebra_type;

	///	Vector type
		typedef typename TAlgebra::vector_type vector_type;

	public:
	///	default constructor
		CG() : m_pPrecond(NULL), m_pConvCheck(NULL){}

	///	constructor setting preconditioner and convergence check
		CG( ILinearIterator<vector_type,vector_type>* Precond,
		    IConvergenceCheck& ConvCheck) :
		m_pPrecond(Precond), m_pConvCheck(&ConvCheck)
		{};

	///	name of solver
		virtual const char* name() const {return "CG";}

	///	sets the convergence check
		void set_convergence_check(IConvergenceCheck& convCheck)
		{
			m_pConvCheck = &convCheck;
			m_pConvCheck->set_offset(3);
		}

	///	returns the convergence check
		IConvergenceCheck* get_convergence_check() {return m_pConvCheck;}

	///	sets the preconditioner
		void set_preconditioner(ILinearIterator<vector_type, vector_type>& precond)
		{
			m_pPrecond = &precond;
		}

	///	initializes the solver
		virtual bool init(ILinearOperator<vector_type, vector_type>& J, const vector_type& u)
		{
		//	remember operator
			m_A = &J;

		// 	init Preconditioner for operator A
			if(m_pPrecond)
				if(!m_pPrecond->init(J, u))
				{
					UG_LOG("ERROR in 'CG::init': "
							"Cannot init Iterator Operator for Operator A.\n");
					return false;
				}

		//	done
			return true;
		}

	///	initializes the solver
		virtual bool init(ILinearOperator<vector_type, vector_type>& L)
		{
		//	remember operator
			m_A = &L;

		// 	init Preconditioner for operator A
			if(m_pPrecond)
				if(!m_pPrecond->init(L))
				{
					UG_LOG("ERROR in 'CG::init': "
							"Cannot init Iterator Operator for Operator A.\n");
					return false;
				}

		//	done
			return true;
		}

	///	Solve J(u)*x = b, such that x = J(u)^{-1} b
		virtual bool apply_return_defect(vector_type& x, vector_type& b)
		{
		//	convergence check required
			if(m_pConvCheck == NULL)
			{
				UG_LOG("ERROR: In 'CG::apply_return_defect': "
						"Convergence check not set.\n");
				return false;
			}

		//	check parallel storage types
			#ifdef UG_PARALLEL
			if(!b.has_storage_type(PST_ADDITIVE) || !x.has_storage_type(PST_CONSISTENT))
				{
					UG_LOG("ERROR: In 'CG::apply_return_defect':"
							"Inadequate storage format of Vectors.\n");
					return false;
				}
			#endif

		// 	rename r as b (for convenience)
			vector_type& r = b;

		// 	Build defect:  r := b - J(u)*x
			m_A->apply_sub(r, x);

		// 	create help vector (h will be consistent r)
		//	todo: 	It would be sufficient to copy only the pattern and
		//			without initializing, but in parallel we have to copy communicators
			vector_type q; q.create(r.size()); q = r;
			vector_type z; z.create(x.size()); z = x;
			vector_type p; p.create(x.size()); p = x;

		// 	Preconditioning
			if(m_pPrecond)
			{
				// apply z = M^-1 * s
				if(!m_pPrecond->apply(z, r))
				{
					UG_LOG("ERROR in 'CG::apply_return_defect': "
							"Cannot apply preconditioner. Aborting.\n");
					return false;
				}
			}
			else z = r;


		// 	make z consistent
			#ifdef UG_PARALLEL
			if(!z.change_storage_type(PST_CONSISTENT))
			{
				UG_LOG("ERROR in 'CG::apply_return_defect': "
						"Cannot convert z to consistent vector.\n");
				return false;
			}
			#endif

		//	compute start defect
			prepare_conv_check();
			m_pConvCheck->start(r);

		// 	start search direction
			p = z;

		// 	start rho
			number rhoOld = VecProd(z, r), rho;

		// 	Iteration loop
			while(!m_pConvCheck->iteration_ended())
			{
			// 	Build q = A*p (q is additive afterwards)
				m_A->apply(q, p);

			// 	lambda = (q,p)
				const number lambda = VecProd(q, p);

			//	check lambda
				if(lambda == 0.0)
				{
					UG_LOG("ERROR in 'CG::apply_return_defect': lambda=" <<
					       lambda<< " is not admitted. Aborting solver.\n");
					return false;
				}

			//	alpha = rho / (q,p)
				const number alpha = rhoOld/lambda;

			// 	Update x := x + alpha*p
				VecScaleAdd(x, 1.0, x, alpha, p);

			// 	Update r := r - alpha*t
				VecScaleAdd(r, 1.0, r, -alpha, q);

			// 	Check convergence
				m_pConvCheck->update(r);
				if(m_pConvCheck->iteration_ended()) break;

			// 	Preconditioning
				if(m_pPrecond)
				{
				// 	apply z = M^-1 * r
					if(!m_pPrecond->apply(z, r))
					{
						UG_LOG("ERROR in 'CG::apply_return_defect': "
								"Cannot apply preconditioner. Aborting.\n");
						return false;
					}
				}
				else z = r;

				#ifdef UG_PARALLEL
			// 	make z consistent
				if(!z.change_storage_type(PST_CONSISTENT))
				{
					UG_LOG("ERROR in 'CG::apply_return_defect': "
							"Cannot convert z to consistent vector.\n");
					return false;
				}
				#endif

			// 	new rho = (z,r)
				rho = VecProd(z, r);

			// 	new beta = rho / rhoOld
				const number beta = rho/rhoOld;

			// 	new direction p := beta * p + z
				VecScaleAdd(p, beta, p, 1.0, z);

			// 	remember old rho
				rhoOld = rho;
			}

		//	post output
			return m_pConvCheck->post();
		}

	///	solves J(u)*x = b
		virtual bool apply(vector_type& x, const vector_type& b)
		{
		//	copy defect
			vector_type bTmp; bTmp.create(b.size());
			bTmp = b;

		//	solve on copy of defect
			return apply_return_defect(x, bTmp);
		}

	/// destructor
		virtual ~CG() {};

	protected:
	///	adjust output of convergence check
		void prepare_conv_check()
		{
		//	set iteration symbol an name
			m_pConvCheck->set_name(name());
			m_pConvCheck->set_symbol('%');
			m_pConvCheck->set_name(name());

		//	set preconditioner string
			std::stringstream ss;
			if(m_pPrecond) ss<<" (Precond: "<<m_pPrecond->name()<<")";
			else ss << " (No Preconditioner) ";
			m_pConvCheck->set_info(ss.str());
		}

	protected:
		number VecProd(vector_type& a, vector_type& b)
		{
			return a.dotprod(b);
		}

	protected:
	/// Operator that is inverted by this Inverse Operator
		ILinearOperator<vector_type,vector_type>* m_A;

	///	Preconditioner
		ILinearIterator<vector_type,vector_type>* m_pPrecond;

	/// Convergence Check
		IConvergenceCheck* m_pConvCheck;
};

} // end namespace ug

#endif /* __H__UG__LIB_DISC__OPERATOR__LINEAR_SOLVER__CG__ */
