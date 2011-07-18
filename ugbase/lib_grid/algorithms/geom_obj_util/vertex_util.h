//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m02 d02

#ifndef __H__LIB_GRID__VERTEX_UTIL__
#define __H__LIB_GRID__VERTEX_UTIL__

#include <vector>
#include "lib_grid/lg_base.h"
#include "common/math/ugmath.h"
#include "lib_grid/algorithms/callbacks/callback_definitions.h"

namespace ug
{
/**
 * \brief contains methods to manipulate vertices
 *
 * \defgroup lib_grid_algorithms_vertex_util vertex util
 * \ingroup lib_grid_algorithms
 * @{
 */


////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given edge
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(EdgeBase* e, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given face
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(Face* f, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given volume
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(Volume* vol, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetConnectedVertex
///	returns the vertex that is connected to v via e.
/**
 * returns NULL if v is not contained in e.
 */
VertexBase* GetConnectedVertex(EdgeBase* e, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetConnectedVertex
///	returns the index of the first vertex that is contained in f and is not contained in e.
VertexBase* GetConnectedVertex(EdgeVertices* e, Face* f);

////////////////////////////////////////////////////////////////////////
//	GetConnectedVertexIndex
///	returns the index of the first vertex that is contained in the specified face and is not contained in the given edge.
int GetConnectedVertexIndex(Face* f, const EdgeDescriptor& ed);

////////////////////////////////////////////////////////////////////////
///	returns the edge in the triangle tri, which does not contain vrt.
/**	Make sure that tri is a triangle!*/
EdgeBase* GetConnectedEdge(Grid& g, VertexBase* vrt, Face* tri);

////////////////////////////////////////////////////////////////////////
//	CollectNeighbours
///	fills an array with all neighbour-vertices of v.
/**
 * v will not be contained in vNeighboursOut.
 * requires grid-option GRIDOPT_STANDARD_INTERCONNECTION.
 * This method is fast if grid-options FACEOPT_AUTOGENERATE_EDGES
 * and VOLOPT_AUTOGENERATE_EDGES are enabled - if there are any
 * faces and volumes.
 * It works without these options, too. The method however will
 * require more time in this case.
 */
void CollectNeighbours(std::vector<VertexBase*>& vNeighborsOut, Grid& grid, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	CollectSurfaceNeighborsSorted
///	Collects neighbour-vertices in either clockwise or counter clockwise order.
/**	Please note: This algorithm uses Grid::mark.
 *
 *	This method will only work if the triangles connected to the given
 *	vertex are homeomorphic to the unit-disc.
 *
 *	Current implementation requires FACEOPT_AUTOGENERATE_EDGES (could be avoided).
 */
bool CollectSurfaceNeighborsSorted(std::vector<VertexBase*>& vNeighborsOut,
								   Grid& grid, VertexBase* v);
									
////////////////////////////////////////////////////////////////////////
//	FindVertexByCoordiante
///	returns the vertex that is the closest to the given coordinate
/**
 * returns NULL if no vertex was found (if iterBegin == iterEnd).
 */
VertexBase* FindVertexByCoordiante(vector3& coord, VertexBaseIterator iterBegin, VertexIterator iterEnd,
									Grid::VertexAttachmentAccessor<APosition>& aaPos);

////////////////////////////////////////////////////////////////////////
///	calculates the normal of a vertex using associated faces
/**
 * TAAPosVRT has to be an attachment accessor for the vector3 type that
 * works on the triangles in grid.
 */
template <class TAAPosVRT>
void CalculateVertexNormal(vector3& nOut, Grid& grid, VertexBase* vrt,
						   TAAPosVRT& aaPos);

////////////////////////////////////////////////////////////////////////
//	CalculateVertexNormals
///	calculates the normals of all vertices in grid and stores them in aNorm.
/**
 * aPos has to be attached to grid.
 * If some attachments were not attached correctly, the method returns false.
 * \{
 */
bool CalculateVertexNormals(Grid& grid, APosition& aPos, ANormal& aNorm);

bool CalculateVertexNormals(Grid& grid,
							Grid::AttachmentAccessor<VertexBase, APosition>& aaPos,
							Grid::AttachmentAccessor<VertexBase, ANormal>& aaNorm);

/** \} */

////////////////////////////////////////////////////////////////////////
//	CalculateBoundingBox
/// calculates the BoundingBox
/**	\{	*/
void CalculateBoundingBox(vector3& vMinOut, vector3& vMaxOut, VertexBaseIterator vrtsBegin,
						  VertexBaseIterator vrtsEnd,
						  Grid::AttachmentAccessor<VertexBase, APosition>& aaPos);

void CalculateBoundingBox(vector2& vMinOut, vector2& vMaxOut, VertexBaseIterator vrtsBegin,
						  VertexBaseIterator vrtsEnd,
						  Grid::AttachmentAccessor<VertexBase, APosition2>& aaPos);

void CalculateBoundingBox(vector1& vMinOut, vector1& vMaxOut, VertexBaseIterator vrtsBegin,
						  VertexBaseIterator vrtsEnd,
						  Grid::AttachmentAccessor<VertexBase, APosition1>& aaPos);
/**	\}	*/

////////////////////////////////////////////////////////////////////////
//	CalculateCenter
/// calculates the center of a set of vertices.
/**	The difference to CalculateBarycenter is that this method
 * returns the center of the bounding box which contains the
 * given set of vertices.*/
template <class TAPosition>
typename TAPosition::ValueType
CalculateCenter(VertexBaseIterator vrtsBegin, VertexBaseIterator vrtsEnd,
				Grid::AttachmentAccessor<VertexBase, TAPosition>& aaPos);

////////////////////////////////////////////////////////////////////////
//	CalculateBarycenter - mstepnie
/// calculates the barycenter of a set of vertices
vector3 CalculateBarycenter(VertexBaseIterator vrtsBegin, VertexBaseIterator vrtsEnd,
							Grid::VertexAttachmentAccessor<AVector3>& aaPos);

////////////////////////////////////////////////////////////////////////
//	MergeVertices
///	merges two vertices and restructures the adjacent elements.
/**
 * Since vertex v2 has to be removed in the process, the associated elements
 * of this vertex have to be replaced by new ones. Values attached to
 * old elements are passed on to the new ones using grid::pass_on_values.
 */
void MergeVertices(Grid& grid, VertexBase* v1, VertexBase* v2);

////////////////////////////////////////////////////////////////////////
///	Merges all vertices between the given iterators into a single vertex.
/**	Note that connected elements may be removed or replaced during this process.
 * The method returns the remaining vertex in the given list (*vrtsBegin).*/
template <class TVrtIterator>
VertexBase* MergeMultipleVertices(Grid& grid, TVrtIterator vrtsBegin,
						  	  	  TVrtIterator vrtsEnd);

////////////////////////////////////////////////////////////////////////
//	RemoveDoubles
///	merges all vertices that are closer to each other than the specified threshold.
/**	The current implementation sadly enforces some restrictions to the
 *	container from which iterBegin and iterEnd stem. Only Grid, MultiGrid,
 *	Selector, MGSelector, SubsetHandler, MGSubsetHandler and similar containers
 *	are allowed. This is due to an implementation detail in the algorithm
 *	that should be removed in future revisins.
 *
 *	\todo	remove container restrictions as described above.
 */
template <int dim, class TVrtIterator>
void RemoveDoubles(Grid& grid, const TVrtIterator& iterBegin,
					const TVrtIterator& iterEnd, Attachment<MathVector<dim> >& aPos,
					number threshold);

////////////////////////////////////////////////////////////////////////
///	returns whether a vertex lies on the boundary of a polygonal chain.
/** The polygonal chain may be part of a bigger grid containing faces
 *	and volume elements. To distinguish which edges should be part of
 *	the polygonal chain, you may specify a callback to identify them.
 */
bool IsBoundaryVertex1D(Grid& grid, VertexBase* v,
						CB_ConsiderEdge cbConsiderEdge = ConsiderAllEdges);

////////////////////////////////////////////////////////////////////////
///	returns whether a vertex lies on the boundary of a 2D grid.
/** A vertex is regarded as a 2d boundary vertex if it lies on a
 * 2d boundary edge.
 * if EDGEOPT_STORE_ASSOCIATED_FACES and VRTOPT_STORE_ASSOCIATED_EDGES
 * are enabled, the algorithm will be faster.
 */
bool IsBoundaryVertex2D(Grid& grid, VertexBase* v);

////////////////////////////////////////////////////////////////////////
///	returns true if a vertex lies on the boundary of a 3D grid.
/** A vertex is regarded as a 3d boundary vertex if it lies on a
 * 3d boundary face.
 * if FACEOPT_STORE_ASSOCIATED_VOLUMES and VRTOPT_STORE_ASSOCIATED_FACES
 * are enabled, the algorithm will be faster.
*/
bool IsBoundaryVertex3D(Grid& grid, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	IsRegularSurfaceVertex
///	returns true if the vertex lies inside a regular surface
/**
 * This algorithm indirectly uses Grid::mark.
 *	
 * The vertex is regarded as a regular surface vertex, if all associated
 * edges are connected to exactly 2 faces.
 */
bool IsRegularSurfaceVertex(Grid& grid, VertexBase* v);

////////////////////////////////////////////////////////////////////////
/**
 * Uses Grid::mark()
 *
 * Vertices that are adjacent with more than two crease-edges are
 * regarded as a fixed vertex.
 */
void MarkFixedCreaseVertices(Grid& grid, SubsetHandler& sh,
							int creaseSI, int fixedSI);

////////////////////////////////////////////////////////////////////////
template <class TIterator, class AAPosVRT>
void LaplacianSmooth(Grid& grid, TIterator vrtsBegin,
					TIterator vrtsEnd, AAPosVRT& aaPos,
					number alpha, int numIterations);

////////////////////////////////////////////////////////////////////////
///	returns the position of the vertex.
/**	Main purpose is to allow the use of vertices in template-methods
 *	that call CalculateCenter*/
template<class TVertexPositionAttachmentAccessor>
inline
typename TVertexPositionAttachmentAccessor::ValueType
CalculateCenter(VertexBase* v, TVertexPositionAttachmentAccessor& aaPosVRT);

////////////////////////////////////////////////////////////////////////
///	transforms a vertex by a given matrix
template<class TAAPos> inline
void TransformVertex(VertexBase* vrt, matrix33& m, TAAPos& aaPos);

////////////////////////////////////////////////////////////////////////
///	transforms all given vertices by a given matrix
template<class TIterator, class TAAPos> inline
void TransformVertices(TIterator vrtsBegin, TIterator vrtsEnd,
					   matrix33& m, TAAPos& aaPos);

/// @} // end of doxygen defgroup command

}//	end of namespace

////////////////////////////////
//	include implementation
#include "vertex_util_impl.hpp"

#endif
