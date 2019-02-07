#include <spacetime.h>
#include <concur_ctrl.h>
#include <time.h>
#include "util.h"
#include "inline-util.h"

///
#include "time_rdtsc.h"
#include "../../include/aether/aether.h"
///

int
inv_skip_or_get_sender_id(uint8_t *req)
{
	spacetime_op_t* op_req = (spacetime_op_t *) req;

	if(ENABLE_ASSERTIONS){
		assert(is_response_code(op_req->op_meta.state) || is_bucket_state_code(op_req->op_meta.state));
		assert(is_input_code(op_req->op_meta.opcode));
	}

	if(op_req->op_meta.state != ST_PUT_SUCCESS &&
	   op_req->op_meta.state != ST_REPLAY_SUCCESS &&
	   op_req->op_meta.state != ST_OP_MEMBERSHIP_CHANGE)
		return -1;
	return 0; // since inv is a bcast we can return any int other than -1
}

void
inv_modify_elem_after_send(uint8_t* req)
{
	spacetime_op_t* op_req = (spacetime_op_t *) req;

	if(op_req->op_meta.state == ST_PUT_SUCCESS)
		op_req->op_meta.state = ST_IN_PROGRESS_PUT;
	else if(op_req->op_meta.state == ST_REPLAY_SUCCESS)
	    op_req->op_meta.state = ST_IN_PROGRESS_REPLAY;
	else if(op_req->op_meta.state == ST_OP_MEMBERSHIP_CHANGE)
	    op_req->op_meta.state = ST_OP_MEMBERSHIP_COMPLETE;
	else{
		printf("state: %d\n", op_req->op_meta.state);
		printf("state: %s\n", code_to_str(op_req->op_meta.state));
		assert(0);
	}
}

void
inv_copy_and_modify_elem(uint8_t* msg_to_send, uint8_t* triggering_req)
{
	spacetime_op_t* op = (spacetime_op_t *) triggering_req;
	spacetime_inv_t* inv_to_send = (spacetime_inv_t *) msg_to_send;

	// Copy op to inv, set sender and opcode
	memcpy(inv_to_send, op, sizeof(spacetime_inv_t));
	inv_to_send->op_meta.sender = (uint8_t) machine_id;
	inv_to_send->op_meta.opcode = ST_OP_INV;
//	//TODO change to include membership change
//	inv_to_send->op_meta.opcode = (uint8_t) (op->op_meta.state == ST_OP_MEMBERSHIP_CHANGE ?
//											 ST_OP_MEMBERSHIP_CHANGE : ST_OP_INV);
}


int
ack_skip_or_get_sender_id(uint8_t *req)
{
	spacetime_inv_t* inv_req = (spacetime_inv_t *) req;

	if(ENABLE_ASSERTIONS)
		assert(inv_req->op_meta.opcode == ST_INV_SUCCESS || inv_req->op_meta.opcode == ST_EMPTY);

	return inv_req->op_meta.opcode == ST_EMPTY ? -1 : inv_req->op_meta.sender;
}

void
ack_modify_elem_after_send(uint8_t* req)
{
	spacetime_inv_t* inv_req = (spacetime_inv_t *) req;

	//empty inv buffer
	if(inv_req->op_meta.opcode == ST_INV_SUCCESS ||
	   inv_req->op_meta.opcode == ST_OP_MEMBERSHIP_CHANGE)
		inv_req->op_meta.opcode = ST_EMPTY;
	else assert(0);
}


void
ack_copy_and_modify_elem(uint8_t* msg_to_send, uint8_t* triggering_req)
{
	spacetime_ack_t* ack_to_send = (spacetime_ack_t *) msg_to_send;
	memcpy(ack_to_send, triggering_req, sizeof(spacetime_ack_t)); // copy req to next_req_ptr
	ack_to_send->sender = (uint8_t) machine_id;
	ack_to_send->opcode = ST_OP_ACK;
}


int
val_skip_or_get_sender_id(uint8_t *req)
{
	spacetime_ack_t* ack_req = (spacetime_ack_t *) req;
	if (ack_req->opcode == ST_ACK_SUCCESS || ack_req->opcode == ST_OP_MEMBERSHIP_CHANGE) {
		ack_req->opcode = ST_EMPTY;
		return -1;
	} else if (ack_req->opcode == ST_EMPTY)
		return -1;

	if(ENABLE_ASSERTIONS)
		assert(ack_req->opcode == ST_LAST_ACK_SUCCESS);

	return ack_req->sender;
}

void val_modify_elem_after_send(uint8_t* req) {
	spacetime_ack_t* ack_req = (spacetime_ack_t *) req;

	if(ENABLE_ASSERTIONS)
		assert(ack_req->opcode == ST_LAST_ACK_SUCCESS);

	ack_req->opcode = ST_EMPTY;
}

void
val_copy_and_modify_elem(uint8_t* msg_to_send, uint8_t* triggering_req)
{
	spacetime_val_t* val_to_send = (spacetime_val_t *) msg_to_send;

	memcpy(val_to_send, triggering_req, sizeof(spacetime_val_t)); // copy req to next_req_ptr
	val_to_send->opcode = ST_OP_VAL;
	val_to_send->sender = (uint8_t) machine_id;
}


int
crd_skip_or_get_sender_id(uint8_t *req)
{
	spacetime_val_t* val_ptr = (spacetime_val_t *) req;

	if(ENABLE_ASSERTIONS)
		assert(val_ptr->opcode == ST_VAL_SUCCESS || val_ptr->opcode == ST_EMPTY);

	return val_ptr->opcode == ST_EMPTY ? -1 : val_ptr->sender;
}

void
crd_modify_elem_after_send(uint8_t* req)
{
	spacetime_val_t* val_req = (spacetime_val_t *) req;

	//empty inv buffer
	if(val_req->opcode == ST_VAL_SUCCESS)
		val_req->opcode = ST_EMPTY;
	else assert(0);
}


void
channel_assertions(ud_channel_t *inv_ud_c, ud_channel_t *ack_ud_c,
				   ud_channel_t *val_ud_c, ud_channel_t *crd_ud_c)
{
	assert(inv_ud_c->max_send_wrs == MAX_SEND_INV_WRS);
	assert(inv_ud_c->max_recv_wrs == MAX_RECV_INV_WRS);
	assert(inv_ud_c->send_q_depth == SEND_INV_Q_DEPTH);
	assert(inv_ud_c->recv_q_depth == RECV_INV_Q_DEPTH);
	assert(inv_ud_c->send_pkt_buff_len == INV_SEND_OPS_SIZE);
	assert(inv_ud_c->max_coalescing == INV_MAX_REQ_COALESCE);
	assert(inv_ud_c->ss_granularity == INV_SS_GRANULARITY);
	assert(inv_ud_c->max_pcie_bcast_batch == MAX_PCIE_BCAST_BATCH);

	assert(ack_ud_c->max_send_wrs == MAX_SEND_ACK_WRS);
	assert(ack_ud_c->max_recv_wrs == MAX_RECV_ACK_WRS);
	assert(ack_ud_c->send_q_depth == SEND_ACK_Q_DEPTH);
	assert(ack_ud_c->recv_q_depth == RECV_ACK_Q_DEPTH);
	assert(ack_ud_c->send_pkt_buff_len == ACK_SEND_OPS_SIZE);
	assert(ack_ud_c->max_coalescing == ACK_MAX_REQ_COALESCE);
	assert(ack_ud_c->ss_granularity == ACK_SS_GRANULARITY);

	assert(val_ud_c->max_send_wrs == MAX_SEND_VAL_WRS);
	assert(val_ud_c->max_recv_wrs == MAX_RECV_VAL_WRS);
	assert(val_ud_c->send_q_depth == SEND_VAL_Q_DEPTH);
	assert(val_ud_c->recv_q_depth == RECV_VAL_Q_DEPTH);
	assert(val_ud_c->send_pkt_buff_len == VAL_SEND_OPS_SIZE);
	assert(val_ud_c->max_coalescing == VAL_MAX_REQ_COALESCE);
	assert(val_ud_c->ss_granularity == VAL_SS_GRANULARITY);
	assert(val_ud_c->max_pcie_bcast_batch == MAX_PCIE_BCAST_BATCH);

	assert(crd_ud_c->max_send_wrs == MAX_SEND_CRD_WRS);
	assert(crd_ud_c->max_recv_wrs == MAX_RECV_CRD_WRS);
	assert(crd_ud_c->send_q_depth == SEND_CRD_Q_DEPTH);
	assert(crd_ud_c->recv_q_depth == RECV_CRD_Q_DEPTH);
	assert(crd_ud_c->ss_granularity == CRD_SS_GRANULARITY);
}

void
print_total_send_recv_msgs(ud_channel_t *inv_ud_c, ud_channel_t *ack_ud_c,
						   ud_channel_t *val_ud_c, ud_channel_t *crd_ud_c)
{
	green_printf ("Total Send: invs %d, acks %d, vals %d, crds %d\n",
				  inv_ud_c->stats.send_total_msgs, ack_ud_c->stats.send_total_msgs,
				  val_ud_c->stats.send_total_msgs, crd_ud_c->stats.send_total_msgs);
	green_printf ("Total Recv: invs %d, acks %d, vals %d, crds %d\n",
				  inv_ud_c->stats.recv_total_msgs, ack_ud_c->stats.recv_total_msgs,
				  val_ud_c->stats.recv_total_msgs, crd_ud_c->stats.recv_total_msgs);
}

dbit_vector_t *barrier;
void*
run_worker(void *arg)
{
	struct thread_params params = *(struct thread_params *) arg;
	uint16_t worker_lid = (uint16_t) params.id;	// Local ID of this worker thread
	uint16_t worker_gid = (uint16_t) (machine_id * WORKERS_PER_MACHINE + params.id);	// Global ID of this worker thread



	/* --------------------------------------------------------
	------------------- RDMA AETHER DECLARATIONS---------------
	---------------------------------------------------------*/
	ud_channel_t ud_channels[TOTAL_WORKER_UD_QPs];
	ud_channel_t* ud_channel_ptrs[TOTAL_WORKER_UD_QPs];
	ud_channel_t* inv_ud_c = &ud_channels[INV_UD_QP_ID];
	ud_channel_t* ack_ud_c = &ud_channels[ACK_UD_QP_ID];
	ud_channel_t* val_ud_c = &ud_channels[VAL_UD_QP_ID];
	ud_channel_t* crd_ud_c = &ud_channels[CRD_UD_QP_ID];

	for(int i = 0; i < TOTAL_WORKER_UD_QPs; ++i)
		ud_channel_ptrs[i] = &ud_channels[i];

	if(worker_lid == 0)
		dbv_init(&barrier, WORKERS_PER_MACHINE);

	char inv_qp_name[200], ack_qp_name[200], val_qp_name[200];
	sprintf(inv_qp_name, "%s%d", "\033[31mINV\033[0m", worker_lid);
	sprintf(ack_qp_name, "%s%d", "\033[33mACK\033[0m", worker_lid);
	sprintf(val_qp_name, "%s%d", "\033[1m\033[32mVAL\033[0m", worker_lid);

	aether_ud_channel_init(inv_ud_c, inv_qp_name, REQ, INV_MAX_REQ_COALESCE, sizeof(spacetime_inv_t),
						   DISABLE_INV_INLINING == 0 ? 1 : 0, 1, 0, ack_ud_c, INV_CREDITS, MACHINE_NUM, 1, 1);
	aether_ud_channel_init(ack_ud_c, ack_qp_name, RESP, ACK_MAX_REQ_COALESCE, sizeof(spacetime_ack_t),
						   DISABLE_ACK_INLINING == 0 ? 1 : 0, 0, 0, inv_ud_c, ACK_CREDITS, MACHINE_NUM, 1, 1);
	aether_ud_channel_init(val_ud_c, val_qp_name, REQ, VAL_MAX_REQ_COALESCE, sizeof(spacetime_val_t),
						   DISABLE_VAL_INLINING == 0 ? 1 : 0, 1, 1, crd_ud_c, VAL_CREDITS, MACHINE_NUM, 1, 1);

	aether_setup_channel_qps_and_recvs(ud_channel_ptrs, TOTAL_WORKER_UD_QPs, barrier, worker_lid);

	channel_assertions(inv_ud_c, ack_ud_c, val_ud_c, crd_ud_c);



	/* -------------------------------------------------------
	------------------- OTHER DECLARATIONS--------------------
	---------------------------------------------------------*/
	//Intermediate buffs where reqs are copied from incoming_* buffs in order to get passed to the KVS
	spacetime_op_t  *ops;
	spacetime_inv_t *inv_recv_ops;
	spacetime_ack_t *ack_recv_ops;
	spacetime_val_t *val_recv_ops;

	setup_kvs_buffs(&ops, &inv_recv_ops, &ack_recv_ops, &val_recv_ops);

	spacetime_group_membership last_group_membership = group_membership;

	struct spacetime_trace_command *trace;
	trace_init(&trace, worker_gid);

	////
	spacetime_op_t* n_hottest_keys_in_ops_get[COALESCE_N_HOTTEST_KEYS];
	spacetime_op_t* n_hottest_keys_in_ops_put[COALESCE_N_HOTTEST_KEYS];
	for(int i = 0; i < COALESCE_N_HOTTEST_KEYS; ++i){
		n_hottest_keys_in_ops_get[i] = NULL;
		n_hottest_keys_in_ops_put[i] = NULL;
	}
	////

	int node_suspected = -1;
	uint32_t trace_iter = 0;
	uint16_t rolling_inv_index = 0;
	uint16_t invs_polled = 0, acks_polled = 0, vals_polled = 0;
	uint32_t num_of_iters_serving_op[MAX_BATCH_OPS_SIZE] = {0};
	uint8_t has_outstanding_vals = 0, has_outstanding_vals_from_memb_change = 0;

	/// Spawn stats thread
	if (worker_lid == 0)
		if (spawn_stats_thread() != 0) red_printf("Stats thread was not successfully spawned \n");

	/* -----------------------------------------------------
       ------------------------Main Loop--------------------
	   ----------------------------------------------------- */
	while (true) {

	    if(unlikely(w_stats[worker_lid].total_loops % M_16 == 0)){
	        //Check something periodically
//			print_total_send_recv_msgs(&inv_ud_c, &ack_ud_c, &val_ud_c, &crd_ud_c);
//	        uint8_t remote_node = (uint8_t) (machine_id == 0 ? 1 : 0);
//	        printf("Inv credits: %d, ack credits: %d\n",
//	        		inv_ud_c.credits_per_channels[remote_node],
//	        		ack_ud_c.credits_per_channels[remote_node]);
//	        for(int i = 0; i < MAX_BATCH_OPS_SIZE; ++i)
//				printf("ops[%d]: state-> %s\n", i, code_to_str(ops[i].op_meta.state));
	    }

		node_suspected = refill_ops_n_suspect_failed_nodes(&trace_iter, worker_lid, trace, ops,
														   num_of_iters_serving_op, last_group_membership,
														   n_hottest_keys_in_ops_get, n_hottest_keys_in_ops_put);

		batch_ops_to_KVS(MAX_BATCH_OPS_SIZE, &ops, worker_lid, last_group_membership);

		if (WRITE_RATIO > 0) {
			///~~~~~~~~~~~~~~~~~~~~~~INVS~~~~~~~~~~~~~~~~~~~~~~~~~~~
			aether_issue_pkts(inv_ud_c, (uint8_t *) ops,
							  MAX_BATCH_OPS_SIZE, sizeof(spacetime_op_t), &rolling_inv_index,
							  inv_skip_or_get_sender_id, inv_modify_elem_after_send, inv_copy_and_modify_elem);

			///Poll for INVs
			invs_polled = aether_poll_buff_and_post_recvs(inv_ud_c, INV_RECV_OPS_SIZE, (uint8_t *) inv_recv_ops);


			if(invs_polled > 0) {
				batch_invs_to_KVS(invs_polled, &inv_recv_ops, ops, worker_lid,
								  &node_suspected, num_of_iters_serving_op);

				///~~~~~~~~~~~~~~~~~~~~~~ACKS~~~~~~~~~~~~~~~~~~~~~~~~~~~
				aether_issue_pkts(ack_ud_c, (uint8_t *) inv_recv_ops,
								  invs_polled, sizeof(spacetime_inv_t), NULL,
								  ack_skip_or_get_sender_id, ack_modify_elem_after_send, ack_copy_and_modify_elem);
				invs_polled = 0;
				if(ENABLE_ASSERTIONS)
					assert(inv_ud_c->stats.recv_total_msgs == ack_ud_c->stats.send_total_msgs);
			}

			if(has_outstanding_vals == 0 && has_outstanding_vals_from_memb_change == 0) {
				///Poll for Acks
				acks_polled = aether_poll_buff_and_post_recvs(ack_ud_c, ACK_RECV_OPS_SIZE, (uint8_t *) ack_recv_ops);

				if (acks_polled > 0) {
					batch_acks_to_KVS(acks_polled, &ack_recv_ops, ops, last_group_membership, worker_lid);
					acks_polled = 0;
				}
			}

			if(!DISABLE_VALS_FOR_DEBUGGING) {
				///~~~~~~~~~~~~~~~~~~~~~~ VALs ~~~~~~~~~~~~~~~~~~~~~~~~~~~
				has_outstanding_vals = aether_issue_pkts(val_ud_c, (uint8_t *) ack_recv_ops,
														 ack_ud_c->recv_pkt_buff_len, sizeof(spacetime_ack_t),
														 NULL, val_skip_or_get_sender_id,
														 val_modify_elem_after_send, val_copy_and_modify_elem);

				///Poll for Vals
				vals_polled = aether_poll_buff_and_post_recvs(val_ud_c, VAL_RECV_OPS_SIZE, (uint8_t *) val_recv_ops);

				if (vals_polled > 0) {
					batch_vals_to_KVS(vals_polled, &val_recv_ops, ops, worker_lid);

					///~~~~~~~~~~~~~~~~~~~~~~CREDITS~~~~~~~~~~~~~~~~~~~~~~~~~~~
					aether_issue_credits(crd_ud_c, (uint8_t *) val_recv_ops, VAL_RECV_OPS_SIZE,
										 sizeof(spacetime_val_t), crd_skip_or_get_sender_id, crd_modify_elem_after_send);
					vals_polled = 0;
				}
			}

//            ///Emulating a perfect failure detector via a group membership
//			if (unlikely(node_suspected >= 0 && worker_lid == WORKER_EMULATING_FAILURE_DETECTOR))
//				follower_removal(node_suspected);
//
//
//			if(group_membership_has_changed(&last_group_membership, worker_lid)) {
//			    printf("Reconfiguring group membership\n");
//				assert(0);
//			}
		}
		w_stats[worker_lid].total_loops++;
	}
	return NULL;
}

