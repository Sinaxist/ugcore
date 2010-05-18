//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y10 m05 d14

#include <queue>
#include <algorithm>
#include "triangle_fill.h"
#include "common/math/ugmath.h"
#include "lib_grid/algorithms/geom_obj_util/geom_obj_util.h"

using namespace std;

namespace ug
{

/**	Make sure that edgeNormalsOut has the same size as poly-chain.
 *	The poly-chain is treated as a closed poly-chain where an edge
 *	between the last and the first entry exists.*/
void CalculatePolychainEdgeNormals(vector2* edgeNormalsOut, vector2* polyChain,
							size_t polyChainSize, bool bOuterNormals)
{
	if(polyChainSize < 2)
		return;

//	perform a first guess
//	in order to check whether those normals are inner or outer normals,
//	we're building the sum of the normal of an edge with the following edge.
	number dot = 0;
	
//	the edge to which we will calculate the normal
	vector2 e;
	VecSubtract(e, polyChain[1], polyChain[0]);
	VecNormalize(e, e);
	
	for(size_t i = 0; i < polyChainSize; ++i){
		edgeNormalsOut[i].x = e.y;
		edgeNormalsOut[i].y = -e.x;
		
	//	calculate the next edge (the normal in the next iteration
	//	will be calculated for this edge)
		VecSubtract(e, polyChain[(i+2)%polyChainSize],
					   polyChain[(i+1)%polyChainSize]);
		VecNormalize(e, e);
		
	//	Build the dot-product with the last normal
		dot += VecDot(edgeNormalsOut[i], e);
	}
	
//	if the dot-product is smaller than 0, the normal is an outer normal
//	if it is bigger than 0, the normal is an inner normal.
	if((dot < 0 &! bOuterNormals) ||
	   (dot > 0 && bOuterNormals))
	{
	//	we have to invert the normals
		for(size_t i = 0; i < polyChainSize; ++i){
			edgeNormalsOut[i].x = -edgeNormalsOut[i].x;
			edgeNormalsOut[i].y = -edgeNormalsOut[i].y;
		}
	}
}

////////////////////////////////////////////////////////////////////////
struct ChainInfo{
	ChainInfo()	{}
	ChainInfo(int vrtInd, int vrtIndIn, int vrtIndOut) :
		myVrt(vrtInd), inVrt(vrtIndIn), outVrt(vrtIndOut),
		isCandidate(false), associatedDistanceSq(0)	{}
		
	bool operator == (const ChainInfo& ci){
		return myVrt == ci.myVrt && inVrt == ci.inVrt && outVrt == ci.outVrt
				&& isCandidate == ci.isCandidate
				&& associatedDistanceSq == ci.associatedDistanceSq;
		}
	
	bool operator < (const ChainInfo& ci) const	{return associatedDistanceSq > 
													ci.associatedDistanceSq;}

	int myVrt;
	int inVrt;
	int outVrt;
	bool isCandidate;
	number associatedDistanceSq;
};

void UpdateChainInfo(ChainInfo& ci, vector2* polyChain, vector2* edgeNormals)
{
	vector2 outEdge;
	VecSubtract(outEdge, polyChain[ci.outVrt], polyChain[ci.myVrt]);
	ci.isCandidate = VecDot(edgeNormals[ci.inVrt], outEdge) < -SMALL; //ci.inVrt == inEdgeIndex.
	ci.associatedDistanceSq = VecDistanceSq(polyChain[ci.inVrt],
											polyChain[ci.outVrt]);
}

////////////////////////////////////////////////////////////////////////
bool TriangleFill(std::vector<int>& vTriIndsOut, vector2* polyChain,
					size_t polyChainSize, bool bTriangulateInside)
{
	vTriIndsOut.clear();
	
	if(polyChainSize < 2)
		return false;
		
//	calculate the normals
	vector<vector2> vNormals(polyChainSize);
	CalculatePolychainEdgeNormals(&vNormals.front(), polyChain,
									polyChainSize, bTriangulateInside);

//tmp. create edges from the edges mid-point along their normals

//	set up a chain that holds the in- and out- edges of each point
//	together with a value that determines the priority.
//	The closer to 0, the higher the priority

//	chain infos are stored in a vector
//	this chain always holds valid and up to date elements
	vector<ChainInfo> chainInfos(polyChainSize);
	
//	create a priority queue that holds pointers to the chain-infos.
//	warning: this queue will contain invalid entries later on.
//	this is handled by a special check before an element is used.
	priority_queue<ChainInfo> qChainInfos;
	for(size_t i = 0; i < polyChainSize; ++i){
		int inVrtInd = (polyChainSize + i - 1) % polyChainSize;//avoid negativity in %
		int outVrtInd = (i + 1) % polyChainSize;
		chainInfos[i] = ChainInfo(i, inVrtInd, outVrtInd);
		UpdateChainInfo(chainInfos[i], polyChain, &vNormals.front());
		if(chainInfos[i].isCandidate)
			qChainInfos.push(chainInfos[i]);
	}
	
//	iterate until all triangles have been created.
	int counter = polyChainSize - 2;
	while(counter > 0 &! qChainInfos.empty())
	{
	//	get the element with top priority
		ChainInfo ci = qChainInfos.top();
		qChainInfos.pop();
		if(!ci.isCandidate){
			continue;
		}
	
	//	we have to make sure that the chain info in the 
	//	priority queue matches the actual chain-entry.
		if(ci == chainInfos[ci.myVrt])
		{
		//	check whether the triangle is a valid triangle or not.
		//	do this by checking for all points whether they lie on the triangle.
			vector2& p0 = polyChain[ci.inVrt];
			vector2& p1 = polyChain[ci.myVrt];
			vector2& p2 = polyChain[ci.outVrt];
			
		//	the bounding rect of the points
			vector2 vMin;
			vector2 vMax;
			vMin.x = min(p0.x, min(p1.x, p2.x));
			vMin.y = min(p0.y, min(p1.y, p2.y));
			vMax.x = max(p0.x, max(p1.x, p2.x));
			vMax.y = max(p0.y, max(p1.y, p2.y));
			
		//	only consider points that lie in the box
//TODO: replace this with a tree lookup.
			bool badTri = false;
			for(size_t i = 0; i < polyChainSize; ++i){
				if(i != ci.inVrt && i != ci.outVrt && i != ci.myVrt){
					vector2& p = polyChain[i];
					if(BoxBoundProbe(p, vMin, vMax))
					{
						if(PointIsInsideTriangle(p, p0, p1, p2)){
							//UG_LOG(" bad tri in " << ci.myVrt << " - ");
							badTri = true;
							break;
						}
					}
				}
			}
			
		//	if no points lie in the triangle we can create it.
			if(!badTri){
				--counter;
				if(bTriangulateInside){
					vTriIndsOut.push_back(ci.inVrt);
					vTriIndsOut.push_back(ci.myVrt);
					vTriIndsOut.push_back(ci.outVrt);
				}
				else{
					vTriIndsOut.push_back(ci.outVrt);
					vTriIndsOut.push_back(ci.myVrt);
					vTriIndsOut.push_back(ci.inVrt);
				}
				
			//	calculate the new normal of the edge
				vector2 eNew;
				VecSubtract(eNew, polyChain[ci.outVrt], polyChain[ci.inVrt]);
				vector2 nNew;
				nNew.x = eNew.y;
				nNew.y = -eNew.x;
			//	make sure that the normal points into the right direction.
				vector2 vTestNorm;
				VecAdd(vTestNorm, vNormals[ci.inVrt], vNormals[ci.myVrt]);
				if(VecDot(nNew, vTestNorm) < 0)
					VecScale(nNew, nNew, -1.);
				VecNormalize(vNormals[ci.inVrt], nNew);
				
				
			//	we have to update the chain-infos and the queue
				chainInfos[ci.myVrt].isCandidate = false;
				ChainInfo& ciIn = chainInfos[ci.inVrt];
				ChainInfo& ciOut = chainInfos[ci.outVrt];
				ciIn.outVrt = ci.outVrt;
				ciOut.inVrt = ci.inVrt;
				UpdateChainInfo(ciIn, polyChain, &vNormals.front());
				UpdateChainInfo(ciOut, polyChain, &vNormals.front());
				if(ciIn.isCandidate)
					qChainInfos.push(ciIn);
				if(ciOut.isCandidate)
					qChainInfos.push(ciOut);
			}
		}
	}
/*
	if(counter == 0){
		UG_LOG("  counter is empty. ");
	}
	if(qChainInfos.empty()){
		UG_LOG("  queue is empty. ");
	}
*/
//	done. return true
	return true;
}

bool CreatePolyChain(std::vector<VertexBase*>& polyChainOut, Grid& grid,
					EdgeBaseIterator edgesBegin, EdgeBaseIterator edgesEnd)
{
	polyChainOut.clear();
//	collect the chain.
//	do the trivial projection
//TODO: replace the trivial projection by a matrix transformation + projection

	grid.begin_marking();
	
//	mark and count edges
	int numEdges = 0;
	for(EdgeBaseIterator iter = edgesBegin; iter != edgesEnd; ++iter){
		grid.mark(*iter);
		++numEdges;
	}
	
//TODO: handle open chains.
	EdgeBase* actEdge = *edgesBegin;
	VertexBase* actVrt = actEdge->vertex(1);
	polyChainOut.push_back(actEdge->vertex(0));
	grid.mark(actEdge->vertex(0));
	polyChainOut.push_back(actEdge->vertex(1));
	grid.mark(actEdge->vertex(1));
	
	bool bRunning = true;
	while(bRunning){
		bRunning = false;
	//	find a connected  unmarked vertex
		EdgeBaseIterator assEdgesEnd = grid.associated_edges_end(actVrt);
		for(EdgeBaseIterator eIter = grid.associated_edges_begin(actVrt);
			eIter != assEdgesEnd; ++eIter)
		{
			EdgeBase* e = *eIter;
			if(grid.is_marked(e)){
			//	check whether the connected vertex is unmarked
				VertexBase* cv = GetConnectedVertex(e, actVrt);
				if(!grid.is_marked(cv)){
				//	push it to the chain and mark it.
					grid.mark(cv);
					polyChainOut.push_back(cv);
				//	we're done with actVrt. cv will be the new actVrt
					actVrt = cv;
					bRunning = true;
					break;
				}
			}
		}
	}

	grid.end_marking();
/*
	if(polyChainOut.size() != numEdges)
		return false;
*/
	return true;
}

bool TriangleFill(Grid& grid, EdgeBaseIterator edgesBegin,
				EdgeBaseIterator edgesEnd, bool bTriangulateInside)
{
	if(edgesBegin == edgesEnd)
		return false;

	if(!grid.has_vertex_attachment(aPosition))
		return false;
		
	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);
	
//	get the vertices of the poly-chain
	std::vector<VertexBase*> vrtPolyChain;
	CreatePolyChain(vrtPolyChain, grid, edgesBegin, edgesEnd);

//	a poly-chain that stores positions
	std::vector<vector3> polyChain3D(vrtPolyChain.size());
	for(size_t i = 0; i < vrtPolyChain.size(); ++i)
		polyChain3D[i] = aaPos[vrtPolyChain[i]];
		
//	perform transform to 2d	
	std::vector<vector2> polyChain2D(vrtPolyChain.size());
	TransformPointSetTo2D(&polyChain2D.front(), &polyChain3D.front(),
						  polyChain3D.size());
/*
//TODO: perform a more elaborated projection!
	for(size_t i = 0; i < vrtPolyChain.size(); ++i){
		vector3& v = aaPos[vrtPolyChain[i]];
		polyChain2D[i] = vector2(v.x, v.y);
	}
*/	

//	perform triangulation
	vector<int> triIndices;
	if(TriangleFill(triIndices, &polyChain2D.front(), polyChain2D.size(),
					bTriangulateInside))
	{
	//	create the triangles
		for(size_t i = 0; i < triIndices.size(); i+=3){
			grid.create<Triangle>(TriangleDescriptor(vrtPolyChain[triIndices[i]],
													vrtPolyChain[triIndices[i + 1]],
													vrtPolyChain[triIndices[i + 2]]));
		}
		
		return true;
	}

/*
//tmp. create edges from the edges mid-point along their normals
	//	calculate the normals
	vector<vector2> vNormals(polyChain2D.size());
	CalculatePolychainEdgeNormals(&vNormals.front(), &polyChain2D.front(),
									polyChain2D.size(), bTriangulateInside);
	
	for(size_t i = 0; i < polyChain2D.size(); ++i){
		VertexBase* vrt1 = *grid.create<Vertex>();
		VertexBase* vrt2 = *grid.create<Vertex>();
		EdgeBase* e = *grid.create<Edge>(EdgeDescriptor(vrt1, vrt2));
		
		VecAdd(aaPos[vrt1], aaPos[vrtPolyChain[i]], aaPos[vrtPolyChain[(i+1)%vrtPolyChain.size()]]);
		VecScale(aaPos[vrt1], aaPos[vrt1], 0.5);
		
		aaPos[vrt2].x = aaPos[vrt1].x + vNormals[i].x * 0.1;
		aaPos[vrt2].y = aaPos[vrt1].y + vNormals[i].y * 0.1;
		aaPos[vrt2].z = aaPos[vrt1].z;
	}
	return true;
*/

	return false;
}

}//	end of namespace
