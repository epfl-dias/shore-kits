/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   dora_env.h
 *
 *  @brief:  A generic Dora environment class
 *
 *  @note:   All the database need to inherit both from a shore_env, as well as, a dora_env
 *
 *  @author: Ippokratis Pandis, Jun 2009
 */


#ifndef __DORA_ENV_H
#define __DORA_ENV_H


#include <cstdio>

#include "tls.h"
#include "util.h"
#include "shore.h"

#include "dora/range_part_table.h"

#ifdef CFG_FLUSHER
#include "dora/dflusher.h"
#endif

using namespace shore;

ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: dora_env
 *
 * @brief: Generic container class for all the data partitions for
 *         DORA databases
 *
 ********************************************************************/

class DoraEnv
{
public:

    typedef range_partition_impl<int>   irpImpl; 
    typedef range_part_table_impl<int>  irpTableImpl;

    typedef irpImpl::RangeAction  irpAction;

    typedef std::vector<irpTableImpl*>       irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

    typedef vector<base_action_t*> baseActionsList;

protected:

    // A vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    

    // Setup variables
    int _starting_cpu;
    int _cpu_table_step;
    int _cpu_range;
    int _sf;

    int _dora_sf;

#ifdef CFG_FLUSHER
    // The dora-flusher thread
    guard<dora_flusher_t> _flusher;
#endif

public:
    
    DoraEnv();

    virtual ~DoraEnv();

    //// Client API
    
    // enqueues action, false on error
    inline int enqueue(irpAction* paction,
                       const bool bWake, 
                       irpTableImpl* ptable, 
                       const int part_pos) 
    {
        assert (paction);
        assert (ptable);
        return (ptable->enqueue(paction, bWake, part_pos));
    }


#ifdef CFG_FLUSHER
    inline void enqueue_toflush(terminal_rvp_t* arvp) {
        assert (_flusher.get());
        _flusher->enqueue_toflush(arvp);
    }
#endif

    //// Partition-related methods

    irpImpl* table_part(const uint table_pos, const uint part_pos);
    int statistics();

protected:

    int _start(ShoreEnv* penv);
    int _stop();

    // algorithm for deciding the distribution of tables 
    processorid_t _next_cpu(const processorid_t& aprd,
                            const irpTableImpl* atable,
                            const int step=DF_CPU_STEP_TABLES);
    
}; // EOF: DoraEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_ENV_H */

