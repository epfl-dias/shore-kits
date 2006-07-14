/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : job_directory.cpp
 *  @brief   : Implementation of a generic job (coded query) directory
 *  @version : 0.1
 *  @history :
 7/7/2006: Initial version
*/

#include "workload/job_directory.h"
#include "engine/util/static_hash_map.h"
#include "engine/util/hash_functions.h"
#include "engine/util/guard.h"
#include "workload/common/copy_string.h"


// include me last!!!
#include "engine/namespace.h"



/* helper functions */
size_t cmd_hash(const void* key);
int string_comparator(const void* key1, const void* key2);
    
///////////////////
// class job_directory
//


job_directory* job_directory::jobDirInstance = NULL;
pthread_mutex_t job_directory::instance_mutex = PTHREAD_MUTEX_INITIALIZER;



/** @fn    : instance()
 *  @brief : Returns the unique instance of the job repository
 */

job_directory* job_directory::instance() {

    // first check without concern of locking
    if ( jobDirInstance == NULL ) {

        // instance does not exist yet
        pthread_mutex_lock(&instance_mutex);

        // we have the lock but another thread may have gotten the lock first
        if (jobDirInstance == NULL) {
            jobDirInstance = new job_directory();
        }

        // release instance mutex
        pthread_mutex_unlock(&instance_mutex);
    }

    return(jobDirInstance);
}




/** @fn     : Constructor
 *  @brief  : Initializes the static hash map
 */

job_directory::job_directory() {

    
    /* initialize the stats */
    pthread_mutex_init(&stats_mutex, NULL);
    pthread_mutex_lock(&stats_mutex);
    jobCounter = 0;
    pthread_mutex_unlock(&stats_mutex);
    
    /* initialize the static hash map */
    static_hash_map_init( &jobs_directory, &jobs_directory_buckets[0],
                          JOB_DIRECTORY_HASH_MAP_BUCKETS,
                          cmd_hash, string_comparator);
}



/** @fn     : Destructor
 *  @brief  : Removes the allocated variables
 */

job_directory::~job_directory() {

    TRACE(TRACE_DEBUG, "job_directory destructor\n");
}



   
/** @fn     : int _register_job_driver(const char*, job_driver_t*)
 *  @brief  : Register the job driver
 *  @return : 0 on success, 1 otherwise
 */
 
int job_directory::_register_job_driver(const c_str &sJobCmd, job_driver_t* aJobDriver)
{    
    TRACE(TRACE_DEBUG, "Registering Job Driver: %s\n", sJobCmd.data());
    
    /* check for duplicates */
    if ( !static_hash_map_find( &jobs_directory, sJobCmd.data(), NULL, NULL ) ) {
        TRACE(TRACE_ALWAYS, "Trying to register duplicate job for command %s\n", sJobCmd.data() );
        QPIPE_PANIC();
    }
    
    /* allocate hash node and copy of key string */
    static_hash_node_t node = (static_hash_node_t)malloc(sizeof(*node));
    if ( node == NULL ) {
        TRACE(TRACE_ALWAYS, "malloc() failed on static_hash_node_t\n");
        QPIPE_PANIC();
    }
    
    /* add to hash map */
    static_hash_map_insert( &jobs_directory, copy_string(sJobCmd.data()), aJobDriver, node );
  
    return (0);
}



/** @fn    : int _print_info()
 *  @brief : Displays info about the registered jobs
 */

void job_directory::_print_info() {

    static_hash_node_s node;
    job_driver_t* jd = NULL;
    
    for (int i=0; i < JOB_DIRECTORY_HASH_MAP_BUCKETS; i++) {
        node = jobs_directory_buckets[i];
        jd = (job_driver_t*)node.value;
        jd->print_info();
    }
}




/** @fn     : job_driver_t* get_job_driver(const char*)
 *  @brief  : Returns the corresponding job driver
 *  @return : The job_driver_t if found, NULL otherwise
 */

job_driver_t* job_directory::_get_job_driver(const c_str &aJobCmd) {
    
    TRACE(TRACE_DEBUG, "Searching Job Driver: [%s]\n", aJobCmd.data());
    
    void* jd;

    if (static_hash_map_find(&jobs_directory, aJobCmd.data(), &jd, NULL)) {
        TRACE( TRACE_ALWAYS, "No job: %s exists\n", aJobCmd.data());
        return (NULL);
    }

    return ((job_driver_t*)jd);
}




/* static wrappers */
int job_directory::register_job_driver(const char* sJobCmd, job_driver_t* aJobDriver) {

    return (instance()->_register_job_driver(sJobCmd, aJobDriver));
}


void job_directory::print_info() {

    instance()->_print_info();
}


job_driver_t* job_directory::get_job_driver(const char* aJobCmd) {
    
    return (instance()->_get_job_driver(aJobCmd));
}





/* definitions of internal helper functions */
 
/** @fn    : int string_comparator(const void*, const void*)
 *  @brief : String comparison function that can be used as argument
 *           in the hash_map structure
 */
 
int string_comparator(const void* key1, const void* key2) {
  const char* str1 = (const char*)key1;
  const char* str2 = (const char*)key2;
  return strcmp(str1, str2);
}



/** @fn    : size_t cmd_hash(const void*)
 *  @brief : Hashing a string function that can be used as argument
 *            in the hash_map stucture
 */
 
size_t cmd_hash(const void* key) {
  const char* str = (const char*)key;
  return (size_t)RSHash(str, strlen(str));
}
 


#include "engine/namespace.h"
