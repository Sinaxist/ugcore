//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m12 d05

#ifndef __H__PCL__PCL_COMMUNICATOR__
#define __H__PCL__PCL_COMMUNICATOR__

#include <iostream>
#include <map>
#include "common/util/binary_stream.h"
#include "common/util/stream_pack.h"
#include "pcl_communication_structs.h"

namespace pcl
{
////////////////////////////////////////////////////////////////////////
//	There should be two types of communicators:
//	- InterfaceCommunicator: Data is exchanged between elements of
//				interfaces / layouts. CommunicationPolicies are used to
//				collect and to extract interface-data.
//				Benefits are low communication overhead and ease of use.
//
//	- ProcessCommunicator: Arbitrary data is exchanged between all processes
//				that are linked by layouts.
//				Benefits are high flexibility and independency of
//				interface element order.
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
//	ParallelCommunicator
///	Performs communication between interfaces on different processes.
template <class TLayout>
class ParallelCommunicator
{
	public:
	//	typedefs
		typedef TLayout 					Layout;
		typedef typename Layout::Interface	Interface;
		typedef typename Layout::Type		Type;

	protected:
		typedef ICommunicationPolicy<Layout>	CommPol;
		
	public:
		ParallelCommunicator();
		
	////////////////////////////////
	//	SEND
	
	///	sends raw data to a target-proc.
	/**	Shedules the data in pBuff to be sent to the target-proc
	 *	pBuff can be reused or cleared directly after the call returns.
	 *
	 *	Data sent with this method can be received using receive_raw.
	 *
	 *	Please note that this method should only be used if custom data
	 *	should be send in a block with data that is communicated through
	 *	interfaces, since an additional copy-operation at the target process
	 *	has to be performed.
	 *	If you're only interested in sending raw data, you should take a
	 *	look into pcl::ProcessCommunicator::send.
	 */
		void send_raw(int targetProc, void* pBuff, int bufferSize,
					   bool bSizeKnownAtTarget = false);

	///	collects data that will be send during communicate.
	/**	Calls ICommunicationPolicy<TLayout>::collect with the specified
	 *	interface and the binary stream that is associated with the
	 *	specified target process.
	 *	Note that data will not be send until communicate has been called.
	 *	\sa receive_data, exchange_data*/
		void send_data(int targetProc,
						  Interface& interface,
						  ICommunicationPolicy<TLayout>& commPol);

	///	collects data that will be send during communicate.
	/**	Calls ICommunicationPolicy<TLayout>::collect with the specified
	 *	layout and the binary stream that is associated with the
	 *	layouts target processes.
	 *	Note that data will not be send until communicate has been called.
	 *	\sa receive_data, exchange_data*/
		void send_data(Layout& layout,
						  ICommunicationPolicy<TLayout>& commPol);

	////////////////////////////////
	//	RECEIVE
	
	///	registers a binary-stream to receive data from a source-proc.
	/**	Receives data that has been sent with send_raw.
	 *	If send_raw was called with bSizeKnownAtTarget == true, an
	 *	exact bufferSize has to be specified. If bSizeKnownAtTarget was
	 *	set to false, bufferSize has to be set to -1.
	 *
	 *	Make sure that binStreamOut exists until communicate has been
	 *	executed.
	 *
	 *	Please note that this method should only be used if custom data
	 *	should be send in a block with data that is communicated through
	 *	interfaces, since an additional copy-operation has to be performed.
	 *	If you're only interested in sending raw data, you should take a
	 *	look into pcl::ProcessCommunicator::receive.
	 */
		void receive_raw(int srcProc, ug::BinaryStream& binStreamOut,
						 int bufferSize = -1);

	///	registers a buffer to receive data from a source-proc.
	/**	Receives data that has been sent with send_raw.
	 *	This method may only be used if send_raw was called with
	 *	bSizeKnownAtTarget == true. Call receive_raw with a binary-
	 *	stream instead, if buffer-sizes are not known.
	 *
	 *	Make sure that the buffer points to valid memory until
	 *	communicate has been executed.
	 *
	 *	Please note that this method should only be used if custom data
	 *	should be send in a block with data that is communicated through
	 *	interfaces, since an additional copy-operation has to be performed.
	 *	If you're only interested in sending raw data, you should take a
	 *	look into pcl::ProcessCommunicator::receive.
	 */
		void receive_raw(int srcProc, void* buffOut,
						 int bufferSize);
		
	///	registers a communication-policy to receive data on communicate.
	/**	Receives have to be registered before communicate is executed.
		make sure that your instance of the communication-policy
		exists until communicate has benn executed.*/
		void receive_data(int srcProc,
						Interface& interface,
						ICommunicationPolicy<TLayout>& commPol);

	///	registers an communication-policy to receive data on communicate.
	/**	Receives have to be registered before communicate is executed.
		make sure that your instance of the communication-policy
		exists until communicate has benn executed.*/
		void receive_data(Layout& layout,
						ICommunicationPolicy<TLayout>& commPol);

	////////////////////////////////
	//	EXCHANGE
	///	internally calls send_data and receive_data with the specified layouts.
	/**	Note that data is not communicated until communicate has been called.
	 *
	 *	Make sure that the Layout- and the Interface-type of TLayoutMap
	 *	are compatible with the layout of the communicator.
	 *	Layouts are queried for TLayout::Type of the communicators TLayout-type.
	 *
	 *	This method is particularily useful if you categorize layouts on a
	 *	process. If you separate your layouts into master and slave layouts,
	 *	you could use this method e.g. to copy data from all master-layouts
	 *	to all slave-layouts of a type with a single call.*/
		template <class TLayoutMap>
		void exchange_data(TLayoutMap& layoutMap,
							typename TLayoutMap::Key keyFrom,
							typename TLayoutMap::Key keyTo,
							ICommunicationPolicy<TLayout>& commPol);
	
	///	sends and receives the collected data.
	/**	The collected data will be send to the associated processes.
	 *	The extract routines of the communication-policies which were registered
	 *	through Communicator::await_data will be called with the received
	 *	data. After all received data is processed, the communication-policies are
	 *	released. Make sure that you will keep your communication-policies
	 *	in memory until this point.*/
		bool communicate();
		
	protected:
	///	helper to collect data from single-level-layouts
		void send_data(Layout& layout,
				  ICommunicationPolicy<TLayout>& commPol,
				  const layout_tags::single_level_layout_tag&);

	///	helper to collect data from multi-level-layouts				  
		void send_data(Layout& layout,
				  ICommunicationPolicy<TLayout>& commPol,
				  const layout_tags::multi_level_layout_tag&);
	
	///	prepare stream-pack-in
		void prepare_receiver_stream_pack(ug::StreamPack& streamPack,
											TLayout& layout);
	/// specialization of stream-pack preparation for single-level-layouts
		void prepare_receiver_stream_pack(ug::StreamPack& streamPack,
										TLayout& layout,
										const layout_tags::single_level_layout_tag&);
	/// specialization of stream-pack preparation for multi-level-layouts
		void prepare_receiver_stream_pack(ug::StreamPack& streamPack,
										TLayout& layout,
										const layout_tags::multi_level_layout_tag&);

	///	collects buffer sizes for a given layout and stores them in a map
	/**	The given map holds pairs of procID, bufferSize
	 *	If buffer-sizes can't be determined, false is returned.
	 *	if pMmapBuffSizesOut == NULL, the method will simply determine
	 *	whether all buffersizes can be calculated.*/
	 	bool collect_layout_buffer_sizes(TLayout& layout,
										ICommunicationPolicy<TLayout>& commPol,
										std::map<int, int>* pMapBuffSizesOut,
										const layout_tags::single_level_layout_tag&);

	///	collects buffer sizes for a given layout and stores them in a map
	/**	The given map holds pairs of procID, bufferSize
	 *	If buffer-sizes can't be determined, false is returned.
	 *	if pMmapBuffSizesOut == NULL, the method will simply determine
	 *	whether all buffersizes can be calculated.*/
	 	bool collect_layout_buffer_sizes(TLayout& layout,
										ICommunicationPolicy<TLayout>& commPol,
										std::map<int, int>* pMapBuffSizesOut,
										const layout_tags::multi_level_layout_tag&);	
	
	///	extract data from stream-pack
		void extract_data(TLayout& layout, ug::StreamPack& streamPack,
						CommPol& extractor);
		
		void extract_data(TLayout& layout, ug::StreamPack& streamPack,
						CommPol& extractor,
						const layout_tags::single_level_layout_tag&);
		
		void extract_data(TLayout& layout, ug::StreamPack& streamPack,
						CommPol& extractor,
						const layout_tags::multi_level_layout_tag&);
		
	protected:		
	///	holds information that will be passed to the extract routines.
	/**	if srcProc == -1, the layout will be used for extraction.
	 *	if srcProc >= 0, either the buffer, the binaryStream or the
	 *	interace will be used for extraction, depending on which is
	 *	not NULL.
	 */
		struct ExtractorInfo
		{
			ExtractorInfo()			{}
			ExtractorInfo(int srcProc, CommPol* pExtractor,
						Interface* pInterface, Layout* pLayout,
						void* buffer, ug::BinaryStream* stream, int rawSize) :
				m_srcProc(srcProc), m_extractor(pExtractor),
				m_interface(pInterface), m_layout(pLayout),
				m_buffer(buffer), m_stream(stream), m_rawSize(rawSize)	{}

			int					m_srcProc;
			CommPol*			m_extractor;			
			Interface*			m_interface;
			Layout*				m_layout;
			void*				m_buffer;
			ug::BinaryStream*	m_stream;
			int					m_rawSize;
		};

	///	A list that holds information about extractors.
		typedef std::list<ExtractorInfo> ExtractorInfoList;

	protected:
	///	holds the streams that are used to send data
		ug::StreamPack		m_streamPackOut;
	///	holds the streams that are used to receive data
		ug::StreamPack		m_streamPackIn;
	///	holds information about the extractors that are awaiting data.
		ExtractorInfoList	m_extractorInfos;
		
	///	holds info whether all send-buffers are of predetermined fixed size.
	/**	reset to true after each communication-step.*/
		bool m_bSendBuffersFixed;
};

}//	end of namespace pcl

////////////////////////////////////////
//	include implementation
#include "pcl_communicator_impl.hpp"

#endif
