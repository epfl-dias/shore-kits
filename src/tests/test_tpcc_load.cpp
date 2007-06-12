/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file load_tpcc_tables.cpp
 *
 *  @brief Test loading TPC-C tables
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <unistd.h>
#include <sys/time.h>
#include <cmath>

#include "util.h"
#include "tests/common.h"
#include "workload/common/table_load.h"


using namespace workload;

int main() {

    thread_init();
    
    /* NGM: Removed DB_TRUNCATE flag since it was causing an exception
       to be thrown in table open. As a temporary fix, we will simply
       delete the old database file before rebuilding. */

    db_open(DB_CREATE|DB_THREAD); 
    db_tpcc_load("tpcc_tbl");
    db_close();

    return 0;
}
