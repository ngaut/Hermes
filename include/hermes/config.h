//
// Created by akatsarakis on 15/03/18.
//

#ifndef SPACETIME_CONFIG_H
#define SPACETIME_CONFIG_H
#include "hrd.h"

#define MACHINE_NUM 5
#define REMOTE_MACHINES (MACHINE_NUM - 1)
#define GROUP_MEMBERSHIP_ARRAY_SIZE CEILING(MACHINE_NUM, 8) //assuming uint8_t
#define WORKERS_PER_MACHINE 36
#define KV_SOCKET 0
#define SOCKET_TO_START_SPAWNING_THREADS 0
#define USE_ALL_SOCKETS 1
#define ENABLE_HYPERTHREADING 1
#define BATCH_POST_RECVS_TO_NIC 1
#define WRITE_RATIO 10
#define MAX_BATCH_OPS_SIZE 50// up to 254


//LATENCY
#define INCREASE_TAIL_LATENCY 1
#define INCREASE_TAIL_BY_MS 40
#define NUM_OF_CORES_TO_INCREASE_TAIL WORKERS_PER_MACHINE
#define INCREASE_TAIL_EVERY_X_ACKS 1000
#define MEASURE_LATENCY 0
#define THREAD_MEASURING_LATENCY 6
#define MAX_LATENCY 800 //in us
#define LATENCY_BUCKETS 800
#define LATENCY_PRECISION (MAX_LATENCY / LATENCY_BUCKETS) //latency granularity in us


//DEBUG
#define ENABLE_ASSERTIONS 0
#define DISABLE_VALS_FOR_DEBUGGING 0
#define KEY_NUM 0 //use 0 to disable

//REQUESTS
#define NUM_OF_REP_REQS K_128
#define FEED_FROM_TRACE 0
#define USE_A_SINGLE_KEY 0

/*-------------------------------------------------
----------------- REQ COALESCING -------------------
--------------------------------------------------*/

#define MAX_REQ_COALESCE 15
#define INV_MAX_REQ_COALESCE MAX_REQ_COALESCE
#define ACK_MAX_REQ_COALESCE MAX_REQ_COALESCE
#define VAL_MAX_REQ_COALESCE MAX_REQ_COALESCE

/*-------------------------------------------------
-----------------FLOW CONTROL---------------------
--------------------------------------------------*/
#define CREDITS_PER_REMOTE_WORKER 15
#define INV_CREDITS CREDITS_PER_REMOTE_WORKER
#define ACK_CREDITS CREDITS_PER_REMOTE_WORKER
#define VAL_CREDITS CREDITS_PER_REMOTE_WORKER
#define CRD_CREDITS CREDITS_PER_REMOTE_WORKER

/*-------------------------------------------------
-----------------PCIe BATCHING---------------------
--------------------------------------------------*/
#define MIN_PCIE_BCAST_BATCH 2 //MAX_BATCH_OPS_SIZE
#define MAX_PCIE_BCAST_BATCH MIN(MIN_PCIE_BCAST_BATCH, INV_CREDITS) //Warning! use min to avoid reseting the first req prior batching to the NIC
#define MAX_MSGS_IN_PCIE_BCAST_BATCH (MAX_PCIE_BCAST_BATCH * REMOTE_MACHINES) //must be smaller than the q_depth

/**/
#define MAX_SEND_INV_WRS MAX_MSGS_IN_PCIE_BCAST_BATCH
#define MAX_SEND_ACK_WRS (INV_CREDITS * REMOTE_MACHINES)
#define MAX_SEND_VAL_WRS MAX_MSGS_IN_PCIE_BCAST_BATCH
#define MAX_SEND_CRD_WRS (VAL_CREDITS * REMOTE_MACHINES)

#define MAX_RECV_INV_WRS (INV_CREDITS * REMOTE_MACHINES)
#define MAX_RECV_ACK_WRS (ACK_CREDITS * REMOTE_MACHINES)
#define MAX_RECV_VAL_WRS (VAL_CREDITS * REMOTE_MACHINES)
#define MAX_RECV_CRD_WRS (CRD_CREDITS * REMOTE_MACHINES)
/*-------------------------------------------------
-----------------REQUEST SIZES---------------------
--------------------------------------------------*/
#define INV_RECV_REQ_SIZE (sizeof(ud_req_inv_t)) // Buffer slot size required for a INV request
#define ACK_RECV_REQ_SIZE (sizeof(ud_req_ack_t)) // Buffer slot size required for a ACK request
#define VAL_RECV_REQ_SIZE (sizeof(ud_req_val_t)) // Buffer slot size required for a VAL request
#define CRD_RECV_REQ_SIZE (sizeof(ud_req_crd_t)) // Buffer slot size required for a CRD request

/*-------------------------------------------------
-----------------SELECTIVE SIGNALING---------------
-------------------------------------------------*/
#define INV_SS_GRANULARITY MAX_PCIE_BCAST_BATCH
#define ACK_SS_GRANULARITY MAX_SEND_ACK_WRS
#define VAL_SS_GRANULARITY MAX_PCIE_BCAST_BATCH
#define CRD_SS_GRANULARITY MAX_SEND_CRD_WRS
/*-------------------------------------------------
-----------------QPs & QUEUE DEPTHS----------------
--------------------------------------------------*/
//QPs
#define INV_UD_QP_ID 0
#define ACK_UD_QP_ID 1
#define VAL_UD_QP_ID 2
#define CRD_UD_QP_ID 3
#define TOTAL_WORKER_UD_QPs 4

//RECV Depths
#define RECV_INV_Q_DEPTH MAX_RECV_INV_WRS
#define RECV_ACK_Q_DEPTH MAX_RECV_ACK_WRS
#define RECV_VAL_Q_DEPTH MAX_RECV_VAL_WRS
#define RECV_CRD_Q_DEPTH MAX_RECV_CRD_WRS

//SEND Depths
#define SEND_INV_Q_DEPTH ((INV_SS_GRANULARITY * REMOTE_MACHINES) * 2)
#define SEND_ACK_Q_DEPTH (ACK_SS_GRANULARITY * 2)
#define SEND_VAL_Q_DEPTH ((VAL_SS_GRANULARITY * REMOTE_MACHINES) * 2)
#define SEND_CRD_Q_DEPTH (CRD_SS_GRANULARITY * 2)


#define DGRAM_BUFF_SIZE ((INV_RECV_REQ_SIZE * RECV_INV_Q_DEPTH) + \
                         (ACK_RECV_REQ_SIZE * RECV_ACK_Q_DEPTH) + \
                         (VAL_RECV_REQ_SIZE * RECV_VAL_Q_DEPTH) + \
                         (64))  //CREDITS are header-only (inlined)


/*-------------------------------------------------
----------------- REQ INLINING --------------------
--------------------------------------------------*/
#define DISABLE_INLINING 0
#define DISABLE_INV_INLINING ((DISABLE_INLINING || sizeof(spacetime_inv_packet_t) >= 188) ? 1 : 0)
#define DISABLE_ACK_INLINING ((DISABLE_INLINING || sizeof(spacetime_ack_packet_t) >= 188) ? 1 : 0)
#define DISABLE_VAL_INLINING ((DISABLE_INLINING || sizeof(spacetime_val_packet_t) >= 188) ? 1 : 0)

/*-------------------------------------------------
----------------- SEND/RECV OPS SIZE --------------
--------------------------------------------------*/
#define INV_SEND_OPS_SIZE (DISABLE_INLINING == 1 ? 2 * MAX_SEND_INV_WRS : MAX_SEND_INV_WRS)
#define ACK_SEND_OPS_SIZE (DISABLE_INLINING == 1 ? 2 * MAX_SEND_ACK_WRS : MAX_SEND_ACK_WRS)
#define VAL_SEND_OPS_SIZE (DISABLE_INLINING == 1 ? 2 * MAX_SEND_VAL_WRS : MAX_SEND_VAL_WRS)

#define INV_RECV_OPS_SIZE (MAX_RECV_INV_WRS * INV_MAX_REQ_COALESCE)
#define ACK_RECV_OPS_SIZE (MAX_RECV_ACK_WRS * ACK_MAX_REQ_COALESCE)
#define VAL_RECV_OPS_SIZE (MAX_RECV_VAL_WRS * VAL_MAX_REQ_COALESCE)

/*-------------------------------------------------
-----------------PRINTS (DBG)---------------------
--------------------------------------------------*/
///Warning some prints assume that there are no faults (multiplications with REMOTE_MACHINES)
#define MAX_THREADS_TO_PRINT 1
#define ENABLE_SS_PRINTS 0
#define ENABLE_REQ_PRINTS 0
#define ENABLE_SEND_PRINTS 0
#define ENABLE_RECV_PRINTS 0
#define ENABLE_CREDIT_PRINTS 0
#define ENABLE_POST_RECV_PRINTS 0
#define ENABLE_BATCH_OP_PRINTS 0
#define ENABLE_INV_PRINTS 0
#define ENABLE_ACK_PRINTS 0
#define ENABLE_VAL_PRINTS 0
#define ENABLE_CRD_PRINTS 0

//Stats prints
#define PRINT_STATS_EVERY_MSECS 1000
#define PRINT_WORKER_STATS 0

//Stats
#define DUMP_STATS_2_FILE 0 //This is not working at the moment
#define EXIT_ON_STATS_PRINT 1
#define PRINT_NUM_STATS_BEFORE_EXITING 30
#define ENABLE_STAT_COUNTING 1

//FAKE NODE FAILURE
#define FAKE_FAILURE 0
#define NODE_TO_FAIL 3
#define ROUNDS_BEFORE_FAILURE 20

//FAILURE DETECTION
#define NODES_WITH_FAILURE_DETECTOR 1
#define WORKER_EMULATING_FAILURE_DETECTOR 4
#define NUM_OF_IDLE_ITERS_FOR_SUSPICION K_128 //M_2 //K_256 //M_1

// Rarely change
#define WORKER_NUM (MACHINE_NUM * WORKERS_PER_MACHINE)
#define TOTAL_THREADS_PER_CORE 2
#define TOTAL_CORES_PER_SOCKET 10
#define TOTAL_NUMBER_OF_SOCKETS 2
#define TOTAL_HW_CORES (TOTAL_THREADS_PER_CORE * TOTAL_CORES_PER_SOCKET * TOTAL_NUMBER_OF_SOCKETS)
#define BASE_SHM_KEY 24
#define WORKER_SL 0 // service level for the workers

///Debug
//#define SPACETIME DEBUG 2
#ifndef SPACETIME_DEBUG
# define SPACETIME_DEBUG 0
#endif

struct thread_params {
    int id;
};

/* ah pointer and qpn are accessed together in the critical path
   so we are putting them in the same cache line */
struct remote_qp {
    struct ibv_ah *ah;
    int qpn;
    // no padding needed- false sharing is not an issue, only fragmentation
};

struct latency_counters{
    uint32_t read_reqs[LATENCY_BUCKETS + 1];
    uint32_t write_reqs[LATENCY_BUCKETS + 1];
    int max_read_latency;
    int max_write_latency;
    long long total_measurements;
};

extern struct latency_counters latency_count;
extern volatile struct remote_qp remote_worker_qps[WORKER_NUM][TOTAL_WORKER_UD_QPs];
extern volatile uint8_t node_suspicions[WORKERS_PER_MACHINE][MACHINE_NUM];
extern volatile char worker_needed_ah_ready;
#endif //SPACETIME_CONFIG_H
