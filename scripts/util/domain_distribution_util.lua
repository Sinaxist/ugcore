--[[!
\file scripts/util/domain_distribution_util.lua
\defgroup scripts_util_domaindistribution Domain Distribution Utility
\ingroup scripts_util
\brief creates partition maps of different structure and provides some easy to use domain distribution methods.
\author Sebastian Reiter
\{
]]--

util = util or {}


--!	Distributes the top-level of a domain to the given number of processes.
--! This method has to be called by all processes with the same parameters!
--! @note Some paramters are optional. nil is a valid value for each optional parameter.
--! @return (bool) Returns whether the method was a success
--! @param dom	(Domain) A valid domain instance.
--! @param partitioningMethod	(optional string) Choose the partitioning method.
--!								Valid values are: "bisection", "metis"
--!								default is "bisection". "metis" is only available,
--!								if ug has been built with metis support.
--! @param verticalInterfaces	(optional bool) Trigger creation of vertical interfaces.
--!								vertical interfaces are required for multi-grid solvers.
--!								If you're only going to solve on the surface grid,
--!								vertical interfaces may introduce an unnecessary overhead.
--!								Default is true
--! @param numTargetProcs	(optional integer) The number of target processes to which the domain
--!							will be distributed. This shouldn't be more than
--!							there are elements in the top-level.
--!							Default is NumProcs()
--! @param distributionLevel	(optional integer) Sets the level on which distribution
--!								is performed. Default is the domains top-level.
--!								Currently only supported in partitioningMethod "metis".
--! @param wFct 			(optional SmartPtr<EdgeWeighting>) Sets the weighting function for the
--!							'metisReweigh' partitioning method.
function util.DistributeDomain(dom, partitioningMethod, verticalInterfaces,
							   numTargetProcs, distributionLevel, wFct)
	if NumProcs() == 1 then
		return true
	end
	
	local partitionMap = PartitionMap()
		
	if partitioningMethod == nil then
		partitioningMethod = "bisection"
	end
	
	if verticalInterfaces == nil then
		verticalInterfaces = true
	end
	
	if numTargetProcs == nil then
		numTargetProcs = NumProcs()
	end
	
	if distributionLevel == nil then
		distributionLevel = dom:grid():num_levels() - 1
		if distributionLevel < 0 then
			distributionLevel = 0
		end
	end
	
	if dom:domain_info():num_elements_on_level(distributionLevel) < numTargetProcs then
		print("\nWARNING in DistributeDomain:")
		print("    There are less elements on distributionLevel than there are target processes.")
		print("    If ug hangs during parallel execution, consider increasing numPreRefs to avoid this!")
		print("    num elements on level " .. distributionLevel .. ": "
			.. dom:domain_info():num_elements_on_level(distributionLevel))
		print("    num target processes: " .. numTargetProcs)
		print("")
	end
	
	if partitioningMethod == "bisection" then
		if distributionLevel < dom:grid():num_levels() - 1 then
			print("WARNING in util.DistributeDomain: 'bisection' can currently "
				  .. "only be performed on the top level. Sorry...")
		end 
		util.PartitionMapBisection(dom, partitionMap, numTargetProcs)
	elseif partitioningMethod == "metis" then
		util.PartitionMapMetis(dom, partitionMap, numTargetProcs, distributionLevel)
	elseif partitioningMethod == "metisReweigh" then
		if wFct ~= nil then
			util.PartitionMapMetisReweigh(dom, partitionMap, numTargetProcs, distributionLevel, wFct)
		else 
			print("ERROR in util.DistributeDomain: requested partitionMethod \"metisReweigh\", but no weightingFct given.")
			return
		end
	else
		print("ERROR in util.DistributeDomain: Unknown partitioning method.")
		print("  Valid partitioning methods are: 'bisection' and 'metis'")
		return
	end
	
	local success = DistributeDomain(dom, partitionMap, verticalInterfaces)
	
	delete(partitionMap)
	
	return success
end


--! create a partition map by performing repeated bisection
function util.PartitionMapBisection(dom, partitionMapOut, numProcs)
	if(partitionMapOut:num_target_procs() ~= numProcs) then
		partitionMapOut:clear()
		partitionMapOut:add_target_procs(0, numProcs)
	end

	procH = ProcessHierarchy()
	if(dom:grid():num_levels() > 0) then
		procH:add_hierarchy_level(dom:grid():num_levels() - 1, numProcs)
	else
		procH:add_hierarchy_level(0, numProcs)
	end
	
	local partitioner = nil
	if(dom:domain_info():element_type() == dom:get_dim() - 2)
		partitioner = HyperManifoldPartitioner_DynamicBisection(dom)
	elseif(dom:domain_info():element_type() == dom:get_dim() - 1)
		partitioner = ManifoldPartitioner_DynamicBisection(dom)
	elseif(dom:domain_info():element_type() == dom:get_dim())
		partitioner = Partitioner_DynamicBisection(dom)
	end
	
	if(partitioner == nil) then return; end
	
	partitioner:enable_clustered_siblings(false)
	partitioner:set_verbose(false)
	partitioner:enable_static_partitioning(true)
	partitioner:set_subset_handler(partitionMapOut:get_partition_handler())
	partitioner:set_next_process_hierarchy(procH)
	partitioner:partition(0, 0)
end

--! create a partition map by using metis graph partitioning.
--! This only works if Metis is available in the current build.
--! Use PartitionDomain_MetisKWay directly in this case.
function util.PartitionMapMetis(dom, partitionMapOut, numProcs, baseLevel)
	if(partitionMapOut:num_target_procs() ~= numProcs) then
		partitionMapOut:clear()
		partitionMapOut:add_target_procs(0, numProcs)
	end
	PartitionDomain_MetisKWay(dom, partitionMapOut, numProcs, baseLevel, 1, 1)
end

--! create a partition map by using metis graph partitioning anf correct
--! weights of dual graph edges by weighting function.
--! This only works if Metis is available in the current build.
--! Use PartitionDomain_MetisKWay directly in this case.
function util.PartitionMapMetisReweigh(dom, partitionMapOut, numProcs, baseLevel, wFct)
	if(partitionMapOut:num_target_procs() ~= numProcs) then
		partitionMapOut:clear()
		partitionMapOut:add_target_procs(0, numProcs)
	end
	PartitionDomain_MetisKWay(dom, partitionMapOut, numProcs, baseLevel, wFct)
end


--! performs lexicographic ordering from the lower left to the upper right.
--! Since each node can consist of multiple processes, we can further
--! perform a 'hierarchical' lexicographic ordering.
--! Set numProcsPerNode to 1 to retrieve a default ordering.
--! Set it to any square of n (n = 2, 3, ...) to retrieve lexicographic ordered
--! subgrids in each node (a node is considered to be a square grid itself).
--! If numProcsPerNode is not a square-number, then it will be defaulted to 1.
--! Since this method adds target procs 0, ..., numNodesX * numNodesY * numProcsPerNode
--! to the given partitionMap, it is not suited for redistribution.
function util.PartitionMapLexicographic2D(dom, partitionMapOut, numNodesX,
										  numNodesY, numProcsPerNode)
	partitionMapOut:clear()
	local strWarning = "WARNING in util.PartitionMap_Lexicographic: "
	
--	get the subgrid width and make sure that numProcsPerNode is a square number.
--	Default them to 1, if numProcsPerNode is not a square number.
	local subGridWidth = math.floor(math.sqrt(numProcsPerNode))
	if subGridWidth * subGridWidth ~= numProcsPerNode then
		print(strWarning .. "numProcsPerNode has to be a square number. Defaulting to 1.")				
		numProcsPerNode = 1
		subGridWidth = 1
	end
	
--	Assign process-ids to the partitionMap. Consider the subgrids...
	local curProcX = 0
	
	for iy = 0, numNodesY - 1 do
		for subY = 0, subGridWidth - 1 do
			local curProcX = iy * numNodesX * numProcsPerNode + subY * subGridWidth
			for ix = 0, numNodesX - 1 do
				for subX = 0, subGridWidth - 1 do
					partitionMapOut:add_target_proc(curProcX + subX)
				end
				curProcX = curProcX + numProcsPerNode
			end
		end
	end
	
--	now perform the actual partitioning.
	local procsX = numNodesX * subGridWidth
	local procsY = numNodesY * subGridWidth
	local procsZ = 1
	
--	TODO: the 'surfaceOnly' parameter should be handled in a more flexible way.
	PartitionDomain_RegularGrid(dom, partitionMapOut, procsX, procsY, procsZ, true)
end

--! Partitions the grid by assigning each element to a process corresponding to 
--! the index of the cell in which the elements center lies.
--! Make sure that numNodesX, numNodesY and numNodesZ are >= 1.
function util.PartitionMapRegularGrid(dom, partitionMapOut, numNodesX, numNodesY, numNodesZ)
	local numProcsRequired = numNodesX * numNodesY * numNodesZ
	if NumProcs() < (numProcsRequired) then
		print("Not enough processes allocated. At least " .. numProcsRequired .. " processes are required!")
		exit()
	end
	if(partitionMapOut:num_target_procs() ~= numProcsRequired) then
		partitionMapOut:clear()
		partitionMapOut:add_target_procs(0, numProcsRequired)
	end
	PartitionDomain_RegularGrid(dom, partitionMapOut, numNodesX, numNodesY, numNodesZ, true)
end

--[[!
\}
]]--
