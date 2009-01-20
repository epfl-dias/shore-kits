/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file range_partition.h
 *
 *  @brief Range partition class in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_RANGE_PARTITION_H
#define __DORA_RANGE_PARTITION_H

#include <cstdio>

#include "util.h"
#include "sm/shore/shore_env.h"

#include "dora/partition.h"
#include "dora/key.h"
#include "dora/action.h"


//#include "dora.h"




ENTER_NAMESPACE(dora);


using namespace shore;




/******************************************************************** 
 *
 * @enum:  eRangePartitionState
 *
 * @brief: Status for the range data partition
 *
 ********************************************************************/

enum eRangePartitionState { RPS_UNSET, RPS_SET };



/******************************************************************** 
 *
 * @class: range_partition_impl
 *
 * @brief: Template-based class for the range data partition
 *
 * @note:  There is a single data type for the comparing fields
 *
 ********************************************************************/

template <class DataType>
class range_partition_impl : public partition_t<DataType>
{
public:
    
    typedef range_action_impl<DataType> RangeAction;

    typedef typename action_t<DataType>          Action;
    typedef typename key_wrapper_t<DataType>     Key;
    //    using partition_t<DataType>::Key;

private:

    eRangePartitionState _rp_state;
    int _part_field_cnt;

    // the two bounds
    Key _part_down;
    Key _part_up;

    //guard<Pool> _keypool;

    /** helper functions */

    const bool _is_between(const Key& contDown, 
                           const Key& contUp) const;

public:

    range_partition_impl(ShoreEnv* env, table_desc_t* ptable,                          
                         const int field_cnt,
                         const int keyEstimation,
                         const int apartid = 0, processorid_t aprsid = PBIND_NONE) 
        : partition_t(env, ptable, keyEstimation, apartid, aprsid),
          _rp_state(RPS_UNSET),
          _part_field_cnt(field_cnt)
    {
        assert (_part_field_cnt>0);
        set_part_policy(PP_RANGE);

//         _keypool = new Pool(sizeof(DataType),4);
//         _down = new Key( _keypool.get() );
//         _up = new Key ( _keypool.get() );
    }


    ~range_partition_impl() { }


    const Key* down() { return (&_down); }
    const Key* up()   { return (&_up); }    

    // sets new limits
    const bool resize(const Key& downLimit, const Key& upLimit);

    // returns true if action can be enqueued here
    const bool verify(Action& rangeaction);

}; // EOF: range_partition_impl


/** Helper functions */


/****************************************************************** 
 *
 * @fn:    _is_between
 *
 * @brief: Returns true if (BaseDown < ContenderDown) (ContenderUp <= BaseUp)
 *
 ******************************************************************/

template <class DataType>
const bool range_partition_impl<DataType>::_is_between(const Key& contDown,
                                                       const Key& contUp) const
{
    w_assert3 (_part_down<=_part_up);
    w_assert3 (contDown<=contUp);
    //    cout << "Checking (" << contDown << " - " << contUp << ") between (" << _down << " - " << _up << ")\n";

    // !!! WARNING !!!
    // The partition boundaries should always be on the left side 
    // because their size can be smaller than the size of the key checked
    return ((_part_down<=contDown) && !(_part_up<contUp));
}




/****************************************************************** 
 *
 * @fn:    resize()
 *
 * @brief: Resizes the assigned range partition to the new limits
 *
 ******************************************************************/

template <class DataType>
const bool range_partition_impl<DataType>::resize(const Key& downLimit, 
                                                  const Key& upLimit) 
{
    if (upLimit<downLimit) {
        TRACE( TRACE_ALWAYS, "Error in new bounds\n");
        return (false);
    }

    // copy new limits
    _part_down.copy(downLimit);
    _part_up.copy(upLimit);
    _rp_state = RPS_SET;

    ostringstream out;
    string sout;
    out << _part_down << "-" << _part_up;
    sout = out.str();

    TRACE( TRACE_DEBUG, "RangePartition (%s-%d) resized (%s)\n", 
           _table->name(), _part_id, sout.c_str());
    return (true);
}



/****************************************************************** 
 *
 * @fn:    verify()
 *
 * @brief: Verifies if action can be enqueued here.
 *
 ******************************************************************/
 
template <class DataType>
const bool range_partition_impl<DataType>::verify(Action& rangeaction)
{
    assert (_rp_state == RPS_SET);
    assert (rangeaction.keys()->size()==2);
    return (_is_between(*rangeaction.keys()->front(), *rangeaction.keys()->back()));
}   



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PARTITION_H */

