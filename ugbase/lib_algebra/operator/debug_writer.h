/*
 * Copyright (c) 2011-2015:  G-CSC, Goethe University Frankfurt
 * Author: Andreas Vogel
 * 
 * This file is part of UG4.
 * 
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#ifndef __H__LIB_ALGEBRA__OPERATOR__DEBUG_WRITER__
#define __H__LIB_ALGEBRA__OPERATOR__DEBUG_WRITER__

#include "common/util/smart_pointer.h"
#include "common/math/ugmath.h"
#include "common/profiler/profiler.h"


namespace ug{

/// Interface for providing vertex positions
template <size_t dim>
class IPositionProvider
{
public:
	/// Assigns position information to given vector
	/** \param vec vector to be modified
	 * todo: change to assign_positions () const?*/
	virtual bool get_positions(std::vector<MathVector<dim> >&vec) = 0;
	virtual ~IPositionProvider() {}
};


/// base class for all vector debug writer
/**
 * This is the base class for debug output of algebraic vectors
 */
template <typename TVector>
class IVectorDebugWriter
{
	public:
	///	type of vector
		typedef TVector vector_type;

	public:
	///	Constructor
		IVectorDebugWriter() : m_baseDir("."), m_currentDim(-1) {}

	///	write vector
		virtual void write_vector(const vector_type& vec, const char* name) = 0;

	///	returns the current dimension
		int current_dimension() const {return m_currentDim;}

	///	returns the positions (only available for current dimension)
		template <int dim>
		const std::vector<MathVector<dim> >& get_positions() const
		{
			if(m_currentDim != dim) throw(UGError("Current dim is different."));
			return get_pos(Int2Type<dim>());
		}

	/// gives subclasses the oportunity to calculate the positions
	/// this is used e.g. in GridFunctionDebugWriter
	/// after this has been called, get_positions gives valid positions
	/// those positions are used e.g. in AMG or Schur methods.
		virtual void update_positions() {};

	///	sets the current positions
		template <int dim>
		void set_positions(const std::vector<MathVector<dim> >& vPos)
		{
			get_pos(Int2Type<dim>()) = vPos; m_currentDim = dim;
		}

	///	employs a position provider to set the current positions
		template <int dim>
		void set_positions(IPositionProvider<dim> & provider)
		{
			provider.get_positions(get_pos(Int2Type<dim>()));
			m_currentDim = dim;
		}

	/// virtual destructor
		virtual ~IVectorDebugWriter(){}

		int get_dim() const
		{
			return m_currentDim;
		}

		/// set the base directory for output files (.vec and .mat)
		inline void set_base_dir(const char* const baseDir) {m_baseDir = std::string(baseDir);}
		std::string get_base_dir() { return m_baseDir; }

	protected:
		///  base directory for output
		std::string m_baseDir;

	///	returns the positions and sets the current dim
		template <int dim>
		std::vector<MathVector<dim> >& positions()
		{
			m_currentDim = dim; return get_pos(Int2Type<dim>());
		}

	///	current dimension
		int m_currentDim;

	///	vectors of positions
		std::vector<MathVector<1> > m_vPos1d;
		std::vector<MathVector<2> > m_vPos2d;
		std::vector<MathVector<3> > m_vPos3d;

	///	help function to get local ips
		std::vector<MathVector<1> >& get_pos(Int2Type<1>) {return m_vPos1d;}
		std::vector<MathVector<2> >& get_pos(Int2Type<2>) {return m_vPos2d;}
		std::vector<MathVector<3> >& get_pos(Int2Type<3>) {return m_vPos3d;}
		const std::vector<MathVector<1> >& get_pos(Int2Type<1>) const {return m_vPos1d;}
		const std::vector<MathVector<2> >& get_pos(Int2Type<2>) const {return m_vPos2d;}
		const std::vector<MathVector<3> >& get_pos(Int2Type<3>) const {return m_vPos3d;}
};

/// base class for all debug writer
/**
 * This is the base class for debug output of algebraic vectors and matrices.
 */
template <typename TAlgebra>
class IDebugWriter : public IVectorDebugWriter<typename TAlgebra::vector_type>
{
	public:
	///	type of algebra
		typedef TAlgebra algebra_type;

	///	type of vector
		typedef typename TAlgebra::vector_type vector_type;

	///	type of matrix
		typedef typename TAlgebra::matrix_type matrix_type;

	public:
	///	write vector
		virtual void write_vector(const vector_type& vec, const char* name) = 0;

	///	write matrix
		virtual void write_matrix(const matrix_type& mat, const char* name) = 0;
};


template <typename TVector>
class VectorDebugWritingObject
{
	public:
	///	type of vector
		typedef TVector vector_type;

	public:
		VectorDebugWritingObject() : m_spVectorDebugWriter(NULL) {}
		VectorDebugWritingObject(SmartPtr<IVectorDebugWriter<vector_type> > spDebugWriter)
			: m_spVectorDebugWriter(spDebugWriter) {}

	///	virtual destructor
		virtual ~VectorDebugWritingObject() {}

	///	set debug writer
		virtual void set_debug(SmartPtr<IVectorDebugWriter<vector_type> > spDebugWriter)
		{
			m_spVectorDebugWriter = spDebugWriter;
		}

	///	returns the debug writer
		SmartPtr<IVectorDebugWriter<vector_type> > vector_debug_writer() {return m_spVectorDebugWriter;}
		ConstSmartPtr<IVectorDebugWriter<vector_type> > vector_debug_writer() const {return m_spVectorDebugWriter;}

	protected:
		virtual void write_debug(const vector_type& vec, std::string filename)
		{
			write_debug(vec, filename.c_str());
		}
	///	writing debug output for a vector (if debug writer set)
		virtual void write_debug(const vector_type& vec, const char* filename)
		{
			PROFILE_FUNC_GROUP("algebra debug");
		//	if no debug writer set, we're done
			if(m_spVectorDebugWriter.invalid()) return;

		//	check ending
			std::string name(filename);
			size_t iExtPos = name.find_last_of(".");
			if(iExtPos != std::string::npos && name.substr(iExtPos).compare(".vec") != 0)
				UG_THROW("Only '.vec' format supported for vectors, but"
								" filename is '"<<name<<"'.");

			if(iExtPos == std::string::npos)
				name.append(".vec");

		//	write
			m_spVectorDebugWriter->write_vector(vec, name.c_str());
		}

	protected:
	///	Debug Writer
		SmartPtr<IVectorDebugWriter<vector_type> > m_spVectorDebugWriter;
};

template <typename TAlgebra>
class DebugWritingObject : public VectorDebugWritingObject<typename TAlgebra::vector_type>
{
	public:
	///	type of algebra
		typedef TAlgebra algebra_type;

	///	type of vector
		typedef typename TAlgebra::vector_type vector_type;

	///	type of matrix
		typedef typename TAlgebra::matrix_type matrix_type;

	protected:
		using VectorDebugWritingObject<vector_type>::write_debug;
		using VectorDebugWritingObject<vector_type>::set_debug;

	public:
		DebugWritingObject() : m_spDebugWriter(NULL) {}
		DebugWritingObject(SmartPtr<IDebugWriter<algebra_type> > spDebugWriter)
			: 	VectorDebugWritingObject<vector_type>(spDebugWriter),
				m_spDebugWriter(spDebugWriter) {}

	/// clone constructor
		DebugWritingObject(const DebugWritingObject<algebra_type> &parent)
			: 	VectorDebugWritingObject<vector_type>(parent.m_spDebugWriter),
				m_spDebugWriter(parent.m_spDebugWriter) {}

	///	virtual destructor
		virtual ~DebugWritingObject() {}

	///	set debug writer
		virtual void set_debug(SmartPtr<IDebugWriter<algebra_type> > spDebugWriter)
		{
			m_spDebugWriter = spDebugWriter;
			VectorDebugWritingObject<vector_type>::set_debug(m_spDebugWriter);
		}

	///	returns the debug writer
		SmartPtr<IDebugWriter<algebra_type> > debug_writer() {return m_spDebugWriter;}
		ConstSmartPtr<IDebugWriter<algebra_type> > debug_writer() const {return m_spDebugWriter;}

	protected:
	///	write debug output for a matrix (if debug writer set)
		virtual void write_debug(const matrix_type& mat, const char* filename)
		{
			PROFILE_FUNC_GROUP("algebra debug");
		//	if no debug writer set, we're done
			if(m_spDebugWriter.invalid()) return;

		//	check ending
			std::string name(filename);
			size_t iExtPos = name.find_last_of(".");
			if(iExtPos != std::string::npos && name.substr(iExtPos).compare(".mat") != 0)
				UG_THROW("Only '.mat' format supported for matrices, but"
								" filename is '"<<name<<"'.");

			if(iExtPos == std::string::npos)
				name.append(".mat");

		//	write
			m_spDebugWriter->write_matrix(mat, name.c_str());
		}

	protected:
	///	Debug Writer
		SmartPtr<IDebugWriter<algebra_type> > m_spDebugWriter;
};

} // end namespace ug

#endif /* __H__LIB_ALGEBRA__OPERATOR__DEBUG_WRITER__ */
