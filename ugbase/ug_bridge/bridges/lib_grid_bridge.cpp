//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m09 d20

#include <iostream>
#include <sstream>
#include "../registry.h"
#include "../ug_bridge.h"
#include "lib_grid/lib_grid.h"
#include "common/profiler/profiler.h"

using namespace std;

namespace ug
{
namespace bridge
{

///	Wrapper object that simplifies script creation
class GridObject : public Grid
{
	public:
		GridObject() : Grid(GRIDOPT_STANDARD_INTERCONNECTION), m_sh(*this)	{}
		inline Grid& get_grid()	{return *this;}
		inline SubsetHandler& get_subset_handler()	{return m_sh;}
		
	protected:
		SubsetHandler m_sh;
};

bool LoadGridObject(GridObject& go, const char* filename)
{
	return LoadGridFromFile(go.get_grid(), filename, go.get_subset_handler());
}

bool SaveGridObject(GridObject& go, const char* filename)
{
	return SaveGridToFile(go.get_grid(), filename, go.get_subset_handler());
}

GridObject* CreateGridObject(const char* filename)
{
	GridObject* go = new GridObject;
	if(!LoadGridObject(*go, filename)){
		delete go;
		return NULL;
	}
	return go;
}

bool CreateFractal(Grid& grid, HangingNodeRefiner_Grid& href,
					number scaleFac, size_t numIterations)
{
	PROFILE_FUNC();
//	HangingNodeRefiner_IR1 href(grid);
	return CreateFractal_NormalScale(grid, href, scaleFac, numIterations);
//	return true;
}

///	Saves a grid hierarchy by offsetting levels along the z-axis.
/**	Note that this method might better be implemented for domains.*/
void SaveGridHierarchyTransformed(MultiGrid& mg, const SubsetHandler& csh,
								  const char* filename, number offset)
{
//	cast away constness
	SubsetHandler& sh = *const_cast<SubsetHandler*>(&csh);

	APosition aPos;
//	uses auto-attach
	Grid::AttachmentAccessor<VertexBase, APosition> aaPos(mg, aPos, true);

//	copy the existing position to aPos. We take care of dimension differences.
//	Note:	if the method was implemented for domains, this could be implemented
//			in a nicer way.
	if(mg.has_vertex_attachment(aPosition))
		ConvertMathVectorAttachmentValues<VertexBase>(mg, aPosition, aPos);
	else if(mg.has_vertex_attachment(aPosition2))
		ConvertMathVectorAttachmentValues<VertexBase>(mg, aPosition2, aPos);
	else if(mg.has_vertex_attachment(aPosition1))
		ConvertMathVectorAttachmentValues<VertexBase>(mg, aPosition1, aPos);

//	iterate through all vertices and apply an offset depending on their level.
	for(size_t lvl = 0; lvl < mg.num_levels(); ++lvl){
		for(VertexBaseIterator iter = mg.begin<VertexBase>(lvl);
			iter != mg.end<VertexBase>(lvl); ++iter)
		{
			aaPos[*iter].z += (number)lvl * offset;
		}
	}

//	finally save the grid
	//SaveGridToFile(mg, filename, sh, aPos);
	SaveGridToUGX(mg, sh, filename, aPos);

//	clean up
	mg.detach_from_vertices(aPos);
}


bool LoadGrid(Grid& grid, ISubsetHandler& sh, const char* filename)
{
	return LoadGridFromFile(grid, filename, sh);
}

bool LoadGrid(Grid& grid, const char* filename)
{
	return LoadGridFromFile(grid, filename);
}

bool SaveGrid(Grid& grid, const char* filename)
{
	return SaveGridToFile(grid, filename);
}

bool SaveGrid(Grid& grid, SubsetHandler& sh, const char* filename)
{
	return SaveGridToFile(grid, filename, sh);
}

bool SaveGrid(Grid& grid, const SubsetHandler& sh, const char* filename)
{
	return SaveGridToFile(grid, filename, *const_cast<SubsetHandler*>(&sh));
}

bool SaveGridHierarchy(MultiGrid& mg, const char* filename)
{
	return SaveGridToFile(mg, filename, mg.get_hierarchy_handler());
}


void TestSubdivision(const char* fileIn, const char* fileOut, int numRefs)
{
//todo: Callbacks have to make sure that their attachment is accessible in the grid.
//		even if they were initialized before the attachment was attached to the grid.
	MultiGrid mg;
	SubsetHandler sh(mg);
	RefinementCallbackSubdivisionLoop<APosition> refCallback(mg, aPosition, aPosition);
	GlobalMultiGridRefiner ref(mg, &refCallback);
	
	if(LoadGridFromFile(mg, fileIn, sh)){
		for(int lvl = 0; lvl < numRefs; ++lvl){
			ref.refine();
		}

		ProjectToLimitPLoop(mg, aPosition, aPosition);
		SaveGridToFile(mg, fileOut, mg.get_hierarchy_handler());

	}
	else{
		UG_LOG("Load failed. aborting...\n");
	}
}

bool CreateSmoothHierarchy(MultiGrid& mg, size_t numRefs)
{
	IRefinementCallback* refCallback = NULL;
//	we're only checking for the main attachments here.
//todo: improve this - add a domain-based hierarchy creator.
	if(mg.has_vertex_attachment(aPosition1))
		refCallback = new RefinementCallbackSubdivisionLoop<APosition1>(mg, aPosition1, aPosition1);
	else if(mg.has_vertex_attachment(aPosition2))
		refCallback = new RefinementCallbackSubdivisionLoop<APosition2>(mg, aPosition2, aPosition2);
	else if(mg.has_vertex_attachment(aPosition))
		refCallback = new RefinementCallbackSubdivisionLoop<APosition>(mg, aPosition, aPosition);
		
	if(!refCallback){
		UG_LOG("No standard position attachment found. Aborting.\n");
		return false;
	}
	
	GlobalMultiGridRefiner ref(mg, refCallback);

	for(size_t lvl = 0; lvl < numRefs; ++lvl){
		ref.refine();
	}

	if(mg.has_vertex_attachment(aPosition1))
		ProjectToLimitPLoop(mg, aPosition1, aPosition1);
	else if(mg.has_vertex_attachment(aPosition2))
		ProjectToLimitPLoop(mg, aPosition2, aPosition2);
	else if(mg.has_vertex_attachment(aPosition))
		ProjectToLimitPLoop(mg, aPosition, aPosition);

	delete refCallback;
	return true;
}

bool CreateSemiSmoothHierarchy(MultiGrid& mg, size_t numRefs)
{
	IRefinementCallback* refCallback = NULL;
//	we're only checking for the main attachments here.
//todo: improve this - add a domain-based hierarchy creator.
	if(mg.has_vertex_attachment(aPosition1))
		refCallback = new RefinementCallbackSubdivBoundary<APosition1>(mg, aPosition1, aPosition1);
	else if(mg.has_vertex_attachment(aPosition2))
		refCallback = new RefinementCallbackSubdivBoundary<APosition2>(mg, aPosition2, aPosition2);
	else if(mg.has_vertex_attachment(aPosition))
		refCallback = new RefinementCallbackSubdivBoundary<APosition>(mg, aPosition, aPosition);
		
	if(!refCallback){
		UG_LOG("No standard position attachment found. Aborting.\n");
		return false;
	}
	
	GlobalMultiGridRefiner ref(mg, refCallback);

	for(size_t lvl = 0; lvl < numRefs; ++lvl){
		ref.refine();
	}

	if(mg.has_vertex_attachment(aPosition1))
		ProjectToLimitSubdivBoundary(mg, aPosition1, aPosition1);
	else if(mg.has_vertex_attachment(aPosition2))
		ProjectToLimitSubdivBoundary(mg, aPosition2, aPosition2);
	else if(mg.has_vertex_attachment(aPosition))
		ProjectToLimitSubdivBoundary(mg, aPosition, aPosition);

	delete refCallback;
	return true;
}

template <class TElem>
void MarkForRefinement(MultiGrid& mg,
					  IRefiner& refiner,
					  float percentage)
{

	typedef typename geometry_traits<TElem>::iterator iterator;
	for(iterator iter = mg.begin<TElem>(); iter != mg.end<TElem>(); ++iter)
	{
		if(urand<float>(0, 99) < percentage){
			refiner.mark(*iter);
		}
	}

/*
	Grid::VertexAttachmentAccessor<APosition> aaPos(mg, aPosition);
	TElem* elem = FindByCoordinate<TElem>(vector3(-0.00001, -0.00001, -0.00001),
									mg.begin<TElem>(mg.num_levels()-1),
									mg.end<TElem>(mg.num_levels()-1),
									aaPos);

	if(elem)
		refiner.mark(elem);
	else{
		UG_LOG("No element found for refinement.\n");
	}*/


}

bool TestHangingNodeRefiner_MultiGrid(const char* filename,
									  const char* outFilename,
									  int numIterations,
									  float percentage)
{
	MultiGrid mg;
	MGSubsetHandler sh(mg);
	HangingNodeRefiner_MultiGrid refiner(mg);

	PROFILE_BEGIN(PROFTEST_loading);
	if(!LoadGridFromFile(mg, filename, sh)){
		UG_LOG("  could not load " << filename << endl);
		return false;
	}
	PROFILE_END();

	{
		PROFILE_BEGIN(PROFTEST_refining);
		for(int i = 0; i < numIterations; ++i){
			UG_LOG("refinement step " << i+1 << endl);

			if(mg.num<Volume>() > 0)
				MarkForRefinement<Volume>(mg, refiner, percentage);
			else if(mg.num<Face>() > 0)
				MarkForRefinement<Face>(mg, refiner, percentage);
			else
				MarkForRefinement<EdgeBase>(mg, refiner, percentage);

			refiner.refine();
		}
	}

	UG_LOG("saving to " << outFilename << endl;)
	SaveGridHierarchy(mg, outFilename);

	UG_LOG("grid element numbers:\n");
	PrintGridElementNumbers(mg);

//	create a surface view
	SurfaceView surfView(mg);

	CreateSurfaceView(surfView, mg, sh);

	SaveGridToFile(mg, "surface_view.ugx", surfView);

	UG_LOG("surface view element numbers:\n");
	PrintGridElementNumbers(surfView);

	return true;
}


template <class TAPos, class vector_t>
void MarkForRefinement_VerticesInSphere(Grid& grid, IRefiner& refiner,
										const vector_t& center,
										number radius, TAPos& aPos)
{
	if(!grid.has_vertex_attachment(aPos)){
		UG_LOG("WARNING in MarkForRefinement_VerticesInSphere: position attachment missing.\n");
		return;
	}

	Grid::VertexAttachmentAccessor<TAPos> aaPos(grid, aPos);

//	we'll compare against the square radius.
	number radiusSq = radius * radius;

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all vertices of the grid. If a vertex is inside the given sphere,
//	then we'll mark all associated elements.
	for(VertexBaseIterator iter = grid.begin<VertexBase>();
		iter != grid.end<VertexBase>(); ++iter)
	{
		if(VecDistanceSq(center, aaPos[*iter]) <= radiusSq){
			CollectAssociated(vEdges, grid, *iter);
			CollectAssociated(vFaces, grid, *iter);
			CollectAssociated(vVols, grid, *iter);

			refiner.mark(vEdges.begin(), vEdges.end());
			refiner.mark(vFaces.begin(), vFaces.end());
			refiner.mark(vVols.begin(), vVols.end());
		}
	}
}

////////////////////////////////////////////////////////////////////////
void MarkForRefinement_VerticesInSphere(IRefiner& refiner, number centerX,
							number centerY, number centerZ, number radius)
{
//	get the associated grid of the refiner.
	Grid* pGrid = refiner.get_associated_grid();
	if(!pGrid){
		UG_LOG("WARNING: Circular refinement failed: no grid assigned.\n");
		return;
	}

//	depending on the position attachment, we'll call different versions of
//	MarkForRefinement_VerticesInSphere.
	if(pGrid->has_vertex_attachment(aPosition1)){
		MarkForRefinement_VerticesInSphere(*pGrid, refiner, vector1(centerX),
											radius, aPosition1);
	}
	else if(pGrid->has_vertex_attachment(aPosition2)){
		MarkForRefinement_VerticesInSphere(*pGrid, refiner,
											vector2(centerX, centerY),
											radius, aPosition2);
	}
	else if(pGrid->has_vertex_attachment(aPosition)){
		MarkForRefinement_VerticesInSphere(*pGrid, refiner,
											vector3(centerX, centerY, centerZ),
											radius, aPosition);
	}
	else{
		UG_LOG("WARNING in MarkForRefinement_VerticesInSphere: No Position attachment found. Aborting.\n");
	}
}


template <class TAPos, class vector_t>
void MarkForRefinement_FacesInSphere(Grid& grid, IRefiner& refiner,
										const vector_t& center,
										number radius, TAPos& aPos)
{
	if(!grid.has_vertex_attachment(aPos)){
		UG_LOG("WARNING in MarkForRefinement_FacesInSphere: position attachment missing.\n");
		return;
	}

	Grid::VertexAttachmentAccessor<TAPos> aaPos(grid, aPos);

//	we'll compare against the square radius.
	number radiusSq = radius * radius;

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all faces of the grid. If all vertex of the face are inside
//	the given sphere then we'll mark all associated elements.
	for(FaceIterator iter = grid.begin<Face>();
		iter != grid.end<Face>(); ++iter)
	{
	//	get face
		Face* face = *iter;

	//	bool flag to check if in sphere
		bool bInSphere = true;

	//	loop all vertices
		for(size_t i = 0; i < face->num_vertices(); ++i)
		{
		//	get vertex
			VertexBase* vrt = face->vertex(i);

		//	check if vertex is in sphere
			if(VecDistanceSq(center, aaPos[vrt]) > radiusSq)
				bInSphere = false;
		}

		//	mark associated elements
		if(bInSphere){
			CollectAssociated(vEdges, grid, face);
			CollectAssociated(vFaces, grid, face);
			CollectAssociated(vVols, grid, face);

			refiner.mark(vEdges.begin(), vEdges.end());
			refiner.mark(vFaces.begin(), vFaces.end());
			refiner.mark(vVols.begin(), vVols.end());
		}
	}
}

////////////////////////////////////////////////////////////////////////
void MarkForRefinement_FacesInSphere(IRefiner& refiner, number centerX,
							number centerY, number centerZ, number radius)
{
//	get the associated grid of the refiner.
	Grid* pGrid = refiner.get_associated_grid();
	if(!pGrid){
		UG_LOG("WARNING: Circular refinement failed: no grid assigned.\n");
		return;
	}

//	depending on the position attachment, we'll call different versions of
//	MarkForRefinement_VerticesInSphere.
	if(pGrid->has_vertex_attachment(aPosition1)){
		MarkForRefinement_FacesInSphere(*pGrid, refiner, vector1(centerX),
											radius, aPosition1);
	}
	else if(pGrid->has_vertex_attachment(aPosition2)){
		MarkForRefinement_FacesInSphere(*pGrid, refiner,
											vector2(centerX, centerY),
											radius, aPosition2);
	}
	else if(pGrid->has_vertex_attachment(aPosition)){
		MarkForRefinement_FacesInSphere(*pGrid, refiner,
											vector3(centerX, centerY, centerZ),
											radius, aPosition);
	}
	else{
		UG_LOG("WARNING in MarkForRefinement_FacesInSphere: No Position attachment found. Aborting.\n");
	}
}


template <class TAPos, class vector_t>
void MarkForRefinement_VerticesInSquare(Grid& grid, IRefiner& refiner,
										const vector_t& min,
										const vector_t& max,
										TAPos& aPos)
{
	if(!grid.has_vertex_attachment(aPos)){
		UG_LOG("WARNING in MarkForRefinement_VerticesInSquare: position attachment missing.\n");
		return;
	}

	Grid::VertexAttachmentAccessor<TAPos> aaPos(grid, aPos);

//	we'll store associated edges, faces and volumes in those containers
	vector<EdgeBase*> vEdges;
	vector<Face*> vFaces;
	vector<Volume*> vVols;

//	iterate over all vertices of the grid. If a vertex is inside the given sphere,
//	then we'll mark all associated elements.
	for(VertexBaseIterator iter = grid.begin<VertexBase>();
		iter != grid.end<VertexBase>(); ++iter)
	{
	//	Position
		vector_t pos = aaPos[*iter];

	//	check flag
		bool bRefine = true;

	//	check node
		for(size_t d = 0; d < pos.size(); ++d)
			if(pos[d] < min[d] || max[d] < pos[d])
				bRefine = false;

		if(bRefine)
		{
			CollectAssociated(vEdges, grid, *iter);
			CollectAssociated(vFaces, grid, *iter);
			CollectAssociated(vVols, grid, *iter);

			refiner.mark(vEdges.begin(), vEdges.end());
			refiner.mark(vFaces.begin(), vFaces.end());
			refiner.mark(vVols.begin(), vVols.end());
		}
	}
}

////////////////////////////////////////////////////////////////////////
void MarkForRefinement_VerticesInSquare(IRefiner& refiner,
                                        number minX, number maxX,
                                        number minY, number maxY,
                                        number minZ, number maxZ)
{
//	get the associated grid of the refiner.
	Grid* pGrid = refiner.get_associated_grid();
	if(!pGrid){
		UG_LOG("WARNING: Square refinement failed: no grid assigned.\n");
		return;
	}

//	depending on the position attachment, we'll call different versions of
//	MarkForRefinement_VerticesInSphere.
	if(pGrid->has_vertex_attachment(aPosition1)){
		MarkForRefinement_VerticesInSquare(*pGrid, refiner, vector1(minX),
		                                   vector1(maxX), aPosition1);
	}
	else if(pGrid->has_vertex_attachment(aPosition2)){
		MarkForRefinement_VerticesInSquare(*pGrid, refiner,
											vector2(minX, minY),
											vector2(maxX, maxY), aPosition2);
	}
	else if(pGrid->has_vertex_attachment(aPosition)){
		MarkForRefinement_VerticesInSquare(*pGrid, refiner,
											vector3(minX, minY, minZ),
											vector3(maxX, maxY, maxZ), aPosition);
	}
	else{
		UG_LOG("WARNING in MarkForRefinement_VerticesInSquare: No Position attachment found. Aborting.\n");
	}
}

////////////////////////////////////////////////////////////////////////
///	Marks all elements from refinement.
/**	If used in a parallel environment only elements on the calling proc
 * are marked.
 */
void MarkForRefinement_All(IRefiner& ref)
{
	Grid* g = ref.get_associated_grid();
	if(!g){
		UG_LOG("Refiner is not registered at a grid. Aborting.\n");
		return;
	}
	ref.mark(g->vertices_begin(), g->vertices_end());
	ref.mark(g->edges_begin(), g->edges_end());
	ref.mark(g->faces_begin(), g->faces_end());
	ref.mark(g->volumes_begin(), g->volumes_end());
}

////////////////////////////////////////////////////////////////////////
///	This method only makes sense during the development of the grid redistribution
/**	Note that the source grid is completely distributed (no vertical interfaces).
 *
 * The method loads a grid on proc 0 and distributes half of it to proc 1.
 * It then creates a new partition map on each process and redistributes
 * half of the grid on process 0 to process 2 and half of the grid on
 * process 1 to process 3.
 */
void TestGridRedistribution(const char* filename)
{
#ifndef UG_PARALLEL
	UG_LOG("WARNING in TestGridRedistribution: ");
	UG_LOG("This method only works in a parallel environment.\n");
#else
	UG_LOG("Executing TestGridRedistribution...\n");
	MultiGrid mg;
	SubsetHandler sh(mg);
	DistributedGridManager distGridMgr(mg);
	GridLayoutMap& glm = distGridMgr.grid_layout_map();

	int numProcs = pcl::GetNumProcesses();
	if(numProcs != 4){
		UG_LOG(" This test-method only runs with exactly 4 processes.\n");
		return;
	}

UG_LOG(" performing initial distribution\n");
	if(pcl::GetProcRank() == 0){
	//	use this for loading and distribution.
		//MultiGrid tmg;
		//SubsetHandler tsh(tmg);
		SubsetHandler shPart(mg);

		bool success = true;
		string strError;

		if(!LoadGridFromFile(mg, filename, sh)){
			strError = "File not found.";
			success = false;
		}
		else{
		//	partition the grid once (only 2 partitions)
			if(mg.num_volumes() > 0){
				PartitionElementsByRepeatedIntersection<Volume, 3>(
												shPart, mg,
												mg.num_levels() - 1,
												2, aPosition);
			}
			else if(mg.num_faces() > 0){
				PartitionElementsByRepeatedIntersection<Face, 2>(
												shPart, mg,
												mg.num_levels() - 1,
												2, aPosition);
			}
			else{
				strError = "This test-method only runs on geometries which contain "
							"faces or volumes.\n";
				success = false;
			}
		}

	//	make sure that all processes exit if something went wrong.
		if(!pcl::AllProcsTrue(success)){
			UG_LOG(strError << endl);
			return;
		}

	//	now distribute the grid
		//if(!DistributeGrid(tmg, tsh, shPart, 0, &mg, &sh, &glm)){
		if(!DistributeGrid_KeepSrcGrid(mg, sh, glm, shPart, 0, false)){
			UG_LOG("Distribution failed\n");
			return;
		}
	}
	else if(pcl::GetProcRank() == 1){
		if(!pcl::AllProcsTrue(true)){
			UG_LOG("Problems occured on process 0. Aborting.\n");
			return;
		}

		if(!ReceiveGrid(mg, sh, glm, 0, true)){
			UG_LOG("Receive failed.\n");
			return;
		}
	}
	else{
		if(!pcl::AllProcsTrue(true)){
			UG_LOG("Problems occured on process 0. Aborting.\n");
			return;
		}
	}

	distGridMgr.grid_layouts_changed(true);

UG_LOG(" done\n");
UG_LOG(" preparing for redistribution\n");
////////////////////////////////
//	The grid is now distributed to two processes (proc 1 and proc 2).
//	note that it was distributed completely (no source grid kept).

////////////////////////////////
//	Now lets redistribute the grid.
	SubsetHandler shPart(mg);

//	at this point it is clear that we either have volumes or faces.
//	note that we now cut the geometry along the second coordinate (start counting at 0).
	int rank = pcl::GetProcRank();
	if(rank < 2){
		if(mg.num_volumes() > 0){
			PartitionElementsByRepeatedIntersection<Volume, 3>(
											shPart, mg,
											mg.num_levels() - 1,
											2, aPosition, 1);
		}
		else if(mg.num_faces() > 0){
			PartitionElementsByRepeatedIntersection<Face, 2>(
											shPart, mg,
											mg.num_levels() - 1,
											2, aPosition, 1);
		}
		else{
			throw UGError("TestGridRedistribution: Method should have been aborted earlier.");
		}


	//	since currently no proc-map is supported during redistribution, we will now
	//	adjust the subsets so that they match the process-ids.
	//	We do this by hand here. If a real process map would exist this could be automated.
		switch(rank){
			case 0:	shPart.move_subset(1, 2);
					break;

			case 1:	shPart.move_subset(1, 3);
					shPart.move_subset(0, 1);
					break;

			default:
					break;
		}

	//	save the partition maps
		{
			stringstream ss;
			ss << "partition_map_" << pcl::GetProcRank() << ".ugx";
			SaveGridToFile(mg, ss.str().c_str(), shPart);
		}
	}

UG_LOG(" done\n");
UG_LOG(" redistributing grid\n");

//	data serialization
	GeomObjAttachmentSerializer<VertexBase, APosition>
		posSerializer(mg, aPosition);
	SubsetHandlerSerializer shSerializer(sh);

	GridDataSerializationHandler serializer;
	serializer.add(&posSerializer);
	serializer.add(&shSerializer);

//	now call redistribution
	RedistributeGrid(distGridMgr, shPart, serializer, serializer, false);
UG_LOG("done\n");

/*
	for(size_t lvl = 0; lvl < mg.num_levels(); ++lvl){
		UG_LOG("level " << lvl << ":\n");
		PrintElementNumbers(mg.get_geometric_object_collection(lvl));
	}
*/
//	save the hierarchy on each process
	{
		stringstream ss;
		ss << "hierarchy_" << pcl::GetProcRank() << ".ugx";
		SaveGridHierarchy(mg, ss.str().c_str());
	}

//	save the domain on each process
	{
		stringstream ss;
		ss << "domain_" << pcl::GetProcRank() << ".ugx";
		SaveGridToFile(mg, ss.str().c_str(), sh);
	}

#endif // UG_PARALLEL
}

///	only for temporary testing purposes.
void TestAttachedLinkedList(const char* filename)
{
	Grid g;
	//SubsetHandler sh(g);

	UG_LOG("loading...");
	//if(!LoadGridFromFile(g, filename, sh)){
	if(!LoadGridFromFile(g, filename)){
		UG_LOG("load file failed\n");
		return;
	}
	UG_LOG(" done.\n");

	//UG_LOG("testing surface view... ");
	//SurfaceView sv(g);
	//sv.assign_subset(*g.begin<VertexBase>(), 0);
	//UG_LOG(" done\n");
}


////////////////////////////////////////////////////////////////////////
bool RegisterLibGridInterface(Registry& reg, const char* parentGroup)
{
	try
	{
	//	get group string
		std::stringstream groupString; groupString << parentGroup << "/Grid";
		std::string grp = groupString.str();

	//	Grid
		reg.add_class_<Grid>("Grid", grp.c_str())
			.add_constructor()
			.add_method("clear", (void (Grid::*)()) &Grid::clear)
			.add_method("num_vertices", &Grid::num_vertices)
			.add_method("num_edges", &Grid::num_edges)
			.add_method("num_faces", &Grid::num_faces)
			.add_method("num_triangles", &Grid::num<Triangle>)
			.add_method("num_quadrilaterals", &Grid::num<Quadrilateral>)
			.add_method("num_volumes", &Grid::num_volumes)
			.add_method("num_tetrahedrons", &Grid::num<Tetrahedron>)
			.add_method("num_pyramids", &Grid::num<Pyramid>)
			.add_method("num_prisms", &Grid::num<Prism>)
			.add_method("num_hexahedrons", &Grid::num<Hexahedron>)
			.add_method("reserve_vertices", &Grid::reserve<VertexBase>)
			.add_method("reserve_edges", &Grid::reserve<EdgeBase>)
			.add_method("reserve_faces", &Grid::reserve<Face>)
			.add_method("reserve_volumes", &Grid::reserve<Volume>);

	//	MultiGrid
		reg.add_class_<MultiGrid, Grid>("MultiGrid", grp.c_str())
			.add_constructor()
			.add_method("num_levels", &MultiGrid::num_levels)

			.add_method("num_vertices", (size_t (MultiGrid::*)() const) &MultiGrid::num<VertexBase>)
			.add_method("num_edges", (size_t (MultiGrid::*)() const) &MultiGrid::num<EdgeBase>)
			.add_method("num_faces", (size_t (MultiGrid::*)() const) &MultiGrid::num<Face>)
			.add_method("num_triangles", (size_t (MultiGrid::*)() const) &MultiGrid::num<Triangle>)
			.add_method("num_quadrilaterals", (size_t (MultiGrid::*)() const) &MultiGrid::num<Quadrilateral>)
			.add_method("num_volumes", (size_t (MultiGrid::*)() const) &MultiGrid::num<Volume>)
			.add_method("num_tetrahedrons", (size_t (MultiGrid::*)() const) &MultiGrid::num<Tetrahedron>)
			.add_method("num_pyramids", (size_t (MultiGrid::*)() const) &MultiGrid::num<Pyramid>)
			.add_method("num_prisms", (size_t (MultiGrid::*)() const) &MultiGrid::num<Prism>)

			.add_method("num_vertices", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<VertexBase>)
			.add_method("num_edges", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<EdgeBase>)
			.add_method("num_faces", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Face>)
			.add_method("num_triangles", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Triangle>)
			.add_method("num_quadrilaterals", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Quadrilateral>)
			.add_method("num_volumes", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Volume>)
			.add_method("num_tetrahedrons", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Tetrahedron>)
			.add_method("num_pyramids", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Pyramid>)
			.add_method("num_prisms", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Prism>)
			.add_method("num_hexahedrons", (size_t (MultiGrid::*)(int) const) &MultiGrid::num<Hexahedron>);


	////////////////////////
	//	SUBSET HANDLERS

	//  ISubsetHandler
		reg.add_class_<ISubsetHandler>("ISubsetHandler", grp.c_str())
			.add_method("num_subsets", &ISubsetHandler::num_subsets)
			.add_method("get_subset_name", &ISubsetHandler::get_subset_name)
			.add_method("set_subset_name", &ISubsetHandler::set_subset_name)
			.add_method("get_subset_index", (int (ISubsetHandler::*)(const char*) const)
											&ISubsetHandler::get_subset_index);
		
	//	SubsetHandler
		reg.add_class_<SubsetHandler, ISubsetHandler>("SubsetHandler", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", &SubsetHandler::assign_grid);

	//	MGSubsetHandler
		reg.add_class_<MGSubsetHandler, ISubsetHandler>("MGSubsetHandler", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", &MGSubsetHandler::assign_grid);

	//	SurfaceView
		reg.add_class_<SurfaceView, SubsetHandler>("SurfaceView", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", (void (SurfaceView::*)(MultiGrid&)) &SurfaceView::assign_grid);

		reg.add_function("CheckSurfaceView", &CheckSurfaceView, grp.c_str());


	////////////////////////
	//	REFINEMENT

	//	IRefiner
		reg.add_class_<IRefiner>("IRefiner", grp.c_str())
			.add_method("refine", &IRefiner::refine)
			.add_method("clear_marks", &IRefiner::clear_marks);

	//	HangingNodeRefiner
		reg.add_class_<HangingNodeRefiner_Grid, IRefiner>("HangingNodeRefiner_Grid", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", &HangingNodeRefiner_Grid::assign_grid);

		reg.add_class_<HangingNodeRefiner_MultiGrid, IRefiner>("HangingNodeRefiner_MultiGrid", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", &HangingNodeRefiner_MultiGrid::assign_grid);

	//	GlobalMultiGridRefiner
		reg.add_class_<GlobalMultiGridRefiner, IRefiner>("GlobalMultiGridRefiner", grp.c_str())
			.add_constructor()
			.add_method("assign_grid", (void (GlobalMultiGridRefiner::*)(MultiGrid&)) &GlobalMultiGridRefiner::assign_grid);
	
	//	parallel refinement
	#ifdef UG_PARALLEL
		reg.add_class_<ParallelHangingNodeRefiner_MultiGrid, HangingNodeRefiner_MultiGrid>
			("ParallelHangingNodeRefiner_MultiGrid", grp.c_str())
			.add_constructor();
	#endif
	
	//	GridObject
		reg.add_class_<GridObject, Grid>("GridObject", grp.c_str())
			.add_constructor()
			.add_method("get_grid", &GridObject::get_grid)
			.add_method("get_subset_handler", &GridObject::get_subset_handler);

	//	Grid functions
		reg.add_function("CreateFractal", &CreateFractal, grp.c_str());

	//  GridObject functions
		reg.add_function("LoadGrid", (bool (*)(Grid&, ISubsetHandler&, const char*))&LoadGrid, grp.c_str())
			.add_function("LoadGrid", (bool (*)(Grid&, const char*))&LoadGrid, grp.c_str())
			.add_function("SaveGrid", (bool (*)(Grid&, const SubsetHandler&, const char*))&SaveGrid, grp.c_str())
			.add_function("SaveGrid", (bool (*)(Grid&, SubsetHandler&, const char*))&SaveGrid, grp.c_str())
			.add_function("SaveGrid", (bool (*)(Grid&, const char*))&SaveGrid, grp.c_str())
			.add_function("LoadGridObject", &LoadGridObject, grp.c_str())
			.add_function("SaveGridObject", &SaveGridObject, grp.c_str())
			.add_function("SaveGridHierarchyTransformed", &SaveGridHierarchyTransformed, grp.c_str())
			.add_function("CreateGridObject", &CreateGridObject, grp.c_str())
			.add_function("PrintGridElementNumbers", (void (*)(MultiGrid&))&PrintGridElementNumbers, grp.c_str())
			.add_function("PrintGridElementNumbers", (void (*)(Grid&))&PrintGridElementNumbers, grp.c_str());

	//	refinement
		reg.add_function("TestSubdivision", &TestSubdivision)
			.add_function("TestHangingNodeRefiner_MultiGrid", &TestHangingNodeRefiner_MultiGrid)
			.add_function("CreateSmoothHierarchy", &CreateSmoothHierarchy, grp.c_str())
			.add_function("CreateSemiSmoothHierarchy", &CreateSemiSmoothHierarchy, grp.c_str())
			.add_function("SaveGridHierarchy", &SaveGridHierarchy, grp.c_str())
			.add_function("MarkForRefinement_VerticesInSphere", (void (*)(IRefiner&, number, number, number, number))
																&MarkForRefinement_VerticesInSphere, grp.c_str())
			.add_function("MarkForRefinement_FacesInSphere", (void (*)(IRefiner&, number, number, number, number))
																&MarkForRefinement_FacesInSphere, grp.c_str())
			.add_function("MarkForRefinement_VerticesInSquare", (void (*)(IRefiner&, number, number, number, number, number, number))
																&MarkForRefinement_VerticesInSquare, grp.c_str())
			.add_function("MarkForRefinement_All", &MarkForRefinement_All, grp.c_str())
			.add_function("TestGridRedistribution", &TestGridRedistribution);
		
	//	subset util
		reg.add_function("AdjustSubsetsForSimulation",
						(void (*)(SubsetHandler&, bool, bool, bool))
						&AdjustSubsetsForSimulation<SubsetHandler>)
			.add_function("AdjustSubsetsForSimulation",
						(void (*)(MGSubsetHandler&, bool, bool, bool))
						&AdjustSubsetsForSimulation<MGSubsetHandler>);

	//	tests:
		reg.add_function("TestAttachedLinkedList", &TestAttachedLinkedList);
	}
	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterLibGridInterface: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}

}//	end of namespace 
}//	end of namespace 
