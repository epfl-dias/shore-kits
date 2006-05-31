/* -*- mode:C++; c-basic-offset:4 -*- */

#include "thread.h"
#include "stage.h"
#include "trace/trace.h"



// include me last!
#include "namespace.h"



/**
 *  @brief packet_t constructor.
 */

packet_t::packet_t(DbTxn* tid,
		   char* pack_id,
		   tuple_buffer_t* outbuf,
		   tuple_filter_t* filt,
                   bool mergeable)
{
  xact_id = tid;
  packet_id = pack_id;
  output_buffer = outbuf;
  filter = filt;
  this->mergeable = mergeable;

  pthread_mutex_init_wrapper( &merge_mutex, NULL );
  merged_packets.push_front(this);
  
  TRACE(TRACE_PACKET_FLOW, "Created a new packet with ID %s\n", packet_id);
}



/**
 *  @brief packet_t destructor.
 */

packet_t::~packet_t(void) {
    
  TRACE(TRACE_PACKET_FLOW, "Destroying packet with ID %s\n", packet_id);

  pthread_mutex_destroy_wrapper(&merge_mutex);

  TRACE(TRACE_ALWAYS, "packet_t destructor not fully implemented... need to destroy packet ID\n");
}



/**
 *  @brief Link the specified packet to this one.
 */

void packet_t::merge(packet_t* packet) {
    
  TRACE(TRACE_PACKET_FLOW, "Adding packet %s to chain of packet %s\n",
	packet->packet_id,
	packet_id);
  
  pthread_mutex_lock_wrapper(&merge_mutex);
  
  // add packet to head of merge set

  merged_packets.push_front(packet);
  
  pthread_mutex_unlock_wrapper(&merge_mutex);
}



#include "namespace.h"