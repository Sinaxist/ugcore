// created by Sebastian Reiter
// s.b.reiter@googlemail.com
// 18.11.2011 (m,d,y)
 
#include "message_hub.h"
#include "common/assert.h"
#include "common/error.h"

namespace ug{

MessageHub::CallbackId::
CallbackId(MessageHub* hub, int msgId, int callbackIndex, bool autoFree) :
	m_hub(hub),
	m_msgId(msgId),
	m_callbackIndex(callbackIndex),
	m_autoFree(autoFree)
{
}

MessageHub::CallbackId::~CallbackId()
{
	if(m_autoFree)
		m_hub->unregister_callback_impl(this);
}



MessageHub::MessageHub() :
	m_highestMsgId(0)
{
}

void MessageHub::
unregister_callback(MessageHub::SPCallbackId cbId)
{
	unregister_callback_impl(cbId.get_impl());
}

void MessageHub::
unregister_callback_impl(MessageHub::CallbackId* cbId)
{
	if(cbId->m_msgId == -1){
		throw(Error("MessageHub::unregister_callback: Invalid callback-id. "
						"The callback was probably already unregistered.",
						MSG_HUB_BAD_CALLBACK_ID));
	}

	UG_ASSERT(cbId->m_hub == this, "Wrong MessageHub");
	UG_ASSERT((cbId->m_msgId >= 0) && (cbId->m_msgId < m_highestMsgId),
			  "Bad message id");

	CallbackVec& callbacks = *m_callbackTable[cbId->m_msgId].get_impl();
	UG_ASSERT((cbId->m_callbackIndex >= 0)
			  && (cbId->m_callbackIndex < (int)callbacks.size()),
			  "Bad callback id");

//	set autoFree to false, since it was already freed
	cbId->m_autoFree = false;

//	set m_msgId to -1, to indicate, that the id is invalid
	cbId->m_msgId = -1;

//	clear the entry
	callbacks[cbId->m_callbackIndex].clear();
}

}// end of namespace
