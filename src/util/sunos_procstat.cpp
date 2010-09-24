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

/** @file:   sunos_procstat.cpp
 *
 *  @brief:  Implementation of a class that provides information about
 *           the CPU usage during runs at SunOS 
 *
 *  @author: Ippokratis Pandis (ipandis)
 *  @author: Ryan Johnson (ryanjohn)
 */

#include "util/sunos_procstat.h"
#include "util.h"

sunos_procmonitor_t::sunos_procmonitor_t(const double interval_sec)
    : procmonitor_t("sunos-mon",interval_sec), _pkc(NULL)
{ 
    _setup(interval_sec);
}


sunos_procmonitor_t::~sunos_procmonitor_t() 
{ 
    if (*&_state != CPS_NOTSET) {            
        kstat_close(_pkc);
    }
}


//// Interface

void sunos_procmonitor_t::stat_reset()
{
    _prcinfo.reset();
}

double sunos_procmonitor_t::get_avg_usage() 
{ 
    return (*&_avg_usage); 
}

void sunos_procmonitor_t::print_avg_usage() 
{ 
    double au = *&_avg_usage;
    double entriessz = _entries.size();
    TRACE( TRACE_STATISTICS, 
           "\nAvgCPU:       (%.1f) (%.1f%%)\n",
           au, 100*(au/entriessz));
}

void sunos_procmonitor_t::print_ext_stats()
{
    _prcinfo.print();
}

ulong_t sunos_procmonitor_t::iochars()
{
    return (_prcinfo.iochars());
}

uint sunos_procmonitor_t::get_num_of_cpus()
{
    return (_prcinfo.get_num_of_cpus());
}

cpu_load_values_t sunos_procmonitor_t::get_load()
{
    return (_prcinfo.get_load());
}


//// Private functions 

void sunos_procmonitor_t::_setup(const double interval_sec) 
{
    procmonitor_t::_setup(interval_sec);

    _entries.clear();
    _entries.reserve(64); 
    
    // get set up
    // - open the kstat
    // - find and stash all the cpu::sys kstats
    // - find and stash the offset of the cpu::sys:cpu_nsec_idle entry
    // - take the first measurement

    _pkc = kstat_open();
    for(kstat_t* ksp=_pkc->kc_chain; ksp; ksp=ksp->ks_next) {
	if(strcmp(ksp->ks_module, "cpu") == 0 && strcmp(ksp->ks_name, "sys") == 0) {
	    int kid = kstat_read(_pkc, ksp, 0);
	    kstat_named_t* kn = (kstat_named_t*) ksp->ks_data;
	    for(long i=0; i < ksp->ks_ndata; i++) {
		if(strcmp(kn[i].name, "cpu_nsec_idle") == 0) {
		    kstat_entry entry = {ksp, i, {{ksp->ks_snaptime, kn->value.ui64}, {0, 0}}};
		    _entries.push_back(entry);
		    break;
		}
	    }
	}
    }
}


void sunos_procmonitor_t::work() 
{
    if (*&_state==CPS_NOTSET) _setup(_interval_sec);
    assert (*&_state!=CPS_NOTSET);
    
    bool first_time = true;
    struct timespec start;

    long last_measurement = 1;

    eCPS astate = *&_state;
    
    long new_measurement = 0;
    cpu_measurement totals = {0,0};

    double entries_sz = _entries.size();
    double inuse = 0;        
    int error = 0;
    struct timespec ts = start;
    
    static long const BILLION = 1000*1000*1000;

    assert (_pkc);
    int i=0;
    kstat_entry* e = NULL;
    int kid;
    cpu_measurement m = {0,0};
    kstat_named_t* kn = NULL;

    pthread_mutex_lock(&_mutex);
    clock_gettime(CLOCK_REALTIME, &start);

    while (true) {

        astate = *&_state;
            
        switch (astate) {
        case (CPS_RUNNING):
        case (CPS_PAUSE):    // PAUSE behaves like running without recording data
            
            new_measurement = 1 - last_measurement;

            // get the new measurement
            totals.clear();

            for(i=0; i<entries_sz; i++) {
                e = &_entries[i];
                kid = kstat_read(_pkc, e->ksp, 0);
                kn = ((kstat_named_t*) e->ksp->ks_data) + e->offset;
                m.set(e->ksp->ks_snaptime, kn->value.ui64);
                e->measured[new_measurement] = m;
                m -= e->measured[last_measurement];
                totals += m;
            }

            // record usage if not paused
            if ((!first_time) && (astate==CPS_RUNNING)) {
                inuse = entries_sz - entries_sz*totals.cpu_nsec_idle/totals.timestamp;

                // update total usage and calculate average
                ++_num_usage_readings;
                _total_usage += inuse;
                _avg_usage = _total_usage/_num_usage_readings; 
            }

            first_time = false;

            ts = start;
            ts.tv_nsec += _interval_usec*1000;
            if(ts.tv_nsec > BILLION) {
                ts.tv_nsec -= BILLION;
                ts.tv_sec++;
            }
            
            // sleep periodically until next measurement
            while(true) {

                error = pthread_cond_timedwait(&_cond, &_mutex, &ts);
                clock_gettime(CLOCK_REALTIME, &start);
                if(start.tv_sec > ts.tv_sec || 
                   (start.tv_sec == ts.tv_sec && start.tv_nsec > ts.tv_nsec))
                    break;
            }
            start = ts;
            last_measurement = new_measurement;
            break;

        case (CPS_RESET):
            // clear
            _total_usage = 0;
            _num_usage_readings = 0;
            _avg_usage = 0;
            first_time=true;
            last_measurement = 1;
            _state = CPS_RUNNING;
            break;

        case (CPS_STOP):
            return;

        case (CPS_NOTSET): 
        default:
            assert(0); // invalid value
            break;
        }
    }
}
