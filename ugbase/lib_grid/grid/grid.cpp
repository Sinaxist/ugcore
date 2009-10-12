//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y08 m10 d10

#include <cassert>
#include "grid.h"
#include "common/common.h"

using namespace std;

namespace ug
{
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//	implementation of Grid

////////////////////////////////////////////////////////////////////////
//	constructor
Grid::Grid() : m_aVertexContainer(false), m_aEdgeContainer(false),
				m_aFaceContainer(false), m_aVolumeContainer(false)
{
	m_options = GRIDOPT_NONE;
	//change_options(GRIDOPT_DEFAULT);
}

Grid::Grid(const Grid& grid) : m_aVertexContainer(false), m_aEdgeContainer(false),
							m_aFaceContainer(false), m_aVolumeContainer(false)
{
	assert(!"WARNING in Grid::Grid(const Grid& grid): Copy-Constructor not yet implemented!");
	LOG("WARNING in Grid::Grid(const Grid& grid): Copy-Constructor not yet implemented! Expect unexpected behavior." << endl);
}

Grid::~Grid()
{
//	unregister all observers
	while(!m_gridObservers.empty())
	{
		unregister_observer(m_gridObservers.back());
	}
}

VertexBaseIterator Grid::create_by_cloning(VertexBase* pCloneMe, GeometricObject* pParent)
{
	VertexBase* pNew = reinterpret_cast<VertexBase*>(pCloneMe->create_empty_instance());
	register_vertex(pNew, pParent);
	return iterator_cast<VertexBaseIterator>(pNew->m_entryIter);
}

EdgeBaseIterator Grid::create_by_cloning(EdgeBase* pCloneMe, const EdgeDescriptor& ed, GeometricObject* pParent)
{
	EdgeBase* pNew = reinterpret_cast<EdgeBase*>(pCloneMe->create_empty_instance());
	pNew->set_vertex(0, ed.vertex(0));
	pNew->set_vertex(1, ed.vertex(1));
	register_edge(pNew, pParent);
	return iterator_cast<EdgeBaseIterator>(pNew->m_entryIter);
}

FaceIterator Grid::create_by_cloning(Face* pCloneMe, const FaceDescriptor& fd, GeometricObject* pParent)
{
	Face* pNew = reinterpret_cast<Face*>(pCloneMe->create_empty_instance());
	uint numVrts = fd.num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		pNew->set_vertex(i, fd.vertex(i));
	register_face(pNew, pParent);
	return iterator_cast<FaceIterator>(pNew->m_entryIter);
}

VolumeIterator Grid::create_by_cloning(Volume* pCloneMe, const VolumeDescriptor& vd, GeometricObject* pParent)
{
	Volume* pNew = reinterpret_cast<Volume*>(pCloneMe->create_empty_instance());
	uint numVrts = vd.num_vertices();
	for(uint i = 0; i < numVrts; ++i)
		pNew->set_vertex(i, vd.vertex(i));
	register_volume(pNew, pParent);
	return iterator_cast<VolumeIterator>(pNew->m_entryIter);
}

////////////////////////////////////////////////////////////////////////
//	erase functions
void Grid::erase(GeometricObject* geomObj)
{
	assert(geomObj->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Vertex*). Invalid pipe section!");

	uint objType = geomObj->base_object_type_id();
	switch(objType)
	{
		case VERTEX:
			erase(dynamic_cast<VertexBase*>(geomObj));
			break;
		case EDGE:
			erase(dynamic_cast<EdgeBase*>(geomObj));
			break;
		case FACE:
			erase(dynamic_cast<Face*>(geomObj));
			break;
		case VOLUME:
			erase(dynamic_cast<Volume*>(geomObj));
			break;
	};
}

void Grid::erase(VertexBase* vrt)
{
	assert((vrt != NULL) && "ERROR in Grid::erase(Vertex*): invalid pointer)");
	assert(vrt->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Vertex*). Invalid pipe section!");

	unregister_vertex(vrt);

	delete vrt;
}

void Grid::erase(EdgeBase* edge)
{
	assert((edge != NULL) && "ERROR in Grid::erase(Edge*): invalid pointer)");
	assert(edge->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Edge*). Invalid pipe section!");

	unregister_edge(edge);

	delete edge;
}

void Grid::erase(Face* face)
{
	assert((face != NULL) && "ERROR in Grid::erase(Face*): invalid pointer)");
	assert(face->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Face*). Invalid pipe section!");

	unregister_face(face);

	delete face;
}

void Grid::erase(Volume* vol)
{
	assert((vol != NULL) && "ERROR in Grid::erase(Volume*): invalid pointer)");
	assert(vol->shared_pipe_section() != -1
			&& "ERROR in Grid::erase(Volume*). Invalid pipe section!");

	unregister_volume(vol);

	delete vol;
}

//	the geometric-object-collection:
GeometricObjectCollection Grid::get_geometric_object_collection()
{
	return GeometricObjectCollection(&m_elementStorage[VERTEX].m_sectionContainer,
									 &m_elementStorage[EDGE].m_sectionContainer,
									 &m_elementStorage[FACE].m_sectionContainer,
									 &m_elementStorage[VOLUME].m_sectionContainer);
}

void Grid::flip_orientation(Face* f)
{
//	inverts the order of vertices.
	uint numVrts = (int)f->num_vertices();
	vector<VertexBase*> vVrts(numVrts);
	
	uint i;
	for(i = 0; i < numVrts; ++i)
		vVrts[i] = f->m_vertices[i];
		
	for(i = 0; i < numVrts; ++i)
		f->m_vertices[i] = vVrts[numVrts - 1 - i];
}

uint Grid::vertex_fragmentation()
{
	return m_elementStorage[VERTEX].m_attachmentPipe.num_data_entries() - m_elementStorage[VERTEX].m_attachmentPipe.num_elements();
}

uint Grid::edge_fragmentation()
{
	return m_elementStorage[EDGE].m_attachmentPipe.num_data_entries() - m_elementStorage[EDGE].m_attachmentPipe.num_elements();
}

uint Grid::face_fragmentation()
{
	return m_elementStorage[FACE].m_attachmentPipe.num_data_entries() - m_elementStorage[FACE].m_attachmentPipe.num_elements();
}

uint Grid::volume_fragmentation()
{
	return m_elementStorage[VOLUME].m_attachmentPipe.num_data_entries() - m_elementStorage[VOLUME].m_attachmentPipe.num_elements();
}

////////////////////////////////////////////////////////////////////////
//	pass_on_values
void Grid::pass_on_values(Grid::AttachmentPipe& attachmentPipe,
							GeometricObject* pSrc, GeometricObject* pDest)
{
	for(AttachmentPipe::ConstAttachmentEntryIterator iter = attachmentPipe.attachments_begin();
		iter != attachmentPipe.attachments_end(); iter++)
	{
		if((*iter).m_userData == 1)
			(*iter).m_pContainer->copy_data(get_attachment_data_index(pSrc),
											get_attachment_data_index(pDest));
	}
}

void Grid::pass_on_values(VertexBase* objSrc, VertexBase* objDest)
{
	pass_on_values(m_elementStorage[VERTEX].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(EdgeBase* objSrc, EdgeBase* objDest)
{
	pass_on_values(m_elementStorage[EDGE].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(Face* objSrc, Face* objDest)
{
	pass_on_values(m_elementStorage[FACE].m_attachmentPipe, objSrc, objDest);
}

void Grid::pass_on_values(Volume* objSrc, Volume* objDest)
{
	pass_on_values(m_elementStorage[VOLUME].m_attachmentPipe, objSrc, objDest);
}

////////////////////////////////////////////////////////////////////////
//	options
void Grid::set_options(uint options)
{
	change_options(options);
}

uint Grid::get_options()
{
	return m_options;
}

void Grid::enable_options(uint options)
{
	change_options(m_options | options);
}

void Grid::disable_options(uint options)
{
	change_options(m_options & (!m_options));
}

bool Grid::option_is_enabled(uint option)
{
	return (m_options & option) == option;
}

void Grid::change_options(uint optsNew)
{
	change_vertex_options(optsNew &	0x000000FF);
	change_edge_options(optsNew & 	0x0000FF00);
	change_face_options(optsNew & 	0x00FF0000);
	change_volume_options(optsNew &	0xFF000000);
}
/*
void Grid::register_observer(GridObserver* observer, uint observerType)
{
//	check which elements have to be observed and store pointers to the observers.
//	avoid double-registration!
	ObserverContainer* observerContainers[] = {&m_gridObservers, &m_vertexObservers,
												&m_edgeObservers, & m_faceObservers, &m_volumeObservers};

	uint observerTypes[] = {OT_GRID_OBSERVER, OT_VERTEX_OBSERVER, OT_EDGE_OBSERVER, OT_FACE_OBSERVER, OT_VOLUME_OBSERVER};
	for(int i = 0; i < 5; ++i)
	{
		if((observerType & observerTypes[i]) == observerTypes[i])
		{
			ObserverContainer::iterator iter = find(observerContainers[i]->begin(), observerContainers[i]->end(), observer);
			if(iter == observerContainers[i]->end())
				observerContainers[i]->push_back(observer);
		}
	}

//	if the observer is a grid observer, notify him about the registration
	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
		observer->registered_at_grid(this);
}

void Grid::unregister_observer(GridObserver* observer)
{
//	check where the observer has been registered and erase the corresponding entries.
	ObserverContainer* observerContainers[] = {&m_gridObservers, &m_vertexObservers,
												&m_edgeObservers, & m_faceObservers, &m_volumeObservers};

	bool unregisterdFromGridObservers = false;
	for(int i = 0; i < 5; ++i)
	{
		ObserverContainer::iterator iter = find(observerContainers[i]->begin(), observerContainers[i]->end(), observer);
		if(iter != observerContainers[i]->end())
		{
			if(i == 0)
				unregisterdFromGridObservers = true;
			observerContainers[i]->erase(iter);
		}
	}

//	if the observer is a grid observer, notify him about the unregistration
	if(unregisterdFromGridObservers)
		observer->unregistered_from_grid(this);
}
*/
void Grid::register_observer(GridObserver* observer, uint observerType)
{
//	check which elements have to be observed and store pointers to the observers.
//	avoid double-registration!
	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_gridObservers.begin(),
												m_gridObservers.end(), observer);
		if(iter == m_gridObservers.end())
			m_gridObservers.push_back(observer);
	}

	if((observerType & OT_VERTEX_OBSERVER) == OT_VERTEX_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_vertexObservers.begin(),
												m_vertexObservers.end(), observer);
		if(iter == m_vertexObservers.end())
			m_vertexObservers.push_back(observer);
	}

	if((observerType & OT_EDGE_OBSERVER) == OT_EDGE_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_edgeObservers.begin(),
												m_edgeObservers.end(), observer);
		if(iter == m_edgeObservers.end())
			m_edgeObservers.push_back(observer);
	}

	if((observerType & OT_FACE_OBSERVER) == OT_FACE_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_faceObservers.begin(),
												m_faceObservers.end(), observer);
		if(iter == m_faceObservers.end())
			m_faceObservers.push_back(observer);
	}

	if((observerType & OT_VOLUME_OBSERVER) == OT_VOLUME_OBSERVER)
	{
		ObserverContainer::iterator iter = find(m_volumeObservers.begin(),
												m_volumeObservers.end(), observer);
		if(iter == m_volumeObservers.end())
			m_volumeObservers.push_back(observer);
	}

//	if the observer is a grid observer, notify him about the registration
	if((observerType & OT_GRID_OBSERVER) == OT_GRID_OBSERVER)
		observer->registered_at_grid(this);
}

void Grid::unregister_observer(GridObserver* observer)
{
//	check where the observer has been registered and erase the corresponding entries.
	bool unregisterdFromGridObservers = false;

	{
		ObserverContainer::iterator iter = find(m_gridObservers.begin(),
												m_gridObservers.end(), observer);
		if(iter != m_gridObservers.end())
			m_gridObservers.erase(iter);

		unregisterdFromGridObservers = true;
	}

	{
		ObserverContainer::iterator iter = find(m_vertexObservers.begin(),
												m_vertexObservers.end(), observer);
		if(iter != m_vertexObservers.end())
			m_vertexObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_edgeObservers.begin(),
												m_edgeObservers.end(), observer);
		if(iter != m_edgeObservers.end())
			m_edgeObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_faceObservers.begin(),
												m_faceObservers.end(), observer);
		if(iter != m_faceObservers.end())
			m_faceObservers.erase(iter);
	}

	{
		ObserverContainer::iterator iter = find(m_volumeObservers.begin(),
												m_volumeObservers.end(), observer);
		if(iter != m_volumeObservers.end())
			m_volumeObservers.erase(iter);
	}

//	if the observer is a grid observer, notify him about the unregistration
	if(unregisterdFromGridObservers)
		observer->unregistered_from_grid(this);
}


////////////////////////////////////////////////////////////////////////
//	associated edge access
EdgeBaseIterator Grid::associated_edges_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_EDGES." << endl);
		vertex_store_associated_edges(true);
	}
	return m_aaEdgeContainerVERTEX[vrt].begin();
}

EdgeBaseIterator Grid::associated_edges_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_EDGES." << endl);
		vertex_store_associated_edges(true);
	}
	return m_aaEdgeContainerVERTEX[vrt].end();
}

EdgeBaseIterator Grid::associated_edges_begin(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(face): auto-enabling FACEOPT_STORE_ASSOCIATED_EDGES." << endl);
		face_store_associated_edges(true);
	}
	return m_aaEdgeContainerFACE[face].begin();
}

EdgeBaseIterator Grid::associated_edges_end(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(face): auto-enabling FACEOPT_STORE_ASSOCIATED_EDGES." << endl);
		face_store_associated_edges(true);
	}
	return m_aaEdgeContainerFACE[face].end();
}

EdgeBaseIterator Grid::associated_edges_begin(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_begin(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_EDGES." << endl);
		volume_store_associated_edges(true);
	}
	return m_aaEdgeContainerVOLUME[vol].begin();
}

EdgeBaseIterator Grid::associated_edges_end(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_EDGES))
	{
		LOG("WARNING in associated_edges_end(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_EDGES." << endl);
		volume_store_associated_edges(true);
	}
	return m_aaEdgeContainerVOLUME[vol].end();
}

////////////////////////////////////////////////////////////////////////
//	associated face access
FaceIterator Grid::associated_faces_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_FACES." << endl);
		vertex_store_associated_faces(true);
	}
	return m_aaFaceContainerVERTEX[vrt].begin();
}

FaceIterator Grid::associated_faces_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_FACES." << endl);
		vertex_store_associated_faces(true);
	}
	return m_aaFaceContainerVERTEX[vrt].end();
}

FaceIterator Grid::associated_faces_begin(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_FACES." << endl);
		edge_store_associated_faces(true);
	}
	return m_aaFaceContainerEDGE[edge].begin();
}

FaceIterator Grid::associated_faces_end(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_FACES." << endl);
		edge_store_associated_faces(true);
	}
	return m_aaFaceContainerEDGE[edge].end();
}

FaceIterator Grid::associated_faces_begin(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_begin(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_FACES." << endl);
		volume_store_associated_faces(true);
	}
	return m_aaFaceContainerVOLUME[vol].begin();
}

FaceIterator Grid::associated_faces_end(Volume* vol)
{
	if(!option_is_enabled(VOLOPT_STORE_ASSOCIATED_FACES))
	{
		LOG("WARNING in associated_faces_end(vol): auto-enabling VOLOPT_STORE_ASSOCIATED_FACES." << endl);
		volume_store_associated_faces(true);
	}
	return m_aaFaceContainerVOLUME[vol].end();
}

////////////////////////////////////////////////////////////////////////
//	associated volume access
VolumeIterator Grid::associated_volumes_begin(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		vertex_store_associated_volumes(true);
	}
	return m_aaVolumeContainerVERTEX[vrt].begin();
}

VolumeIterator Grid::associated_volumes_end(VertexBase* vrt)
{
	if(!option_is_enabled(VRTOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(vrt): auto-enabling VRTOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		vertex_store_associated_volumes(true);
	}
	return m_aaVolumeContainerVERTEX[vrt].end();
}

VolumeIterator Grid::associated_volumes_begin(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		edge_store_associated_volumes(true);
	}
	return m_aaVolumeContainerEDGE[edge].begin();
}

VolumeIterator Grid::associated_volumes_end(EdgeBase* edge)
{
	if(!option_is_enabled(EDGEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(edge): auto-enabling EDGEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		edge_store_associated_volumes(true);
	}
	return m_aaVolumeContainerEDGE[edge].end();
}

VolumeIterator Grid::associated_volumes_begin(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_begin(face): auto-enabling FACEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		face_store_associated_volumes(true);
	}
	return m_aaVolumeContainerFACE[face].begin();
}

VolumeIterator Grid::associated_volumes_end(Face* face)
{
	if(!option_is_enabled(FACEOPT_STORE_ASSOCIATED_VOLUMES))
	{
		LOG("WARNING in associated_volumes_end(face): auto-enabling FACEOPT_STORE_ASSOCIATED_VOLUMES." << endl);
		face_store_associated_volumes(true);
	}
	return m_aaVolumeContainerFACE[face].end();
}

}//	end of namespace
