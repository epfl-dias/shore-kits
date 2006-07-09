// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/stages/aggregate.h"
#include "engine/core/tuple.h"
#include "engine/util/guard.h"
#include "engine/dispatcher.h"
#include "qpipe_panic.h"
#include "trace.h"



const char* aggregate_packet_t::PACKET_TYPE = "AGGREGATE";



const char* aggregate_stage_t::DEFAULT_STAGE_NAME = "AGGREGATE_STAGE";



stage_t::result_t aggregate_stage_t::process_packet() {


    adaptor_t* adaptor = _adaptor;
    aggregate_packet_t* packet = (aggregate_packet_t*)adaptor->get_packet();

    
    tuple_aggregate_t* aggregate = packet->_aggregator;
    key_extractor_t* extract = packet->_extract;
    tuple_buffer_t* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);


    // "I" own dest, so allocate space for it on the stack
    size_t dest_size = packet->_output_buffer->tuple_size;
    char dest_data[dest_size];
    tuple_t dest(dest_data, dest_size);


    size_t agg_size = aggregate->tuple_size();
    char agg_data[agg_size];
    tuple_t agg(agg_data, agg_size);
    
    size_t key_size = extract->key_size();
    char* last_key = aggregate->key_extractor()->extract_key(agg_data);

    bool first = true;
    while (1) {

        tuple_t src;
        int result = input_buffer->get_tuple(src);
        
        if(result) {
            // No more tuples? Exit from loop, but can't return quite
            // yet since we may still have one more aggregation to
            // perform.
            if(result == 1)
                break;

            // unknown error code?
            assert(result == -1);
            
            // producer has terminated buffer!
            TRACE(TRACE_DEBUG, "Detected input buffer termination. Halting aggregation\n");
            return stage_t::RESULT_ERROR;
        }
            
        // got another tuple
        const char* key = extract->extract_key(src);

        // break group?
        if(first || (key_size && memcmp(last_key, key, key_size))) {
            if(!first) {
                aggregate->finish(dest, agg.data);
                result_t output_ret = adaptor->output(dest);
                if (output_ret)
                    return output_ret;
            }
            aggregate->init(agg.data);
            memcpy(last_key, key, key_size);
            first = false;
        }
        
        aggregate->aggregate(agg.data, src);
    }

    // output the last group, if any
    if(!first) {
        aggregate->finish(dest, agg.data);
        result_t output_ret = adaptor->output(dest);
        if (output_ret)
            return output_ret;
    }

    return stage_t::RESULT_STOP;
}