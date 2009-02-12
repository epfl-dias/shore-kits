/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_tm1_env.cpp
 *
 *  @brief:  Implementation of the various test clients (Baseline, DORA, etc...) for the TM1 benchmark
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "workload/tm1/shore_tm1_client.h"


ENTER_NAMESPACE(tm1);



/********************************************************************* 
 *
 *  baseline_tm1_client_t
 *
 *********************************************************************/

const int baseline_tm1_client_t::load_sup_xct(mapSupTrxs& stmap)
{
    // clears the supported trx map and loads its own
    stmap.clear();

    // Baseline TM1 trxs
    stmap[XCT_TM1_MIX]             = "TM1-Mix";
    stmap[XCT_TM1_GET_SUB_DATA]    = "TM1-GetSubData";
    stmap[XCT_TM1_GET_NEW_DEST]    = "TM1-GetNewDest";
    stmap[XCT_TM1_GET_ACC_DATA]    = "TM1-GetAccData";
    stmap[XCT_TM1_UPD_SUB_DATA]    = "TM1-UpdSubData";
    stmap[XCT_TM1_UPD_LOCATION]    = "TM1-UpdLocation";
    stmap[XCT_TM1_INS_CALL_FWD]    = "TM1-InsCallFwd";
    stmap[XCT_TM1_DEL_CALL_FWD]    = "TM1-DelCallFwd";
    return (stmap.size());
}


/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: Baseline client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/
 
w_rc_t baseline_tm1_client_t::run_one_xct(int xct_type, int xctid) 
{
    // 1. Initiate and detach from transaction
    tid_t atid;   
    W_DO(_tm1db->db()->begin_xct(atid));
    xct_t* pxct = smthread_t::me()->xct();
    w_assert3 (pxct);
    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);
    detach_xct(pxct);
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);
    
    // 2. Set input
    trx_result_tuple_t atrt;
    if (_cp->take_one) {
        TRACE( TRACE_TRX_FLOW, "Sleeping on (%d)\n", atid);
        atrt.set_notify(_cp->wait+_cp->index);
    }

    // pick a valid ID
    int selid = _selid;
    if (_selid==0) 
        selid = URand(1,_qf*TM1_SUBS_PER_SF); 

    // 3. Get one action from the trash stack
    assert (_tm1db);
    trx_request_t* arequest = new (_tm1db->_request_pool) trx_request_t;
    arequest->set(pxct,atid,xctid,atrt,xct_type,selid);

    // 4. enqueue to worker thread
    assert (_worker);
    _worker->enqueue(arequest);
    return (RCOK);
}





// /********************************************************************* 
//  *
//  *  dora_tm1_client_t
//  *
//   *********************************************************************/

// const int dora_tm1_client_t::load_sup_xct(mapSupTrxs& stmap)
// {
//     // clears the supported trx map and loads its own
//     stmap.clear();

//     // Baseline TPC-C trxs
//     stmap[XCT_DORA_MIX]          = "DORA-Mix";
//     stmap[XCT_DORA_NEW_ORDER]    = "DORA-NewOrder";
//     stmap[XCT_DORA_PAYMENT]      = "DORA-Payment";
//     stmap[XCT_DORA_ORDER_STATUS] = "DORA-OrderStatus";
//     stmap[XCT_DORA_DELIVERY]     = "DORA-Delivery";
//     stmap[XCT_DORA_STOCK_LEVEL]  = "DORA-StockLevel";

//     // Microbenchmarks
//     stmap[XCT_DORA_MBENCH_WH]    = "DORA-MBench-WHs";
//     stmap[XCT_DORA_MBENCH_CUST]  = "DORA-MBench-CUSTs";

//     return (stmap.size());
// }

// /********************************************************************* 
//  *
//  *  @fn:    run_one_xct
//  *
//  *  @brief: DORA client - Entry point for running one trx 
//  *
//  *  @note:  The execution of this trx will not be stopped even if the
//  *          measure internal has expired.
//  *
//  *********************************************************************/
 
// w_rc_t dora_tm1_client_t::run_one_xct(int xct_type, int xctid) 
// {
//     // if DORA TPC-C MIX
//     if (xct_type == XCT_DORA_MIX) {        
//         xct_type = XCT_DORA_MIX + random_xct_type(rand(100));
//     }

//     // pick a valid wh id
//     int whid = _wh;
//     if (_wh==0) 
//         whid = URand(1,_qf); 

//     trx_result_tuple_t atrt;
//     if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    
//     switch (xct_type) {

//         // TPC-C DORA
//     case XCT_DORA_NEW_ORDER:
//         return (_tm1db->dora_new_order(xctid,atrt,whid));
//     case XCT_DORA_PAYMENT:
//         return (_tm1db->dora_payment(xctid,atrt,whid));
//     case XCT_DORA_ORDER_STATUS:
//         return (_tm1db->dora_order_status(xctid,atrt,whid));
//     case XCT_DORA_DELIVERY:
//         return (_tm1db->dora_delivery(xctid,atrt,whid));
//     case XCT_DORA_STOCK_LEVEL:
//         return (_tm1db->dora_stock_level(xctid,atrt,whid));

//         // MBENCH DORA
//     case XCT_DORA_MBENCH_WH:
//         return (_tm1db->dora_mbench_wh(xctid,atrt,whid));
//     case XCT_DORA_MBENCH_CUST:
//         return (_tm1db->dora_mbench_cust(xctid,atrt,whid));

//     default:
//         assert (0); // UNKNOWN TRX-ID
//     }
//     return (RCOK);
// }



EXIT_NAMESPACE(tm1);


