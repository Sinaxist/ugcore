----------------------------------------------------------
--
--   Lua - Script to perform the Laplace-Problem
--
--   Author: Martin Rupp / Andreas Vogel
--
----------------------------------------------------------
SetOutputProfileStats(false)

ug_load_script("ug_util.lua")

-- constants
if util.HasParamOption("-3d") then
	dim = 3
else
	dim = 2
end

-- choose dimension and algebra
InitUG(dim, AlgebraType("CPU", 1));

gridName = util.GetParam("-grid", "unit_square_01/unit_square_01_tri_2x2.ugx")

-- choose number of pre-Refinements (before sending grid onto different processes)	
numPreRefs = util.GetParamNumber("-numPreRefs", 0)

-- choose number of total Refinements (incl. pre-Refinements)
numRefs = util.GetParamNumber("-numRefs", 3)

maxBase = util.GetParamNumber("-maxBase", 1000)

RAepsilon = util.GetParamNumber("-RAepsilon", 0.1)
RAalpha = util.GetParamNumber("-RAalpha", 45)

bFileOutput = false
bOutput = true

bRSAMG = util.HasParamOption("-rsamg")

print("Parameters: ")
print("    numPreRefs = "..numPreRefs)
print("    numRefs = "..numRefs)
print("    maxBase = "..maxBase)
print("    dim = "..dim)
print("    gridName = "..gridName)
print("    RAepsilon = "..RAepsilon)
print("    RAalpha = "..RAalpha.." degree")
RAalpha = RAalpha * (2*math.pi/360)
print("    RAalpha = "..RAalpha.." grad")

--------------------------------
-- User Data Functions (begin)
--------------------------------
	function ourDiffTensor2d(x, y, t)
		return	1, 0, 
				0, 1
	end
	
	function CreateRotatedAnisotropyMatrix2d(alpha, epsilon)
		local sinalpha = math.sin(alpha)
		local cosalpha = math.cos(alpha)
		RAmat = ConstUserMatrix2d()
		print((sinalpha*sinalpha + epsilon*cosalpha*cosalpha)..", "..(1-epsilon)*sinalpha*cosalpha)
		print((1-epsilon)*sinalpha*cosalpha..", "..epsilon*sinalpha*sinalpha + cosalpha*cosalpha)
		RAmat:set_entry(0, 0, sinalpha*sinalpha + epsilon*cosalpha*cosalpha)
		RAmat:set_entry(0, 1, (1-epsilon)*sinalpha*cosalpha)
		RAmat:set_entry(1, 0, (1-epsilon)*sinalpha*cosalpha)
		RAmat:set_entry(1, 1, epsilon*sinalpha*sinalpha + cosalpha*cosalpha)
		return RAmat
	end		
	
	function ourVelocityField2d(x, y, t)
		return	0, 0
	end
	
	function ourReaction2d(x, y, t)
		return	0
	end
	
	function ourRhs2d(x, y, t)
		return 0
	end
	
	function ourNeumannBnd2d(x, y, t)
		return true, -2*x*y
	end
	
	function ourDirichletBnd2d(x, y, t)
		return true, 0
	end

	----------------------------
-- User Data Functions (end)
--------------------------------

-- create Instance of a Domain
dom = Domain()
if util.LoadDomain(dom, gridName) == false then
	print("Loading Domain failed.")
	exit()
end

-- refine
refiner = GlobalDomainRefiner(dom)
for i=1,numPreRefs do
refiner:refine()
end

-- Distribute the domain to all involved processes
if DistributeDomain(dom) == false then
print("Error while Distributing Grid.")
exit()
end

for i=numPreRefs+1,numRefs do
	refiner:refine()
end

sh = dom:get_subset_handler()
if sh:num_subsets() ~= 2 then 
print("Domain must have 2 Subsets for this problem.")
exit()
end
sh:set_subset_name("Inner", 0)
sh:set_subset_name("DirichletBoundary", 1)

-- create Approximation Space
approxSpace = ApproximationSpace(dom)
approxSpace:add_fct("c", "Lagrange", 1)
approxSpace:init()
-- approxSpace:print_layout_statistic()
-- approxSpace:print_statistic()

-------------------------------------------
--  Setup User Functions
-------------------------------------------
diffusionMatrix = CreateRotatedAnisotropyMatrix2d(RAalpha, RAepsilon)
velocityField = LuaUserVector("ourVelocityField"..dim.."d")
reaction = LuaUserNumber("ourReaction"..dim.."d")
rhs = LuaUserNumber("ourRhs"..dim.."d")
dirichlet = LuaBoundaryNumber("ourDirichletBnd"..dim.."d")

-----------------------------------------------------------------
--  Setup FV Convection-Diffusion Element Discretization
-----------------------------------------------------------------
if dim == 2 then
upwind = WeightedUpwind2d(); 
else
upwind = WeightedUpwind3d();
end
upwind:set_weight(0.0)
elemDisc = util.CreateFV1ConvDiff(approxSpace, "c", "Inner")
if elemDisc:set_upwind(upwind) == false then exit() end
elemDisc:set_diffusion_tensor(diffusionMatrix)
elemDisc:set_velocity_field(velocityField)
elemDisc:set_reaction(reaction)
elemDisc:set_source(rhs)

-----------------------------------------------------------------
--  Setup Dirichlet Boundary
-----------------------------------------------------------------

dirichletBND = util.CreateDirichletBoundary(approxSpace)
dirichletBND:add(dirichlet, "c", "DirichletBoundary")

-------------------------------------------
--  Setup Domain Discretization
-------------------------------------------

domainDisc = DomainDiscretization()
domainDisc:set_approximation_space(approxSpace)
domainDisc:add(elemDisc)
domainDisc:add(dirichletBND)

-------------------------------------------
--  Algebra
-------------------------------------------
print ("Setting up Algebra Solver")

-- create operator from discretization
linOp = AssembledLinearOperator()
linOp:set_discretization(domainDisc)
linOp:set_dof_distribution(approxSpace:get_surface_dof_distribution())

-- get grid function
u = approxSpace:create_surface_function()
b = approxSpace:create_surface_function()
b2 = approxSpace:create_surface_function()

-- set initial value
u:set_random(-1.0, 1.0)

-- init Operator
linOp:init_op_and_rhs(b)
linOp:set_dirichlet_values(u)

jac = Jacobi()
jac:set_damp(0.6)
gs = GaussSeidel()
sgs = SymmetricGaussSeidel()
bgs = BackwardGaussSeidel()
ilu = ILU()
ilut = ILUT()


-- create Base Solver
baseConvCheck = StandardConvergenceCheck()
baseConvCheck:set_maximum_steps(500)
baseConvCheck:set_minimum_defect(1e-16)
baseConvCheck:set_reduction(1e-16)
baseConvCheck:set_verbose_level(false)

if false then
	base = LinearSolver()
	base:set_convergence_check(baseConvCheck)
	base:set_preconditioner(jac)
else
	base = LU()
end

-- create AMG ---
-----------------

if bRSAMG then
	print ("create AMG... ")
	amg = RSAMGPreconditioner()
	--amg:enable_aggressive_coarsening_A(2)
else
	print ("create FAMG... ")
	-- Testvectors for FAMG ---
	--------------------------	
	function CreateAMGTestvector(gridfunction, luaCallbackName, dim)
		local amgTestvector;
		if dim == 1 then
			amgTestvector = GridFunctionVectorWriter1d()
		elseif dim == 2 then
			amgTestvector = GridFunctionVectorWriter2d()
		elseif dim == 3 then
			amgTestvector = GridFunctionVectorWriter3d()
		end
		amgTestvector:set_reference_grid_function(gridfunction)
		amgTestvector:set_user_data(LuaUserNumber(luaCallbackName))
		return amgTestvector	
	end
	
	
	
	function CreateAMGTestvectorDirichlet0(dirichletBND, approxSpace)
		local amgDirichlet0
		if dim == 2 then
		amgDirichlet0 = GridFunctionVectorWriterDirichlet02d()
		else
		amgDirichlet0 = GridFunctionVectorWriterDirichlet03d()
		end
		amgDirichlet0:init(dirichletBND, approxSpace)
		return amgDirichlet0
	end
	
	
	function ourTestvector2d_0_0(x, y, t)
		return 0
	end
	
	function ourTestvector2d_1_1(x, y, t)
		return math.sin(math.pi*x)*math.sin(math.pi*y)
	end
	
	function ourTestvector2d_2_1(x, y, t)
		return math.sin(2*math.pi*x)*math.sin(math.pi*y)
	end
	
	
	function ourTestvector2d_1_2(x, y, t)
		return math.sin(math.pi*x)*math.sin(2*math.pi*y)
	end
	
	
	function ourTestvector2d_2_2(x, y, t)
		return math.sin(2*math.pi*x)*math.sin(2*math.pi*y)
	end

	amg = FAMGPreconditioner()	
	amg:set_delta(0.5) -- "Interpolation quality" F may not be worse than this (F < m_delta)
	amg:set_theta(0.95) -- with multiple parents paris, discard pairs with m_theta * F > min F.
	amg:set_epsilon_truncation(0.3) -- parameter used for truncation of interpolation
	amg:set_aggressive_coarsening(true)
	amg:set_damping_for_smoother_in_interpolation_calculation(0.5)
	-- add testvectors with lua callbacks (see ourTestvector2d_1_1)
	-- amg:add_vector_writer(CreateAMGTestvector(u, "ourTestvector2d_0_0", dim), 1.0)
	-- amg:add_vector_writer(CreateAMGTestvector(u, "ourTestvector2d_1_1", dim), 1.0)
	-- amg:add_vector_writer(CreateAMGTestvector(u, "ourTestvector2d_1_2", dim), 1.0)
	-- amg:add_vector_writer(CreateAMGTestvector(u, "ourTestvector2d_2_1", dim), 1.0)
	-- amg:add_vector_writer(CreateAMGTestvector(u, "ourTestvector2d_2_2", dim), 1.0)
	
	-- add testvector which is 1 everywhere and only 0 on the dirichlet Boundary.
	testvectorwriter = CreateAMGTestvectorDirichlet0(dirichletBND, approxSpace)
	-- testvectorwriter = CreateAMGTestvector(u, "ourTestvector2d_1_1", dim)
	
	-- your algebraic testvector
	testvector = approxSpace:create_surface_function()
	-- SaveVectorForConnectionViewer(testvector, "testvector.vec")
	-- there you write it
	testvectorwriter:update(testvector)
	
	amg:add_vector_writer(testvectorwriter, 1.0)
	amg:set_testvector_damps(1)
	
	amg:set_debug_level_calculate_parent_pairs(0)
	amg:set_debug_level_precalculate_coarsening(0)
	amg:set_debug_level_aggressive_coarsening(0)	
end


if dim == 2 then
vectorWriter = GridFunctionPositionProvider2d()
vectorWriter:set_reference_grid_function(u)
amg:set_position_provider2d(vectorWriter)
else
vectorWriter = GridFunctionPositionProvider3d()
vectorWriter:set_reference_grid_function(u)
amg:set_position_provider3d(vectorWriter)
end
if bOutput then
amg:set_matrix_write_path("/Users/mrupp/matrices/")
end

amg:set_num_presmooth(3)
amg:set_num_postsmooth(3)
amg:set_cycle_type(1)
amg:set_presmoother(gs)
amg:set_postsmoother(gs)

amg:set_base_solver(base)


amg:set_max_levels(2)

-- amg:set_min_nodes_on_one_processor(200) not functional
amg:set_max_nodes_for_base(maxBase)
amg:set_max_fill_before_base(0.7)
amg:set_fsmoothing(true)
amg:set_epsilon_truncation(0)

amg:tostring()


-- create Convergence Check
convCheck = StandardConvergenceCheck()
convCheck:set_maximum_steps(30)
convCheck:set_minimum_defect(1e-11)
convCheck:set_reduction(1e-12)

print("done.")
-- create Linear Solver
linSolver = LinearSolver()
linSolver:set_preconditioner(amg)
linSolver:set_convergence_check(convCheck)

-- Apply Solver

log = GetLogAssistant();

-------------------------------------------
--  Apply Solver
-------------------------------------------
-- 1. init
linOp:init_op_and_rhs(b2)
linSolver:init(linOp)

-- 2. apply solver
tBefore = os.clock()
linSolver:apply_return_defect(u,b)
tSolve = os.clock()-tBefore
WriteGridFunctionToVTK(u, "Solution")


print("done")

if bOutput then
WriteGridFunctionToVTK(u, "Solution")
end

amg:check(u, b)
amg:check2(u, b)

log:set_debug_levels(0)

printf = function(s,...)
	print(s:format(...))
end -- function


if bFileOutput then
output = io.open("output.txt", "a")

output:write("dim\t"..dim)
output:write("\tprocs\t"..GetNumProcesses())
-- output:write("\tgridName\t"..gridName)
-- output:write("\tnumPreRefs\t"..numPreRefs)
output:write("\tnumRefs\t".. numRefs)
output:write("\tanisotropy\t".. RAepsilon)
output:write("\talpha\t"..(RAalpha * (180/math.pi)))
output:write("\tsteps\t"..convCheck:step())
output:write("\tlast reduction\t".. convCheck:defect()/convCheck:previous_defect())
output:write("\tamg_timing_whole_setup\t".. amg:get_timing_whole_setup_ms())
output:write("\tc_A\t".. amg:get_operator_complexity())
output:write("\tc_G\t".. amg:get_grid_complexity())
output:write("\tusedLevels\t"..amg:get_used_levels())
output:write("\ttSolve [ms]\t".. tSolve)
output:write("\n")
print(s)
else

print("AMG Information:")
printf("Setup took %.2f ms",amg:get_timing_whole_setup_ms())
printf("Coarse solver setup took %.2f ms",amg:get_timing_coarse_solver_setup_ms())
printf("Operator complexity c_A = %.2f", amg:get_operator_complexity())
printf("Grid complexity c_G = %.2f", amg:get_grid_complexity())

LI = amg:get_level_information(0)
printf("Level 0: number of nodes: %d. fill in %.2f%%, nr of interface elements: %d (%.2f%%)", LI:get_nr_of_nodes(), LI:get_fill_in()*100, LI:get_nr_of_interface_elements(), LI:get_nr_of_interface_elements()/LI:get_nr_of_nodes()*100)
for i = 1, amg:get_used_levels()-1 do
	LI = amg:get_level_information(i)
	printf("Level %d: creation time: %.2f ms. number of nodes: %d. fill in %.2f%%. coarsening rate: %.2f%%, nr of interface elements: %d (%.2f%%)", i, 
		LI:get_creation_time_ms(),  LI:get_nr_of_nodes(), LI:get_fill_in()*100,
		LI:get_nr_of_nodes()*100/amg:get_level_information(i-1):get_nr_of_nodes(), LI:get_nr_of_interface_elements(), LI:get_nr_of_interface_elements()/LI:get_nr_of_nodes()*100)
		
end


if GetProfilerAvailable() == true then
	
	create_levelPN = GetProfileNode("c_create_AMG_level")
	
	function PrintParallelProfileNode(name)
		pn = GetProfileNode(name)
		t = pn:get_avg_total_time_ms()/to100 * 100
		tmin = ParallelMin(t)
		tmax = ParallelMax(t)
		printf("%s:\n%.2f %%, min: %.2f %%, max: %.2f %%", name, t, tmin, tmax)
	end
	
	if create_levelPN:is_valid() then
		if true then
			print(create_levelPN:call_tree())
			print(create_levelPN:child_self_time_sorted())
		end
		to100 = create_levelPN:get_avg_total_time_ms()
		PrintParallelProfileNode("create_OL2_matrix")
		PrintParallelProfileNode("CalculateTestvector")
		PrintParallelProfileNode("CreateSymmConnectivityGraph")
		PrintParallelProfileNode("calculate_all_possible_parent_pairs")
		PrintParallelProfileNode("color_process_graph")
		PrintParallelProfileNode("FAMG_recv_coarsening_communicate")
		PrintParallelProfileNode("update_rating")
		PrintParallelProfileNode("precalculate_coarsening")
		PrintParallelProfileNode("send_coarsening_data_to_processes_with_higher_color")
		PrintParallelProfileNode("communicate_prolongation")
		PrintParallelProfileNode("create_new_index")
		PrintParallelProfileNode("create_parent_index")
		PrintParallelProfileNode("create_interfaces")
		PrintParallelProfileNode("create_fine_marks")
		PrintParallelProfileNode("FAMGCreateAsMultiplyOf")
		PrintParallelProfileNode("CalculateNextTestvector")
	end

else
	print("Profiler not available.")
end	
	

end
-- print("-----------------------------------------------")
-- amg:check2(u, b);



-- Output
