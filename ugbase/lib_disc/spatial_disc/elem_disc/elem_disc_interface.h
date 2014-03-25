/*
 * elem_disc_interface.h
 *
 *  Created on: 07.07.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__
#define __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__

// extern headers
#include <vector>
#include <string>

// intern headers
#include "lib_disc/common/local_algebra.h"
#include "lib_disc/time_disc/solution_time_series.h"
#include "lib_disc/function_spaces/approximation_space.h"
#include "lib_disc/local_finite_element/local_finite_element_id.h"
#include "lib_disc/reference_element/reference_element_traits.h"
#include "lib_disc/spatial_disc/user_data/data_import.h"
#include "common/util/provider.h"
#include "lib_disc/domain_util.h"
#include "lib_disc/domain_traits.h"
#include "lib_disc/spatial_disc/elem_disc/err_est_data.h"

namespace ug{

/// Types of elem disc
enum ElemDiscType
{
	EDT_NONE = 0,
	EDT_ELEM = 1 << 0,
	EDT_SIDE = 1 << 1,
	EDT_BND = 1 << 2,
	EDT_ALL = EDT_NONE | EDT_SIDE | EDT_ELEM | EDT_BND
};

/**
 * Element Discretizations
 *
 * \defgroup lib_disc_elem_disc Elem Disc
 * \ingroup lib_discretization
 */

/// \ingroup lib_disc_elem_disc
/// @{

///	base class for all element-wise discretizations
/**
 * This class is the base class for element-wise discretizations. An
 * implementation of this class must provide local stiffness/mass-matrix
 * contribution of one element to the global jacobian and local contributions
 * of one element to the local defect.
 */
template <typename TDomain>
class IElemDisc
{
	public:
	///	Domain type
		typedef TDomain domain_type;

	///	World dimension
		static const int dim = TDomain::dim;

	///	Position type
		typedef typename TDomain::position_type position_type;
		
	public:
	///	Constructor
		IElemDisc(const char* functions = "", const char* subsets = "");

	///	Constructor
		IElemDisc(const std::vector<std::string>& vFct, const std::vector<std::string>& vSubset);

	/// Virtual destructor
		virtual ~IElemDisc() {}

	public:
	///	sets the approximation space
	/**	Calls protected virtual 'approximation_space_changed', when a new approximation space
	 * has been set. Note that 'approximation_space_changed' is only called once if the
	 * same approximation space is set multiple times.*/
		void set_approximation_space(SmartPtr<ApproximationSpace<TDomain> > approxSpace);

	///	returns approximation space
		SmartPtr<ApproximationSpace<TDomain> > approx_space() {return m_spApproxSpace;}

	///	returns approximation space
		ConstSmartPtr<ApproximationSpace<TDomain> > approx_space() const {return m_spApproxSpace;}

	///	returns the domain
		SmartPtr<TDomain> domain(){return m_spApproxSpace->domain();}

	///	returns the domain
		ConstSmartPtr<TDomain> domain() const{return m_spApproxSpace->domain();}

	///	returns the subset handler
		typename TDomain::subset_handler_type& subset_handler()
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain()->subset_handler();
		}

	///	returns the subset handler
		const typename TDomain::subset_handler_type& subset_handler() const
		{
			UG_ASSERT(m_spApproxSpace.valid(), "ApproxSpace not set.");
			return *m_spApproxSpace->domain()->subset_handler();
		}

	protected:
	///	callback invoked, when approximation space is changed
		virtual void approximation_space_changed() {}

	///	Approximation Space
		SmartPtr<ApproximationSpace<TDomain> > m_spApproxSpace;

	////////////////////////////
	// Functions and Subsets
	////////////////////////////
	public:
	///	sets functions by name list, divided by ','
		void set_functions(const std::string& functions);

	/// sets functions by vector of names
		void set_functions(const std::vector<std::string>& functions);

	///	sets subset(s) by name list, divided by ','
		void set_subsets(const std::string& subsets);

	///	sets subset(s) by name list, divided by ','
		void set_subsets(const std::vector<std::string>& subsets);

	/// number of functions this discretization handles
		size_t num_fct() const {return m_vFct.size();}

	///	returns the symbolic functions
		const std::vector<std::string>& symb_fcts() const {return m_vFct;}

	/// number of subsets this discretization handles
		size_t num_subsets() const {return m_vSubset.size();}

	///	returns the symbolic subsets
		const std::vector<std::string>& symb_subsets() const {return m_vSubset;}

	///	returns the current function pattern
		ConstSmartPtr<FunctionPattern> function_pattern() const {return m_spFctPattern;}

	///	returns the current function group
		const FunctionGroup& function_group() const {return m_fctGrp;}

	///	returns the current function index mapping
		const FunctionIndexMapping& map() const {return m_fctIndexMap;}

	///	checks the setup of the elem disc
		void check_setup(bool bNonRegularGrid);

	protected:
	///	vector holding name of all symbolic functions
		std::vector<std::string> m_vFct;

	///	vector holding name of all symbolic subsets
		std::vector<std::string> m_vSubset;

	///	current function pattern
		ConstSmartPtr<FunctionPattern> m_spFctPattern;

	///	current function group
		FunctionGroup m_fctGrp;

	///	current function index mapping
		FunctionIndexMapping m_fctIndexMap;

	///	sets current function pattern
		void set_function_pattern(ConstSmartPtr<FunctionPattern> fctPatt);

	///	updates the function index mapping
		void update_function_index_mapping();

	////////////////////////////
	// UserData and Coupling
	////////////////////////////
	public:
	///	registers a data import
		void register_import(IDataImport<dim>& Imp);

	///	returns number of imports
		size_t num_imports() const {return m_vIImport.size();}

	/// returns an import
		IDataImport<dim>& get_import(size_t i)
		{
			UG_ASSERT(i < num_imports(), "Invalid index");
			return *m_vIImport[i];
		}

	///	removes all imports
		void clear_imports() {m_vIImport.clear();}

	protected:
	/// data imports
		std::vector<IDataImport<dim>*> m_vIImport;

	////////////////////////////
	// time handling
	////////////////////////////
	public:
	///	sets if assembling should be time-dependent and the local time series
	/**
	 * This function specifies if the assembling is time-dependent. If NULL is
	 * passed, the assembling is assumed to be time-independent. If a local
	 * time series is passed, this series is used as previous solution.
	 *
	 * \param[in]	locTimeSeries	Time series of previous solutions
	 */
		void set_time_dependent(LocalVectorTimeSeries& locTimeSeries,
		        				const std::vector<number>& vScaleMass,
		        				const std::vector<number>& vScaleStiff);

	///	sets that the assembling is time independent
		void set_time_independent();

	///	returns if assembling is time-dependent
		bool is_time_dependent() const {return (m_pLocalVectorTimeSeries != NULL) && !m_bStationaryForced;}

	///	sets that the assembling is always stationary (even in instationary case)
		void set_stationary(bool bStationaryForced = true) {m_bStationaryForced = bStationaryForced;}
		void set_stationary() {set_stationary(true);}

	///	returns if local time series needed by assembling
	/**
	 * This callback must be implemented by a derived Elem Disc in order to handle
	 * time-dependent data. As return the derived Elem Disc can specify, if
	 * it really needs data from previous time steps for the (spatial) disc. The
	 * default is false.
	 *
	 * \returns 	if elem disc needs time series local solutions
	 */
		virtual bool requests_local_time_series() {return false;}

		bool local_time_series_needed() {return is_time_dependent() && requests_local_time_series();}

	///	sets the current time point
		void set_time_point(const size_t timePoint) {m_timePoint = timePoint;}

	///	returns the currently considered time point of the time-disc scheme
		size_t time_point() const {return m_timePoint;}

	///	returns currently set timepoint
		number time() const {if(m_pLocalVectorTimeSeries)
								return m_pLocalVectorTimeSeries->time(m_timePoint);
							else return 0.0;}

	///	returns the local time solutions
	/**
	 * This function returns the local time solutions. This a type of vector,
	 * that holds the local unknowns for each time point of a time series.
	 * Note, that the first sol is always the current (iterated, unknown)
	 * time point, while all remaining sols are the already computed time steps
	 * used e.g. in a time stepping scheme.
	 *
	 * \returns vLocalTimeSol		vector of local time Solutions
	 */
		const LocalVectorTimeSeries* local_time_solutions() const
			{return m_pLocalVectorTimeSeries;}

	///	returns the weight factors of the time-disc scheme
	///	\{
		const std::vector<number>& mass_scales() const {return m_vScaleMass;}
		const std::vector<number>& stiff_scales() const {return m_vScaleStiff;}

		number mass_scale(const size_t timePoint) const {return m_vScaleMass[timePoint];}
		number stiff_scale(const size_t timePoint) const {return m_vScaleStiff[timePoint];}

		number mass_scale() const {return m_vScaleMass[m_timePoint];}
		number stiff_scale() const {return m_vScaleStiff[m_timePoint];}
	///	\}

	protected:
	///	time point
		size_t m_timePoint;

	///	list of local vectors for all solutions of the time series
		LocalVectorTimeSeries* m_pLocalVectorTimeSeries;

	///	weight factors for time dependent assembling
	/// \{
		std::vector<number> m_vScaleMass;
		std::vector<number> m_vScaleStiff;
	/// \}

	///	flag if stationary assembling is to be used even in instationary assembling
		bool m_bStationaryForced;

	////////////////////////////
	// Error estimator
	////////////////////////////
	public:
	///	sets the pointer to an error estimator data object (or NULL)
	/**
	 * This function sets the pointer to an error estimator data object
	 * that should be used for this discretization. Note that the ElemDisc
	 * object must ust RTTI to try to convert this pointer to the type
	 * of the objects accepted by it for this purpose. If the conversion
	 * fails than an exception must be thrown since this situation is not
	 * allowed. But this function can be always used with the SPNULL argument:
	 * This means that no error estimators are assembled for this discretization.
	 */
	///	returns the pointer to the error estimator data object (or NULL)
		virtual SmartPtr<IErrEstData> err_est_data () {return SPNULL;}
	
	////////////////////////////
	// general info
	////////////////////////////
	public:
	///	 returns the type of elem disc
		virtual int type() const {return EDT_ELEM | EDT_SIDE;}

	/// requests assembling for trial spaces and grid type
	/**
	 * This function is called before the assembling starts. The
	 * IElemDisc-Implementation is supposed to checks if it can assemble the set
	 * of LFEID and the grid type. It may register corresponding assembling
	 * functions or perform other initialization.
	 * If the ElemDisc does not support the setting it should throw an exception.
	 *
	 * \param[in] vLfeID			vector of Local Finite Element IDs
	 * \param[in] bNonRegularGrid	regular grid type
	 */
		virtual void prepare_setting(const std::vector<LFEID>& vLfeID, bool bNonRegularGrid) = 0;

	///	returns if discretization acts on hanging nodes if present
	/**
	 * This function returns if a discretization really needs the hanging nodes
	 * in case of non-regular grid. This may not be the case for e.g. finite
	 * element assemblings but is needed for finite volumes
	 */
		virtual bool use_hanging() const {return false;}

	////////////////////////////
	// assembling functions
	////////////////////////////
	public:
	/// prepare the timestep
		virtual void prep_timestep_elem(const number time, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	///	virtual prepares the loop over all elements of one type
		virtual void prep_elem_loop(const ReferenceObjectID roid, const int si);

	///	virtual prepare one elements for assembling
		virtual void prep_elem(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	///	virtual postprocesses the loop over all elements of one type
		virtual void fsh_elem_loop();

	/// virtual finish the timestep
		virtual void fsh_timestep_elem(const number time, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	/// Assembling of Jacobian (Stiffness part)
		virtual void add_jac_A_elem(LocalMatrix& J, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	/// Assembling of Jacobian (Mass part)
		virtual void add_jac_M_elem(LocalMatrix& J, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	/// virtual Assembling of Defect (Stiffness part)
		virtual void add_def_A_elem(LocalVector& d, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

    /// defect for explicit terms
		virtual void add_def_A_expl_elem(LocalVector& d, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	/// virtual Assembling of Defect (Mass part)
		virtual void add_def_M_elem(LocalVector& d, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	/// virtual Assembling of Right-Hand Side
		virtual void add_rhs_elem(LocalVector& rhs, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		
	///	virtual prepares the loop over all elements of one type for the computation of the error estimator
		virtual void prep_err_est_elem_loop(const ReferenceObjectID roid, const int si);

	///	virtual compute the error estimator contribution for one element
		virtual void compute_err_est_elem(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		
	///	virtual summarize the contributions of the error estimator in one element
		virtual number get_err_est_elem(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	///	virtual postprocesses the loop over all elements of one type in the computation of the error estimator
		virtual void fsh_err_est_elem_loop();
		
	///	function dispatching call to implementation
	/// \{
		void do_prep_timestep_elem(const number time, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_prep_elem_loop(const ReferenceObjectID roid, const int si);
		void do_prep_elem(LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_fsh_elem_loop();
		void do_fsh_timestep_elem(const number time, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_add_jac_A_elem(LocalMatrix& J, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_add_jac_M_elem(LocalMatrix& J, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_add_def_A_elem(LocalVector& d, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
   	    void do_add_def_A_expl_elem(LocalVector& d, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_add_def_M_elem(LocalVector& d, LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_add_rhs_elem(LocalVector& rhs, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_prep_err_est_elem_loop(const ReferenceObjectID roid, const int si);
		void do_compute_err_est_elem(LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		number do_get_err_est_elem(LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		void do_fsh_err_est_elem_loop();
	/// \}

	private:
	//	abbreviation for own type
		typedef IElemDisc<TDomain> T;

	// 	types of timestep function pointers
		typedef void (T::*PrepareTimestepElemFct)(number, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef void (T::*FinishTimestepElemFct)(number, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	// 	types of loop function pointers
		typedef void (T::*PrepareElemLoopFct)(ReferenceObjectID roid, int si);
		typedef void (T::*PrepareElemFct)(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef void (T::*FinishElemLoopFct)();

	// 	types of Jacobian assemble functions
		typedef void (T::*ElemJAFct)(LocalMatrix& J, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef void (T::*ElemJMFct)(LocalMatrix& J, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	// 	types of Defect assemble functions
		typedef void (T::*ElemdAFct)(LocalVector& d, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef void (T::*ElemdMFct)(LocalVector& d, const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);

	// 	types of right hand side assemble functions
		typedef void (T::*ElemRHSFct)(LocalVector& rhs, GridObject* elem, const MathVector<dim> vCornerCoords[]);
	
	//	types of the error estimator assembler
		typedef void (T::*PrepareErrEstElemLoopFct)(ReferenceObjectID roid, int si);
		typedef void (T::*ElemComputeErrEstFct)(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef number (T::*ElemGetErrEstFct)(const LocalVector& u, GridObject* elem, const MathVector<dim> vCornerCoords[]);
		typedef void (T::*FinishErrEstElemLoopFct)();

	protected:
	// 	register the functions
		template <typename TAssFunc> void set_prep_timestep_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_fsh_timestep_elem_fct(ReferenceObjectID id, TAssFunc func);

		template <typename TAssFunc> void set_prep_elem_loop_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_prep_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_fsh_elem_loop_fct(ReferenceObjectID id, TAssFunc func);

		template <typename TAssFunc> void set_add_jac_A_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_add_jac_M_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_add_def_A_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_add_def_A_expl_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_add_def_M_elem_fct(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_add_rhs_elem_fct(ReferenceObjectID id, TAssFunc func);
		
		template <typename TAssFunc> void set_prep_err_est_elem_loop(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_compute_err_est_elem(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_get_err_est_elem(ReferenceObjectID id, TAssFunc func);
		template <typename TAssFunc> void set_fsh_err_est_elem_loop(ReferenceObjectID id, TAssFunc func);
		
	//	unregister functions
		void remove_prep_timestep_elem_fct(ReferenceObjectID id);
		void remove_fsh_timestep_elem_fct(ReferenceObjectID id);

		void remove_prep_elem_loop_fct(ReferenceObjectID id);
		void remove_prep_elem_fct(ReferenceObjectID id);
		void remove_fsh_elem_loop_fct(ReferenceObjectID id);

		void remove_add_jac_A_elem_fct(ReferenceObjectID id);
		void remove_add_jac_M_elem_fct(ReferenceObjectID id);
		void remove_add_def_A_elem_fct(ReferenceObjectID id);
		void remove_add_def_A_expl_elem_fct(ReferenceObjectID id);
		void remove_add_def_M_elem_fct(ReferenceObjectID id);
		void remove_add_rhs_elem_fct(ReferenceObjectID id);
		
		void remove_prep_err_est_elem_loop(ReferenceObjectID id);
		void remove_compute_err_est_elem(ReferenceObjectID id);
		void remove_get_err_est_elem(ReferenceObjectID id);
		void remove_fsh_err_est_elem_loop(ReferenceObjectID id);

	///	sets all assemble functions to NULL for a given ReferenceObjectID
		void clear_add_fct(ReferenceObjectID id);

	///	sets all assemble functions to NULL (for all ReferenceObjectID's)
		void clear_add_fct();

	///	sets all assemble functions to the corresponding virtual ones
		void set_default_add_fct();

	public:
	/// sets the geometric object type
	/**
	 * This functions set the geometric object type of the object, that is
	 * assembled next. The user has to call this function before most of the
	 * assembling routines can be called. Keep in mind, that the elements are
	 * looped type by type, thus this function has to be called very few times.
	 */
		void set_roid(ReferenceObjectID id, int discType);

	private:
	// 	timestep function pointers
		PrepareTimestepElemFct 		m_vPrepareTimestepElemFct[NUM_REFERENCE_OBJECTS];
		FinishTimestepElemFct 		m_vFinishTimestepElemFct[NUM_REFERENCE_OBJECTS];

	// 	loop function pointers
		PrepareElemLoopFct 	m_vPrepareElemLoopFct[NUM_REFERENCE_OBJECTS];
		PrepareElemFct 		m_vPrepareElemFct[NUM_REFERENCE_OBJECTS];
		FinishElemLoopFct 	m_vFinishElemLoopFct[NUM_REFERENCE_OBJECTS];

	// 	Jacobian function pointers
		ElemJAFct 	m_vElemJAFct[NUM_REFERENCE_OBJECTS];
		ElemJMFct 	m_vElemJMFct[NUM_REFERENCE_OBJECTS];

	// 	Defect function pointers
		ElemdAFct 	m_vElemdAFct[NUM_REFERENCE_OBJECTS];
		ElemdAFct 	m_vElemdAExplFct[NUM_REFERENCE_OBJECTS];
		ElemdMFct 	m_vElemdMFct[NUM_REFERENCE_OBJECTS];

	// 	Rhs function pointers
		ElemRHSFct 	m_vElemRHSFct[NUM_REFERENCE_OBJECTS];
		
	//	Error estimator functions
		PrepareErrEstElemLoopFct	m_vPrepareErrEstElemLoopFct[NUM_REFERENCE_OBJECTS];
		ElemComputeErrEstFct		m_vElemComputeErrEstFct[NUM_REFERENCE_OBJECTS];
		ElemGetErrEstFct			m_vElemGetErrEstFct[NUM_REFERENCE_OBJECTS];
		FinishErrEstElemLoopFct		m_vFinishErrEstElemLoopFct[NUM_REFERENCE_OBJECTS];

	protected:
	/// current Geometric Object
		ReferenceObjectID m_id;
};
/// @}

} // end namespace ug

#include "elem_disc_interface_impl.h"

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__ELEM_DISC__ELEM_DISC_INTERFACE__ */
