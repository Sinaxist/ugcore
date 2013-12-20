# included from build_algebra.cmake
########################################

# if the search for LAPACK/BLAS is not working, you can use
# a) USER_LAPACK_LIBRARIES, (optionally  USER_LAPACK_INCLUDE_DIR ) and USER_BLAS_LIBRARIES (optionally USER_BLAS_INCLUDE_DIR )
# b) BUILTIN_LAPACK and BUILTIN_BLAS (this is used e.g. on the Hermit cluster)
#
# Some common paths for LAPACK/BLAS are:
# /usr/local/lib /usr/lib /usr/local/lib64 /usr/lib64 /bgsys/local/lib
# Please mail martin.rupp@gcsc.uni-frankfurt.de if you have problems setting up
# OR if you solved your problem.

# we'll check for lapack and blas here
if(LAPACK OR BLAS)
	# On OSX, we know where lapack and blas are located. To avoid errors
	# with Fortran compilers on OSX, we'll add APPLE as a special case here.
	if(APPLE)
		if(LAPACK)
			set(LAPACK_LIBRARIES "-framework vecLib" CACHE STRING "LAPACK library" FORCE)
			FIND_PATH(LAPACK_INCLUDE_DIR clapack.h /usr/local/include/ /usr/include /include)
			if(LAPACK_INCLUDE_DIR)
	  			set(LAPACK_FOUND YES)
	  		endif(LAPACK_INCLUDE_DIR)
		endif(LAPACK)
		
		if(BLAS)
			set(BLAS_LIBRARIES "-framework vecLib" CACHE STRING "CBLAS library" FORCE)
			find_path(BLAS_INCLUDE_DIR cblas.h /usr/local/include/ /usr/include /include)
			set(BLAS_FOUND YES)
			if(BLAS_INCLUDE_DIR)
				set(BLAS_FOUND YES)
			endif(BLAS_INCLUDE_DIR)
		endif(BLAS)
		
	elseif(NOT BUILTIN_BLAS AND NOT BUILTIN_LAPACK)			
		if(LAPACK)
			find_package(LAPACK)
		endif(LAPACK)
		
		if(BLAS)
	    	find_package(BLAS)
		endif(BLAS)
	endif(APPLE)

# We'll output whether lapack and blas are used, to avoid misconceptions
	
	if(USER_LAPACK_LIBRARIES)
	    message(STATUS "Info: Using User LAPACK (Include: ${USER_LAPACK_INCLUDE_DIR}, Lib: ${USER_LAPACK_LIBRARIES})")
	    if(LAPACK_FOUND)
	        message(STATUS "Note: (CMake also found LAPACK in Includes: ${LAPACK_INCLUDE_DIR}, Lib: ${LAPACK_LIBRARIES})")
	    endif(LAPACK_FOUND)
		include_directories (${USER_LAPACK_INCLUDE_DIR})
		set(linkLibraries ${linkLibraries} ${USER_LAPACK_LIBRARIES})
		add_definitions(-DLAPACK_AVAILABLE)	
	elseif(LAPACK_FOUND)
		message(STATUS "Info: Using LAPACK (Include: ${LAPACK_INCLUDE_DIR}, Lib: ${LAPACK_LIBRARIES})")
		include_directories (${LAPACK_INCLUDE_DIR})
		set(linkLibraries ${linkLibraries} ${LAPACK_LIBRARIES})
		add_definitions(-DLAPACK_AVAILABLE)	
	elseif(BUILTIN_LAPACK)
		message(STATUS "Info: Using Builtin LAPACK")
		add_definitions(-DLAPACK_AVAILABLE)
	else()	
		message(STATUS "WARNING: Not using LAPACK. No package found.")
		message(STATUS "If you have a builtin LAPACK, use -DBUILTIN_LAPACK")
		message(STATUS "To supply your own LAPACK libraries, use -DUSER_LAPACK_LIBRARIES and (optionally) -DUSER_LAPACK_INCLUDES:\n\tcmake -DUSER_LAPACK_LIBRARIES=/usr/lib64/liblapack.so -DUSER_LAPACK_INCLUDES=/includepath/ ..")
	endif()
	
	if(USER_BLAS_LIBRARIES)
		message(STATUS "Info: Using User BLAS (Include: ${USER_BLAS_INCLUDE_DIR}, Lib: ${USER_BLAS_LIBRARIES})")
		 if(BLAS_FOUND)
	        message(STATUS "Note: (CMake also found BLAS in Includes: ${BLAS_INCLUDE_DIR}, Lib: ${BLAS_LIBRARIES})")
	    endif(BLAS_FOUND)
		include_directories (${USER_BLAS_INCLUDE_DIR})
		set(linkLibraries ${linkLibraries} ${USER_BLAS_LIBRARIES})
		add_definitions(-DBLAS_AVAILABLE)
	elseif(BLAS_FOUND)
		message(STATUS "Info: Using BLAS (Include: ${BLAS_INCLUDE_DIR}, Lib: ${BLAS_LIBRARIES})")
		include_directories (${BLAS_INCLUDE_DIR})
		set(linkLibraries ${linkLibraries} ${BLAS_LIBRARIES})
		add_definitions(-DBLAS_AVAILABLE)
	elseif(BUILTIN_BLAS)
		message(STATUS "Info: Using Builtin BLAS")
		add_definitions(-DBLAS_AVAILABLE)
	else()	
		message(STATUS "WARNING: Not using BLAS. No package found.")
		message(STATUS "If you have a builtin BLAS, use -DBUILTIN_BLAS")
		message(STATUS "To supply your own BLAS libraries, use -DUSER_BLAS_LIBRARIES and (optionally) -DUSER_BLAS_INCLUDES:\n\tcmake -DUSER_BLAS_LIBRARIES=/usr/lib64/liblablas.so -DUSER_BLAS_INCLUDES=/includepath/ ..")
	endif()
	
	if(BLAS_goto2_LIBRARY)
		message(STATUS "Info: GotoBLAS2 found: (${BLAS_goto2_LIBRARY}). Adding -lgfortran.")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgfortran")
	endif()
endif(LAPACK OR BLAS)