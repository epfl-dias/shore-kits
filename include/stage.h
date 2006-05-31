/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _STAGE_H
#define _STAGE_H

#include "thread.h"
#include "tuple.h"
#include "packet.h"



// include me last!!!
#include "namespace.h"



class stage_t;



/**
 *  @brief Convenient wrapper around a tuple_buffer_t that will ensure
 *  that the buffer is closed when wrapper goes out of scope. By using
 *  this wrapper, we can avoid duplicating close() code at every exit
 *  point.
 */

struct buffer_closer_t {

    tuple_buffer_t *buffer;
    buffer_closer_t(tuple_buffer_t *buf)
        : buffer(buf)
    {
    }
  
    ~buffer_closer_t() {
        buffer->close();
    }

    tuple_buffer_t *operator->() {
        return buffer;
    }

};



/**
 *  @brief A QPIPE stage is a queue of packets (work that must be
 *  completed) and a process_next_packet() function that worker
 *  threads can call to process the packets.
 */

class stage_t {

public:

    typedef enum {
        STAGE_IPROBE = 1,
        STAGE_INSERT,
        STAGE_UPDATE,
        STAGE_DELETE,
        STAGE_SCAN,
        STAGE_AGGREGATE,
        STAGE_SORT,
        STAGE_JOIN,
        STAGE_OTHER,
    } type_t;

protected:

    // general information about the stage
    char* stage_name;

    // synch vars
    pthread_mutex_t stage_lock;
    pthread_cond_t  stage_queue_packet_available;
    
    // packet queue
    packet_list_t stage_queue;
    
    // set of packets currently being processed
    packet_list_t merge_candidates;
    
public:

    stage_t(const char* sname);
    virtual ~stage_t();


    /**
     *  @brief Accessor for this stage's name.
     */
    const char* get_name() const { return stage_name; }


    /* The dispatcher can use this method to send work to this stage. */
    void enqueue(packet_t*);


    /* A worker thread for this stage should loop around this
       function. */
    int process_next_packet();


protected:

    
    /* Each stage must override this method with the functionality of
       that stage. */
    virtual int process_packet(packet_t *packet)=0;


    packet_t* dequeue();
    

    void set_not_mergeable(packet_t *packet);
    

    /**
     *  @brief Write a tuple to each waiting output buffer in a chain of
     *  packets.
     *
     *  @return 0 on success. Non-zero on error (such as early
     *  termination).
     */

    int output(packet_t* packet, const tuple_t &tuple);

    /**
     *  @brief cleans up after completing work on a packet.
     */

    void done(packet_t *packet);

};



#include "namespace.h"
#endif