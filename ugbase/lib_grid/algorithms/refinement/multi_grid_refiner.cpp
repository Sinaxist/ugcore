//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m11 d09

#include <cassert>
#include "multi_grid_refiner.h"
#include "lib_grid/algorithms/algorithms.h"

using namespace std;

namespace ug
{

MultiGridRefiner::MultiGridRefiner()
{
	m_pMG = NULL;
	m_copyRange = 2;
}

MultiGridRefiner::MultiGridRefiner(MultiGrid& mg)
{
	m_pMG = NULL;
	assign_grid(mg);
	m_copyRange = 2;
}

MultiGridRefiner::~MultiGridRefiner()
{
	if(m_pMG)
		m_pMG->unregister_observer(this);
}

void MultiGridRefiner::assign_grid(MultiGrid& mg)
{
	if(m_pMG)
	{
		m_pMG->unregister_observer(this);
		m_pMG = NULL;
	}

	mg.register_observer(this, OT_GRID_OBSERVER);
}

void MultiGridRefiner::registered_at_grid(Grid* grid)
{
	if(m_pMG)
		m_pMG->unregister_observer(this);

	m_pMG = dynamic_cast<MultiGrid*>(grid);
	assert(m_pMG && "MultiGridRefiner registered at incopatible grid.");

	if(!m_pMG)
		return;

	MultiGrid& mg = *m_pMG;

//	selection-marks
	m_selMarks.assign_grid(mg);
	m_selMarks.enable_autoselection(false);
	m_selMarks.enable_selection_inheritance(false);

//	status- and refinement-marks
	mg.attach_to_vertices_dv(m_aInt, 0);
	mg.attach_to_edges_dv(m_aInt, 0);
	mg.attach_to_faces_dv(m_aInt, 0);
	mg.attach_to_volumes_dv(m_aInt, 0);

	m_aaIntVRT.access(mg, m_aInt);
	m_aaIntEDGE.access(mg, m_aInt);
	m_aaIntFACE.access(mg, m_aInt);
	m_aaIntVOL.access(mg, m_aInt);
}

void MultiGridRefiner::unregistered_from_grid(Grid* grid)
{
	if(m_pMG)
	{
		m_pMG->detach_from_vertices(m_aInt);
		m_pMG->detach_from_edges(m_aInt);
		m_pMG->detach_from_faces(m_aInt);
		m_pMG->detach_from_volumes(m_aInt);

		m_pMG->unregister_observer(&m_selMarks);
		m_pMG = NULL;
	}
}

void MultiGridRefiner::clear_marks()
{
	m_selMarks.clear();
}

////////////////////////////////////////////////////////////////////////
void MultiGridRefiner::refine()
{
	assert(m_pMG && "refiner not has to be assigned to a multi-grid!");
	if(!m_pMG)
		return;

//	the multi-grid
	MultiGrid& mg = *m_pMG;

//	make sure that the required options are enabled.
	if(!mg.option_is_enabled(GRIDOPT_FULL_INTERCONNECTION))
	{
		LOG("WARNING in MultiGridRefiner::refine(): auto-enabling GRIDOPT_FULL_INTERCONNECTION.\n");
		mg.enable_options(GRIDOPT_FULL_INTERCONNECTION);
	}

//	access position attachments
	Grid::VertexAttachmentAccessor<APosition> aaPos;
	if(mg.has_vertex_attachment(aPosition))
		aaPos.access(mg, aPosition);

//	collect objects for refine
	collect_objects_for_refine();

//	cout << "num marked edges: " << m_selMarks.num<EdgeBase>() << endl;
//	cout << "num marked faces: " << m_selMarks.num<Face>() << endl;

//	we want to add new elements in a new layer.
	bool bHierarchicalInsertionWasEnabled = mg.hierarchical_insertion_enabled();
	if(!bHierarchicalInsertionWasEnabled)
		mg.enable_hierarchical_insertion(true);


//	some buffers
	vector<VertexBase*> vVrts;
	vector<VertexBase*> vEdgeVrts;
	vector<EdgeBase*>	vEdges;
	vector<Face*>		vFaces;

//	some repeatedly used objects
	EdgeDescriptor ed;
	FaceDescriptor fd;
	VolumeDescriptor vd;

//LOG("creating new vertices\n");
//	create new vertices from marked vertices
	for(VertexBaseIterator iter = m_selMarks.begin<VertexBase>();
		iter != m_selMarks.end<VertexBase>(); ++iter)
	{
		VertexBase* v = *iter;
		if(!mg.get_child_vertex(v))
		{
		//	create a new vertex in the next layer.
			VertexBase* nVrt = *mg.create_by_cloning(v, v);

			if(aaPos.valid())
			{
				aaPos[nVrt] = aaPos[v];
			//	change z-coord to visualise the hierarchy
				aaPos[nVrt].z += 0.01;
			}
		}
	}

//LOG("creating new edges\n");
//	create new vertices and edges from marked edges
	int numNewEdges = 0;

	for(EdgeBaseIterator iter = m_selMarks.begin<EdgeBase>();
		iter != m_selMarks.end<EdgeBase>(); ++iter)
	{
	//	collect_objects_for_refine removed all edges that already were
	//	refined. No need to check that again.
		EdgeBase* e = *iter;
		int rule = get_rule(e);
		switch(rule)
		{
		case RM_COPY:
			{
			//	clone the edge.
				ed.set_vertices(mg.get_child_vertex(e->vertex(0)),
								mg.get_child_vertex(e->vertex(1)));
				EdgeBase* newEdge = *mg.create_by_cloning(e, ed, e);
				set_status(newEdge, SM_COPY);
			}break;

		default:
			{
			//	create two new edges by edge-split
				Vertex* nVrt = *mg.create<Vertex>(e);
				VertexBase* substituteVrts[2];
				substituteVrts[0] = mg.get_child_vertex(e->vertex(0));
				substituteVrts[1] = mg.get_child_vertex(e->vertex(1));

				if(aaPos.valid())
				{
					VecScaleAdd(aaPos[nVrt], 0.5, aaPos[e->vertex(0)],
											0.5, aaPos[e->vertex(1)]);
				//	change z-coord to visualise the hierarchy
					aaPos[nVrt].z += 0.01;
				}

			//	split the edge
				e->refine(vEdges, nVrt, substituteVrts);
				assert((vEdges.size() == 2) && "Edge refine produced wrong number of edges.");
				mg.register_element(vEdges[0], e);
				mg.register_element(vEdges[1], e);
				set_status(vEdges[0], SM_REGULAR);
				set_status(vEdges[1], SM_REGULAR);
			}break;
		}
	}

//LOG("creating new faces\n");
//	create new vertices and faces from marked faces
	for(FaceIterator iter = m_selMarks.begin<Face>();
		iter != m_selMarks.end<Face>(); ++iter)
	{
		Face* f = *iter;

		int rule = get_rule(f);
		switch(rule)
		{
		case RM_COPY:
			{
			//	clone the face.
				if(fd.num_vertices() != f->num_vertices())
					fd.set_num_vertices(f->num_vertices());

				for(size_t i = 0; i < fd.num_vertices(); ++i)
					fd.set_vertex(i, mg.get_child_vertex(f->vertex(i)));

				Face* nf = *mg.create_by_cloning(f, fd, f);
				set_status(nf, SM_COPY);
			}break;

		default:
			{
			//	collect child-vertices
				vVrts.clear();
				for(uint j = 0; j < f->num_vertices(); ++j){
					vVrts.push_back(mg.get_child_vertex(f->vertex(j)));
				}

			//	collect the associated edges
				vEdgeVrts.clear();
				//bool bIrregular = false;
				for(uint j = 0; j < f->num_edges(); ++j){
					VertexBase* vrt = mg.get_child_vertex(mg.get_edge(f, j));
					vEdgeVrts.push_back(vrt);
					//if(!vrt)
					//	bIrregular = true;
				}
/*
				if(bIrregular){
					assert((get_rule(f) != RM_REGULAR) && "Bad refinement-rule set during collect_objects_for_refine!");
		//TODO:	care about anisotropy
					set_rule(f, RM_IRREGULAR);
				}
*/
				VertexBase* newVrt;
				if(f->refine(vFaces, &newVrt, &vEdgeVrts.front(), NULL, &vVrts.front())){
				//	if a new vertex was generated, we have to register it
					if(newVrt){
						mg.register_element(newVrt, f);
						if(aaPos.valid()){
							aaPos[newVrt] = CalculateCenter(f, aaPos);
						//	change z-coord to visualise the hierarchy
							aaPos[newVrt].z += 0.01;
						}
					}

					int oldRule = get_rule(f);

				//	register the new faces and assign status
					for(size_t j = 0; j < vFaces.size(); ++j){
						mg.register_element(vFaces[j], f);
						switch(oldRule)
						{
						case RM_REGULAR:	set_status(vFaces[j], SM_REGULAR); break;
						case RM_IRREGULAR:	set_status(vFaces[j], SM_IRREGULAR); break;
						default:
							assert((oldRule == RM_REGULAR) && "rule not yet handled.");//always fails.
							break;
						}
					}
				}
				else{
					LOG("  WARINING in Refine: could not refine face.\n");
				}
			}
		}
	}

//	done - clean up
	if(!bHierarchicalInsertionWasEnabled)
		mg.enable_hierarchical_insertion(false);

	m_selMarks.clear();
}

void MultiGridRefiner::collect_objects_for_refine()
{
//	make sure, that elements that already have children won't be
//	refined again.
//	all elements that are associated with marked elements and
//	which are of lower dimension have to be marked too.

//	algorithm layout:
/*
//initial selection
	deselect volumes that must not be refined
	select faces that are associated with volumes
	deselect faces that must not be refined
	select edges that are associated with faces
	deselect edges that must not be refined
	select vertices that are associated with edges

//build closure
	select faces that are associated with edges
		iterate over associated edges
			if all are marked RM_REGULAR -> mark face RM_REGULAR
			else-> mark face RM_IRREGULAR, select and mark unselected edges as COPY-edge
		iterate over associated vertices
			push unselected to vVrts
			select them

	select volumes that are associated with faces
		iterate over associated faces
			if all are marked RM_REGULAR -> mark volume RM_REGULAR
			else-> mark vol RM_IRREGULAR, select and mark unselected faces as COPY-face
		iterate over associated edges
			select and mark unselected edges as COPY-edge
		iterate over associated vertices
			push unselected to vVrts
			select them

//select copy-elements
	 iterate over vVrts (make sure that newly added vrts are not handled in this iteration)
	 	iterate over associated edges
			if unselected
				select and mark  as COPY-edge
				select associated vertices and add them to vVrts
		iterate over associated faces
			if unselected
				select and mark  as COPY-face
				select associated vertices and add them to vVrts
				iterate over associated edges and mark unselected ones as COPY-edge
		iterate over associated volumes
			if unselected
				 select and mark as COPY-volume
				 select associated vertices and add them to vVrts
				 iterate over associated faces and mark unselected ones as COPY-face
				 iterate over associated edges and mark unselected ones as COPY-edge
*/

//	Some buffers and repeatedly used objects
	vector<VertexBase*> vVrts;//	vertices stored in here are used during copy-element-selection.
	vector<EdgeBase*> 	vEdges;//	vector used for temporary results
	vector<Face*> 		vFaces;//	vector used for temporary results
	vector<Volume*>		vVolumes;//	vector used for temporary results

//	regard the multi-grid as a grid
	Grid& grid =*static_cast<Grid*>(m_pMG);
	MultiGrid& mg = *m_pMG;
	
////////////////////////////////
//	initial selection
//	select elements that are associated with volumes
//	make sure to deselect all elements that must not be refined.
	for(VolumeIterator iter = m_selMarks.begin<Volume>();
		iter != m_selMarks.end<Volume>();)
	{
		Volume* v = *iter;
		++iter;//iterator is incremented since it would be invalidated during deselect.
		if(mg.has_children(v))
		{
		//	the element has already been refined.
		//	don't refine it again.
			m_selMarks.deselect(v);
		}
		else if(get_status(v) == SM_COPY){
		//	copy elements must not be refined in the current version
			LOG("  WARNING: copy-elements must not be refined in the current version!\n");
			m_selMarks.deselect(v);
		}
		else if(get_status(v) == SM_IRREGULAR){
		//	irregular elements must not be refined in the current version
			LOG("  WARNING: irregular-elements must not be refined in the current version!\n");
			m_selMarks.deselect(v);
		}
		else{
		//	mark it for regular refinement
			set_rule(v, RM_REGULAR);
		//	collect associated faces
			CollectFaces(vFaces, grid, v);
			for(uint i = 0; i < vFaces.size(); ++i)
				if(!mg.has_children(vFaces[i]))
					m_selMarks.select(vFaces[i]);
		}
	}

//	select elements that are associated with faces
	for(FaceIterator iter = m_selMarks.begin<Face>();
		iter != m_selMarks.end<Face>();)
	{
		Face* f = *iter;
		++iter;	//iterator is incremented since it would be invalidated during deselect.
		if(mg.has_children(f))
		{
		//	the element has already been refined.
		//	don't refine it again.
			m_selMarks.deselect(f);
		}
		else if(get_status(f) == SM_COPY){
		//	copy elements must not be refined in the current version
			LOG("  WARNING: copy-elements must not be refined in the current version!\n");
			m_selMarks.deselect(f);
		}
		else if(get_status(f) == SM_IRREGULAR){
		//	irregular elements must not be refined in the current version
			LOG("  WARNING: irregular-elements must not be refined in the current version!\n");
			m_selMarks.deselect(f);
		}
		else{
		//	mark it for regular refinement
			set_rule(f, RM_REGULAR);
		//	collect associated edges
			CollectEdges(vEdges, grid, f);
			for(uint i = 0; i < vEdges.size(); ++i)
				if(!mg.has_children(vEdges[i]))
					m_selMarks.select(vEdges[i]);
		}
	}

//	select elements that are associated with edges
	for(EdgeBaseIterator iter = m_selMarks.begin<EdgeBase>();
		iter != m_selMarks.end<EdgeBase>();)
	{
		EdgeBase* e = *iter;
		++iter;	//iterator is incremented since it would be invalidated during deselect.
		if(mg.has_children(e)){
		//	the element has already been refined.
		//	don't refine it again.
			m_selMarks.deselect(e);
		}
		else if(get_status(e) == SM_COPY){
		//	copy elements must not be refined in the current version
			LOG("  WARNING: copy-elements must not be refined in the current version!\n");
			m_selMarks.deselect(e);
		}
		else if(get_status(e) == SM_IRREGULAR){
		//	irregular elements must not be refined in the current version
			LOG("  WARNING: irregular-elements must not be refined in the current version!\n");
			m_selMarks.deselect(e);
		}
		else{
		//	mark it for regular refinement
			set_rule(e, RM_REGULAR);
		//	select associated vertices
			for(uint i = 0; i < e->num_vertices(); ++i)
				m_selMarks.select(e->vertex(i));
		}
	}
	
////////////////////////////////
//	closure selection
//	collect associated faces of refine-edges that will be used for the closure.
//	associated volumes will be collected implicitly later on from refine-faces.

	if(mg.num<Face>() > 0)
	{
	//	store end-iterator so that newly marked edges won't be considered.
		for(EdgeBaseIterator iter = m_selMarks.begin<EdgeBase>();
			iter != m_selMarks.end<EdgeBase>(); ++iter)
		{
		//	as soon as we encounter an edge that is scheduled for COPY, we're done in here
			if(get_rule(*iter) == RM_COPY)
				break;
			
			CollectFaces(vFaces, grid, *iter);
			for(size_t i = 0; i < vFaces.size(); ++i){
				Face* f = vFaces[i];
				if(!m_selMarks.is_selected(f) &! mg.has_children(f))
				{
					m_selMarks.select(f);
					
				//	we now have to check all associated edges.
				//	unselected ones will be added as copy-elements
					size_t numRegular = 0;
					CollectEdges(vEdges, grid, f);
					for(size_t j = 0; j < vEdges.size(); ++j){
						EdgeBase* e = vEdges[j];
						if(m_selMarks.is_selected(e)){
							if(get_rule(e) == RM_REGULAR)
								++numRegular;
						}
						else{
							if(!mg.has_children(e)){
								set_rule(e, RM_COPY);
								m_selMarks.select(e);
							}
						}
					}
					
				//	set rule
				//	if all associated edges are refined regular,
				//	we'll refine the face regular, too.
					if(numRegular == vEdges.size())
						set_rule(f, RM_REGULAR);
					else
						set_rule(f, RM_IRREGULAR);
						
				//	finally we have to make sure that all vertices are selected.
					for(size_t j = 0; j < f->num_vertices(); ++j)
					{
						if(!m_selMarks.is_selected(f->vertex(j))){
							if(get_copy_range() > 0)
								vVrts.push_back(f->vertex(j));
							m_selMarks.select(f->vertex(j));
						}
					}
				}
			}
		}
	}

//	collect associated volumes of refine-faces that will be used for the closure.
//	we don't have to check associated volumes of refine-edges, since
//	those are associated volumes of refine-faces, too.
	if(mg.num<Volume>() > 0)
	{
	//	store end-iterator so that newly marked faces won't be considered.
		for(FaceIterator iter = m_selMarks.begin<Face>();
			iter != m_selMarks.end<Face>(); ++iter)
		{
		//	as soon as we encounter a face that is scheduled for COPY, we're done in here
			if(get_rule(*iter) == RM_COPY)
				break;

			CollectVolumes(vVolumes, grid, *iter);
			for(size_t i = 0; i < vVolumes.size(); ++i){
				Volume* v = vVolumes[i];
				if(!m_selMarks.is_selected(v) &! mg.has_children(v))
				{
					m_selMarks.select(v);
					
				//	we now have to check all associated faces.
				//	unselected ones will be added as copy-elements.
					size_t numRegular = 0;
					CollectFaces(vFaces, grid, v);
					for(size_t j = 0; j < vFaces.size(); ++j){
						Face* f = vFaces[j];
						if(m_selMarks.is_selected(f)){
							if(get_rule(f) == RM_REGULAR)
								++numRegular;
						}
						else{
							if(!mg.has_children(f)){
								set_rule(f, RM_COPY);
								m_selMarks.select(f);
							}
						}
					}
					
				//	set rule
				//	if all faces are refined regular, we'll refine the volume regular, too.
					if(numRegular == vFaces.size())
						set_rule(v, RM_REGULAR);
					else
						set_rule(v, RM_IRREGULAR);
					
				//	we now have to check all associated edges.
				//	unselected ones will be added as copy-elements.
					CollectEdges(vEdges, grid, v);
					for(size_t j = 0; j < vEdges.size(); ++j){
						EdgeBase* e = vEdges[j];
						if(!m_selMarks.is_selected(e)
							&! mg.has_children(e))
						{
							if(!mg.has_children(e)){
								set_rule(e, RM_COPY);
								m_selMarks.select(e);
							}
						}
					}
					
				//	finally we have to make sure that all vertices are selected.
					for(size_t j = 0; j < v->num_vertices(); ++j)
					{
						if(!m_selMarks.is_selected(v->vertex(j))){
							if(get_copy_range() > 0)
								vVrts.push_back(v->vertex(j));
							m_selMarks.select(v->vertex(j));
						}
					}
				}
			}
		}
	}

////////////////////////////////
//	collect copy-elements
//	we'll collect unselected edges, faces and volumes that are in
//	a neighbourhood to the selection
	if(get_copy_range() > 0){		
	//	we have to make sure that we'll only collect copy-elements in
	//	the correct neighbourhood.
	//	After we processed all elements between iFirst and iEnd, the
	//	first neighbourhood is done. We may then set iFirst to iEnd and
	//	iEnd to vVrts.size(), to process the next neighbourhood.
		size_t iFirst = 0;
		size_t iEnd = vVrts.size();
	//	iterate for each neighbourhood
		for(size_t iNbr = 0; iNbr < get_copy_range(); ++iNbr)
		{
		//	iterate over candidates
			for(size_t i = iFirst; i != iEnd; ++i){
				VertexBase* vrt = vVrts[i];
			//	check all associated elements.
			//	If we can find an unselected, it is a copy-element.
			//	we then have to add all its associated unselected vertices
			//	to vVrts.
			
			//	check edges
				{
					EdgeBaseIterator iterEnd = grid.associated_edges_end(vrt);
					for(EdgeBaseIterator iter = grid.associated_edges_begin(vrt);
						iter != iterEnd; ++iter)
					{
						EdgeBase* e = *iter;
						if(!m_selMarks.is_selected(e) &! mg.has_children(e)){
						//	we found a copy-element
							m_selMarks.select(e);
							set_rule(e, RM_COPY);
							
						//	schedule associated unselected vertices for further checks
							for(size_t j = 0; j < 2; ++j){
								if(!m_selMarks.is_selected(e->vertex(j))){
									m_selMarks.select(e->vertex(j));
									vVrts.push_back(e->vertex(j));
								}
							}
						}
					}
				}

			//	check faces
				{
					FaceIterator iterEnd = grid.associated_faces_end(vrt);
					for(FaceIterator iter = grid.associated_faces_begin(vrt);
						iter != iterEnd; ++iter)
					{
						Face* f = *iter;
						if(!m_selMarks.is_selected(f) &! mg.has_children(f)){
						//	we found a copy-element
							m_selMarks.select(f);
							set_rule(f, RM_COPY);
							
						//	schedule associated unselected vertices for further checks
							for(size_t j = 0; j < f->num_vertices(); ++j){
								if(!m_selMarks.is_selected(f->vertex(j))){
									m_selMarks.select(f->vertex(j));
									vVrts.push_back(f->vertex(j));
								}
							}
							
						//	mark associated unselected edges as copy-edge
							CollectEdges(vEdges, grid, f);
							for(size_t j = 0; j < vEdges.size(); ++j){
								EdgeBase* e = vEdges[j];
								if(!m_selMarks.is_selected(e) &! mg.has_children(e)){
									m_selMarks.select(e);
									set_rule(e, RM_COPY);
								}
							}
						}
					}
				}

			//	check volumes
				{
					VolumeIterator iterEnd = grid.associated_volumes_end(vrt);
					for(VolumeIterator iter = grid.associated_volumes_begin(vrt);
						iter != iterEnd; ++iter)
					{
						Volume* v = *iter;
						if(!m_selMarks.is_selected(v) &! mg.has_children(v)){
						//	we found a copy-element
							m_selMarks.select(v);
							set_rule(v, RM_COPY);
							
						//	schedule associated unselected vertices for further checks
							for(size_t j = 0; j < v->num_vertices(); ++j){
								if(!m_selMarks.is_selected(v->vertex(j))){
									m_selMarks.select(v->vertex(j));
									vVrts.push_back(v->vertex(j));
								}
							}
							
						//	mark associated unselected edges as copy-edge
							CollectEdges(vEdges, grid, v);
							for(size_t j = 0; j < vEdges.size(); ++j){
								EdgeBase* e = vEdges[j];
								if(!m_selMarks.is_selected(e) &! mg.has_children(e)){
									m_selMarks.select(e);
									set_rule(e, RM_COPY);
								}
							}
							
						//	mark associated unselected faces as copy-edge
							CollectFaces(vFaces, grid, v);
							for(size_t j = 0; j < vFaces.size(); ++j){
								Face* f = vFaces[j];
								if(!m_selMarks.is_selected(f) &! mg.has_children(f)){
									m_selMarks.select(f);
									set_rule(f, RM_COPY);
								}
							}
						}
					}
				}
			}

		//	the neighbourhood is done. check the next neighbourhood.
			iFirst = iEnd;
			iEnd = vVrts.size();
		}
	}
}

}//	end of namespace
