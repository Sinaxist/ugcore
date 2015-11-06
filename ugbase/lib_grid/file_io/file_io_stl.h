/*
 * Copyright (c) 2010-2015:  G-CSC, Goethe University Frankfurt
 * Author: Sebastian Reiter
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

#ifndef __LIBGRID__FILE_IO_STL__
#define __LIBGRID__FILE_IO_STL__

#include "lib_grid/grid/grid.h"
#include "lib_grid/tools/subset_handler_interface.h"
#include "lib_grid/common_attachments.h"

namespace ug
{

///	loads stl-ascii-files.
/**
 * if aPosition is not attached to the vertices of the grid,
 * it will be automatically attached.
 * If however aNormal is not attached to the faces of the grid,
 * it will be ignored.
 */
bool LoadGridFromSTL(Grid& grid, const char* filename,
					ISubsetHandler* pSH = NULL,
					AVector3& aPos = aPosition,
					AVector3& aNormFACE = aNormal);

bool STLFileHasASCIIFormat(const char* filename);

bool LoadGridFromSTL_ASCII(Grid& grid, const char* filename,
						   ISubsetHandler* pSH = NULL,
						   AVector3& aPos = aPosition,
						   AVector3& aNormFACE = aNormal);

bool LoadGridFromSTL_BINARY(Grid& grid, const char* filename,
							ISubsetHandler* pSH = NULL,
							AVector3& aPos = aPosition,
							AVector3& aNormFACE = aNormal);

bool SaveGridToSTL(Grid& grid, const char* filename,
					ISubsetHandler* pSH = NULL,
					AVector3& aPos = aPosition);

}

#endif
