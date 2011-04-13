/*
 * lib_algebra_bridge.cpp
 *
 *  Created on: 11.10.2010
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>

// bridge
#include "ug_bridge/ug_bridge.h"

// algebra includes
#include "lib_algebra_bridge.h"

// \todo: extract only really needed includes
// all parts of lib algebra
#include "lib_algebra/lib_algebra.h"
#include "lib_algebra/operator/operator_impl.h"


namespace ug
{

namespace bridge
{

//! calculates dest = alpha1*v1 + alpha2*v2
template<typename vector_t>
inline void VecScaleAdd2(vector_t &dest, double alpha1, const vector_t &v1, double alpha2, const vector_t &v2)
{
	VecScaleAdd(dest, alpha1, v1, alpha2, v2);
}

//! calculates dest = alpha1*v1 + alpha2*v2 + alpha3*v3
template<typename vector_t>
inline void VecScaleAdd3(vector_t &dest, double alpha1, const vector_t &v1, double alpha2, const vector_t &v2, double alpha3, const vector_t &v3)
{
	VecScaleAdd(dest, alpha1, v1, alpha2, v2, alpha3, v3);
}

template <size_t N>
void TestInverse()
{
		int i;

		i+= 2;

		UG_LOG("i="<<i<<"\n");

	//	a fixed size matrix
		DenseMatrix< FixedArray2<number, N, N> > mat;

	//	reset all values of the matrix to zero
		mat = 0.0;

	//	we now create a matrix, where we store the inverse matrix
		typename block_traits<DenseMatrix< FixedArray2<number, N, N> > >::inverse_type inv;

	//	get the inverse
		GetInverse(inv, mat);

		UG_LOG("Inv(0,0) = "<< inv(0,0) << "\n");

	//	invert the system for all contributions
		DenseVector< FixedArray1<number, N> > x, f;

		MatMult(x, 1.0, inv, f);

}

template <typename TAlgebra>
struct cRegisterAlgebraType
{
	static bool reg(Registry& reg, const char* parentGroup)
	{
	//	get group string (use same as parent)
		std::string grp = std::string(parentGroup);

	//	typedefs for this algebra
		typedef TAlgebra algebra_type;
		typedef typename algebra_type::vector_type vector_type;
		typedef typename algebra_type::matrix_type matrix_type;


		//	Vector
		{
				reg.add_class_<vector_type>("Vector", grp.c_str())
				.add_constructor()
				.add_method("set|hide=true", (bool (vector_type::*)(number))&vector_type::set,
										"Success", "Number")
				.add_method("size|hide=true", (size_t (vector_type::*)())&vector_type::size,
										"Size", "")
				.add_method("set_random|hide=true", (bool (vector_type::*)(number))&vector_type::set_random,
										"Success", "Number")
				.add_method("print|hide=true", &vector_type::p);

				reg.add_function("VecScaleAssign", (void (*)(vector_type&, number, const vector_type &))&VecScaleAssign<vector_type>);
				reg.add_function("VecScaleAdd2", /*(void (*)(vector_type&, number, const vector_type&, number, const vector_type &)) */
						&VecScaleAdd2<vector_type>, "", "alpha1*vec1 + alpha2*vec2",
						"dest#alpha1#vec1#alpha2#vec2");
				reg.add_function("VecScaleAdd3", /*(void (*)(vector_type&, number, const vector_type&, number, const vector_type &, number, const vector_type &))*/
						&VecScaleAdd3<vector_type>, "", "alpha1*vec1 + alpha2*vec2 + alpha3*vec3",
						"dest#alpha1#vec1#alpha2#vec2#alpha3#vec3");
		}

		//	Matrix
		{
			reg.add_class_<matrix_type>("Matrix", grp.c_str())
				.add_constructor()
				.add_method("print|hide=true", &matrix_type::p);
		}


		//	ApplyLinearSolver
		{
			reg.add_function( "ApplyLinearSolver",
							  &ApplyLinearSolver<vector_type>, grp.c_str());
		}


		// Debug Writer (abstract base class)
			{
				typedef IDebugWriter<algebra_type> T;
				reg.add_class_<T>("IDebugWriter", grp.c_str());
			}

		// IPositionProvider (abstract base class)
		{
			reg.add_class_<IPositionProvider<1> >("IPositionProvider1d", grp.c_str());
			reg.add_class_<IPositionProvider<2> >("IPositionProvider2d", grp.c_str());
			reg.add_class_<IPositionProvider<3> >("IPositionProvider3d", grp.c_str());
		}

		// IVectorWriter (abstract base class)
		{
			typedef IVectorWriter<vector_type> T;
			reg.add_class_<T>("IVectorWriter", grp.c_str())
					.add_method("update", &T::update, "", "v", "updates the vector v");
		}


		// Base Classes
		{
			{
			//	ILinearOperator
				typedef ILinearOperator<vector_type, vector_type> T;
				reg.add_class_<T>("ILinearOperator", grp.c_str())
					.add_method("init", (bool(T::*)())&T::init);
			}

			{
			//	IMatrixOperator
				typedef ILinearOperator<vector_type, vector_type> TBase;
				typedef IMatrixOperator<vector_type, vector_type, matrix_type> T;
				reg.add_class_<T, TBase>("IMatrixOperator", grp.c_str())
					.add_method("resize", &T::resize)
					.add_method("num_rows", &T::num_rows, "rows")
					.add_method("num_cols", &T::num_cols, "cols")
					.add_method("apply", &TBase::apply);

			}

			{
			//	PureMatrixOperator
				typedef IMatrixOperator<vector_type, vector_type, matrix_type> TBase;
				typedef PureMatrixOperator<vector_type, vector_type, matrix_type> T;
				reg.add_class_<T, TBase>("PureMatrixOperator", grp.c_str())
					.add_constructor()
					.add_method("get_matrix", &T::get_matrix);
			}

			{
			//	ILinearIterator
				typedef ILinearIterator<vector_type, vector_type> T;
				reg.add_class_<T>("ILinearIterator", grp.c_str());
			}

			{
			//	IPreconditioner
				typedef ILinearIterator<vector_type, vector_type>  TBase;
				typedef IPreconditioner<algebra_type> T;
				reg.add_class_<T, TBase>("IPreconditioner", grp.c_str());
			}

			{
			//	ILinearOperatorInverse
				typedef ILinearOperatorInverse<vector_type, vector_type> T;
				reg.add_class_<T>("ILinearOperatorInverse", grp.c_str())
					.add_method("init", (bool(T::*)(ILinearOperator<vector_type,vector_type>&))&T::init)
					.add_method("apply_return_defect", &T::apply_return_defect, "Success", "u#f",
							"Solve A*u = f, such that u = A^{-1} f by iterating u := u + B(f - A*u),  f := f - A*u becomes new defect")
					.add_method("apply", &T::apply, "Success", "u#f", "Solve A*u = f, such that u = A^{-1} f by iterating u := u + B(f - A*u), f remains constant");
			}

			{
			//	IMatrixOperatorInverse
				typedef ILinearOperatorInverse<vector_type, vector_type>  TBase;
				typedef IMatrixOperatorInverse<vector_type, vector_type, matrix_type> T;
				reg.add_class_<T, TBase>("IMatrixOperatorInverse", grp.c_str());
			}

			{
			//	IOperator
				typedef IOperator<vector_type, vector_type> T;
				reg.add_class_<T>("IOperator", grp.c_str());
			}

			{
			//	IOperatorInverse
				typedef IOperatorInverse<vector_type, vector_type> T;
				reg.add_class_<T>("IOperatorInverse", grp.c_str());
			}

			{
			//	IProlongationOperator
				typedef IProlongationOperator<vector_type, vector_type> T;
				reg.add_class_<T>("IProlongationOperator", grp.c_str());
			}

			{
			//	IProjectionOperator
				typedef IProjectionOperator<vector_type, vector_type> T;
				reg.add_class_<T>("IProjectionOperator", grp.c_str());
			}
		}

		// Preconditioner
		{
		//	get group string
			std::stringstream grpSS2; grpSS2 << grp << "/Preconditioner";
			std::string grp2 = grpSS2.str();

		//	Jacobi
			reg.add_class_<	JacobiPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("Jacobi", grp2.c_str(), "Jacobi Preconditioner")
				.add_constructor()
				.add_method("set_damp", &JacobiPreconditioner<algebra_type>::set_damp, "", "damp");

		//	GaussSeidel
			reg.add_class_<	GSPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("GaussSeidel", grp2.c_str(), "Gauss-Seidel Preconditioner")
				.add_constructor();

		//	Symmetric GaussSeidel
			reg.add_class_<	SGSPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("SymmetricGaussSeidel", grp2.c_str())
				.add_constructor();

		//	Backward GaussSeidel
			reg.add_class_<	BGSPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("BackwardGaussSeidel", grp2.c_str())
				.add_constructor();

		//	ILU
			reg.add_class_<	ILUPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("ILU", grp2.c_str(), "Incomplete LU Decomposition")
				.add_constructor()
				.add_method("set_debug", &ILUPreconditioner<algebra_type>::set_debug, "", "d");


		//	ILU Threshold
			reg.add_class_<	ILUTPreconditioner<algebra_type>,
							IPreconditioner<algebra_type> >("ILUT", grp2.c_str(), "Incomplete LU Decomposition with threshold")
				.add_constructor()
				.add_method("set_threshold", &ILUTPreconditioner<algebra_type>::set_threshold,
							"", "threshold", "sets threshold of incomplete LU factorisation");

		}

		{
			//	LinearIteratorProduct
				reg.add_class_<	LinearIteratorProduct<vector_type, vector_type>,
						ILinearIterator<vector_type, vector_type> >("LinearIteratorProduct", grp.c_str(),
								"Linear Iterator consisting of several LinearIterations")
						.add_constructor()
						.add_method("add_iteration", &LinearIteratorProduct<vector_type, vector_type>::add_iterator, "Add an iterator");
		}
	/*
		{
	#ifdef LAPACK_AVAILABLE
			string subgroup = grp; // + string("/Preconditioner");

			reg.add_class_<	PINVIT<algebra_type> >("EigenSolver", subgroup.c_str())
				.add_constructor()
				.add_method("add_vector", &PINVIT<algebra_type>::add_vector,
							"", "vector")
				.add_method("set_preconditioner|interactive=false", &PINVIT<algebra_type>::set_preconditioner,
							"", "Preconditioner")
				.add_method("set_linear_operator_A|interactive=false", &PINVIT<algebra_type>::set_linear_operator_A,
							"", "LinearOperatorA")
				.add_method("set_linear_operator_B|interactive=false", &PINVIT<algebra_type>::set_linear_operator_B,
							"", "LinearOperatorB")
				.add_method("set_max_iterations|interactive=false", &PINVIT<algebra_type>::set_max_iterations,
								"", "precision")
				.add_method("set_precision|interactive=false", &PINVIT<algebra_type>::set_precision,
								"", "precision")
				.add_method("apply", &PINVIT<algebra_type>::apply);
	#endif
		}
	*/
		// todo: Solvers should be independent of type and placed in general section
		{
		//	get group string
			std::stringstream grpSS3; grpSS3 << grp << "/Solver";
			std::string grp3 = grpSS3.str();

		// 	LinearSolver
			reg.add_class_<	LinearSolver<algebra_type>,
							ILinearOperatorInverse<vector_type, vector_type> >("LinearSolver", grp3.c_str())
				.add_constructor()
				.add_method("set_preconditioner|interactive=false", &LinearSolver<algebra_type>::set_preconditioner,
							"", "Preconditioner")
				.add_method("set_convergence_check|interactive=false", &LinearSolver<algebra_type>::set_convergence_check,
							"", "Check");

		// 	CG Solver
			reg.add_class_<	CGSolver<algebra_type>,
							ILinearOperatorInverse<vector_type, vector_type> >("CG", grp3.c_str(), "Conjugate Gradient")
				.add_constructor()
				.add_method("set_preconditioner|interactive=false", &CGSolver<algebra_type>::set_preconditioner,
							"", "Preconditioner")
				.add_method("set_convergence_check|interactive=false", &CGSolver<algebra_type>::set_convergence_check,
							"", "Check");

		// 	BiCGStab Solver
			reg.add_class_<	BiCGStabSolver<algebra_type>,
							ILinearOperatorInverse<vector_type, vector_type> >("BiCGStab", grp3.c_str())
				.add_constructor()
				.add_method("set_preconditioner|interactive=false", &BiCGStabSolver<algebra_type>::set_preconditioner,
							"", "Preconditioner")
				.add_method("set_convergence_check|interactive=false", &BiCGStabSolver<algebra_type>::set_convergence_check,
							"", "Check");

		// 	LUSolver
			reg.add_class_<	LUSolver<algebra_type>,
							ILinearOperatorInverse<vector_type, vector_type> >("LU", grp3.c_str(), "LU-Decomposition exact solver")
				.add_constructor()
				.add_method("set_convergence_check|interactive=false", &LUSolver<algebra_type>::set_convergence_check,
							"", "Check");

		// 	DirichletDirichletSolver
	#ifdef UG_PARALLEL
			{
				typedef DirichletDirichletSolver<algebra_type> T;
				typedef ILinearOperatorInverse<vector_type, vector_type> BaseT;
				reg.add_class_<	T, BaseT >("DirichletDirichlet", grp3.c_str(), "Dirichlet-Dirichlet Domain Decomposition Algorithm")
				.add_constructor()
				.add_method("set_convergence_check|interactive=false", &T::set_convergence_check,
							"", "Check")
				.add_method("set_theta|interactive=false", &T::set_theta,
							"", "Theta", "set damping factor theta")
				.add_method("set_neumann_solver|interactive=false", &T::set_neumann_solver,
							"", "Neumann Solver")
				.add_method("set_dirichlet_solver|interactive=false", &T::set_dirichlet_solver,
							"", "Dirichlet Solver")
				.add_method("set_debug", &T::set_debug);
			}
	#endif
		// 	LocalSchurComplement
	#ifdef UG_PARALLEL
			{
				typedef LocalSchurComplement<algebra_type> T;
				reg.add_class_<	T, ILinearOperator<vector_type, vector_type> >("LocalSchurComplement", grp3.c_str())
				.add_constructor()
				.add_method("set_matrix|interactive=false", &T::set_matrix,
							"", "Matrix")
				.add_method("set_dirichlet_solver|interactive=false", &T::set_dirichlet_solver,
							"", "Dirichlet Solver")
				.add_method("set_debug", &T::set_debug, "", "d")
				// the following functions would normally not be executed from script
				.add_method("init", (bool (T::*)())&T::init)
				.add_method("apply", &T::apply,
							"Success", "local SC times Vector#Vector");
			}
	#endif
		// 	FETISolver
	#ifdef UG_PARALLEL
			{
				typedef FETISolver<algebra_type> T;
				typedef IMatrixOperatorInverse<vector_type, vector_type, matrix_type> BaseT;
				reg.add_class_<	T, BaseT >("FETI", grp3.c_str(), "FETI Domain Decomposition Solver")
				.add_constructor()
				.add_method("set_convergence_check|interactive=false", &T::set_convergence_check,
							"", "Check")
				.add_method("set_neumann_solver|interactive=false", &T::set_neumann_solver,
							"", "Neumann Solver")
				.add_method("set_dirichlet_solver|interactive=false", &T::set_dirichlet_solver,
							"", "Dirichlet Solver")
				.add_method("set_coarse_problem_solver|interactive=false", &T::set_coarse_problem_solver,
							"", "Coarse Problem Solver")
				.add_method("set_domain_decomp_info", &T::set_domain_decomp_info)
				.add_method("print_statistic_of_inner_solver", &T::print_statistic_of_inner_solver)
				.add_method("set_debug", &T::set_debug);
			}
	#endif

		// 	HLIBSolver
	#ifdef USE_HLIBPRO
			{
				typedef HLIBSolver<algebra_type> T;
				reg.add_class_<	T, ILinearOperatorInverse<vector_type, vector_type> >("HLIBSolver", grp3.c_str())
				.add_constructor()
				.add_method("set_convergence_check|interactive=false", &T::set_convergence_check,
							"", "Check")
				.add_method("set_hlib_nmin|interactive=false",         &T::set_hlib_nmin,
							"", "HLIB nmin")
				.add_method("set_hlib_accuracy_H|interactive=false",   &T::set_hlib_accuracy_H,
							"", "HLIB accuracy_H")
				.add_method("set_hlib_accuracy_LU|interactive=false",  &T::set_hlib_accuracy_LU,
							"", "HLIB accuracy_LU")
				.add_method("set_hlib_verbosity|interactive=false",    &T::set_hlib_verbosity,
							"", "HLIB verbosity")
				.add_method("set_clustering_method|interactive=false", &T::set_clustering_method,
							"", "Clustering")
				.add_method("set_ps_basename|interactive=false",       &T::set_ps_basename,
							"", "PostScript basename")
				.add_method("check_crs_matrix|interactive=false",      &T::check_crs_matrix,
							"", "Check CRS matrix")
	/*			*/
				.add_method("set_debug", &T::set_debug);
			}
	#endif

		}
		return true;
	}
};



bool RegisterStaticLibAlgebraInterface(Registry& reg, const char* parentGroup)
{
	try
	{
	//	get group string
		std::stringstream groupString; groupString << parentGroup << "/Algebra";
		std::string grp = groupString.str();

		// AlgebraSelector Interface
		reg.add_class_<	IAlgebraTypeSelector>("IAlgebraTypeSelector", grp.c_str());
		reg.add_class_<	CPUAlgebraSelector, IAlgebraTypeSelector>("CPUAlgebraSelector", grp.c_str())
			.add_constructor()
			.add_method("set_fixed_blocksize", &CPUAlgebraSelector::set_fixed_blocksize, "", "blocksize")
			.add_method("set_variable_blocksize", &CPUAlgebraSelector::set_variable_blocksize);

		// StandardConvCheck
		reg.add_class_<IConvergenceCheck>("IConvergenceCheck", grp.c_str());

		reg.add_class_<StandardConvCheck, IConvergenceCheck>("StandardConvergenceCheck", grp.c_str())
			.add_constructor()
			.add_method("set_maximum_steps|interactive=false", &StandardConvCheck::set_maximum_steps,
					"", "Maximum Steps")
			.add_method("set_minimum_defect|interactive=false", &StandardConvCheck::set_minimum_defect,
					"", "Minimum Defect")
			.add_method("set_reduction|interactive=false", &StandardConvCheck::set_reduction,
					"", "Reduction")
			.add_method("set_verbose_level|interactive=false", &StandardConvCheck::set_verbose_level,
					"", "Verbose");
				

		reg.add_function("TestInverse3", &TestInverse<3>);
	}

	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterLibAlgebraInterface: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}


bool RegisterAMG(Registry& reg, int algebra_type, const char* parentGroup);

bool RegisterDynamicLibAlgebraInterface(Registry& reg, int algebra_type, const char* parentGroup)
{
	// switch moved to lib_algebra_bridge.h
	RegisterAlgebraClass<cRegisterAlgebraType>(reg, algebra_type, parentGroup);

	RegisterAMG(reg, algebra_type, parentGroup);

	return true;
}



} // end namespace bridge
} // end namespace ug
