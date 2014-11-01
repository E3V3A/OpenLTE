#line 2 "LTE_fdd_enb_rrc.cc" // Make __FILE__ omit the path
/*******************************************************************************

    Copyright 2013-2014 Ben Wojtowicz

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************

    File: LTE_fdd_enb_rrc.cc

    Description: Contains all the implementations for the LTE FDD eNodeB
                 radio resource control layer.

    Revision History
    ----------    -------------    --------------------------------------------
    11/10/2013    Ben Wojtowicz    Created file
    01/18/2014    Ben Wojtowicz    Added level to debug prints.
    05/04/2014    Ben Wojtowicz    Added PDCP communication and UL CCCH state
                                   machine.
    06/15/2014    Ben Wojtowicz    Added UL DCCH message handling and MME NAS
                                   message handling.
    08/03/2014    Ben Wojtowicz    Added downlink NAS message handling and
                                   connection release.
    11/01/2014    Ben Wojtowicz    Added RRC connection reconfiguration and
                                   security mode command messages.

*******************************************************************************/

/*******************************************************************************
                              INCLUDES
*******************************************************************************/

#include "LTE_fdd_enb_rrc.h"
#include "LTE_fdd_enb_pdcp.h"
#include "LTE_fdd_enb_interface.h"

/*******************************************************************************
                              DEFINES
*******************************************************************************/


/*******************************************************************************
                              TYPEDEFS
*******************************************************************************/


/*******************************************************************************
                              GLOBAL VARIABLES
*******************************************************************************/

LTE_fdd_enb_rrc* LTE_fdd_enb_rrc::instance = NULL;
boost::mutex     rrc_instance_mutex;

/*******************************************************************************
                              CLASS IMPLEMENTATIONS
*******************************************************************************/

/*******************/
/*    Singleton    */
/*******************/
LTE_fdd_enb_rrc* LTE_fdd_enb_rrc::get_instance(void)
{
    boost::mutex::scoped_lock lock(rrc_instance_mutex);

    if(NULL == instance)
    {
        instance = new LTE_fdd_enb_rrc();
    }

    return(instance);
}
void LTE_fdd_enb_rrc::cleanup(void)
{
    boost::mutex::scoped_lock lock(rrc_instance_mutex);

    if(NULL != instance)
    {
        delete instance;
        instance = NULL;
    }
}

/********************************/
/*    Constructor/Destructor    */
/********************************/
LTE_fdd_enb_rrc::LTE_fdd_enb_rrc()
{
    started = false;
}
LTE_fdd_enb_rrc::~LTE_fdd_enb_rrc()
{
    stop();
}

/********************/
/*    Start/Stop    */
/********************/
void LTE_fdd_enb_rrc::start(void)
{
    boost::mutex::scoped_lock lock(start_mutex);
    LTE_fdd_enb_msgq_cb       pdcp_cb(&LTE_fdd_enb_msgq_cb_wrapper<LTE_fdd_enb_rrc, &LTE_fdd_enb_rrc::handle_pdcp_msg>, this);
    LTE_fdd_enb_msgq_cb       mme_cb(&LTE_fdd_enb_msgq_cb_wrapper<LTE_fdd_enb_rrc, &LTE_fdd_enb_rrc::handle_mme_msg>, this);

    if(!started)
    {
        started        = true;
        pdcp_comm_msgq = new LTE_fdd_enb_msgq("pdcp_rrc_mq",
                                              pdcp_cb);
        mme_comm_msgq  = new LTE_fdd_enb_msgq("mme_rrc_mq",
                                              mme_cb);
        rrc_pdcp_mq    = new boost::interprocess::message_queue(boost::interprocess::open_only,
                                                                "rrc_pdcp_mq");
        rrc_mme_mq     = new boost::interprocess::message_queue(boost::interprocess::open_only,
                                                                "rrc_mme_mq");
    }
}
void LTE_fdd_enb_rrc::stop(void)
{
    boost::mutex::scoped_lock lock(start_mutex);

    if(started)
    {
        started = false;
        delete pdcp_comm_msgq;
        delete mme_comm_msgq;
    }
}

/***********************/
/*    Communication    */
/***********************/
void LTE_fdd_enb_rrc::handle_pdcp_msg(LTE_FDD_ENB_MESSAGE_STRUCT *msg)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();

    if(LTE_FDD_ENB_DEST_LAYER_RRC == msg->dest_layer ||
       LTE_FDD_ENB_DEST_LAYER_ANY == msg->dest_layer)
    {
        switch(msg->type)
        {
        case LTE_FDD_ENB_MESSAGE_TYPE_RRC_PDU_READY:
            handle_pdu_ready(&msg->msg.rrc_pdu_ready);
            delete msg;
            break;
        default:
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      "Received invalid PDCP message %s",
                                      LTE_fdd_enb_message_type_text[msg->type]);
            delete msg;
            break;
        }
    }else{
        // Forward message to MME
        rrc_mme_mq->send(&msg, sizeof(msg), 0);
    }
}
void LTE_fdd_enb_rrc::handle_mme_msg(LTE_FDD_ENB_MESSAGE_STRUCT *msg)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();

    if(LTE_FDD_ENB_DEST_LAYER_RRC == msg->dest_layer ||
       LTE_FDD_ENB_DEST_LAYER_ANY == msg->dest_layer)
    {
        switch(msg->type)
        {
        case LTE_FDD_ENB_MESSAGE_TYPE_RRC_NAS_MSG_READY:
            handle_nas_msg(&msg->msg.rrc_nas_msg_ready);
            delete msg;
            break;
        case LTE_FDD_ENB_MESSAGE_TYPE_RRC_CMD_READY:
            handle_cmd(&msg->msg.rrc_cmd_ready);
            delete msg;
            break;
        default:
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      "Received invalid MME message %s",
                                      LTE_fdd_enb_message_type_text[msg->type]);
            delete msg;
            break;
        }
    }else{
        // Forward message to PDCP
        rrc_pdcp_mq->send(&msg, sizeof(msg), 0);
    }
}

/****************************/
/*    External Interface    */
/****************************/
void LTE_fdd_enb_rrc::update_sys_info(void)
{
    LTE_fdd_enb_cnfg_db *cnfg_db = LTE_fdd_enb_cnfg_db::get_instance();

    sys_info_mutex.lock();
    cnfg_db->get_sys_info(sys_info);
    sys_info_mutex.unlock();
}

/*******************************/
/*    PDCP Message Handlers    */
/*******************************/
void LTE_fdd_enb_rrc::handle_pdu_ready(LTE_FDD_ENB_RRC_PDU_READY_MSG_STRUCT *pdu_ready)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();
    LIBLTE_BIT_MSG_STRUCT *pdu;

    if(LTE_FDD_ENB_ERROR_NONE == pdu_ready->rb->get_next_rrc_pdu(&pdu))
    {
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  pdu,
                                  "Received PDU for RNTI=%u and RB=%s",
                                  pdu_ready->user->get_c_rnti(),
                                  LTE_fdd_enb_rb_text[pdu_ready->rb->get_rb_id()]);

        // Call the appropriate state machine
        switch(pdu_ready->rb->get_rb_id())
        {
        case LTE_FDD_ENB_RB_SRB0:
            ccch_sm(pdu, pdu_ready->user, pdu_ready->rb);
            break;
        case LTE_FDD_ENB_RB_SRB1:
        case LTE_FDD_ENB_RB_SRB2:
            dcch_sm(pdu, pdu_ready->user, pdu_ready->rb);
            break;
        default:
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      pdu,
                                      "Received PDU for RNTI=%u and invalid RB=%s",
                                      pdu_ready->user->get_c_rnti(),
                                      LTE_fdd_enb_rb_text[pdu_ready->rb->get_rb_id()]);
            break;
        }

        // Delete the PDU
        pdu_ready->rb->delete_next_rrc_pdu();
    }else{
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "Received pdu_ready message with no PDU queued");
    }
}

/******************************/
/*    MME Message Handlers    */
/******************************/
void LTE_fdd_enb_rrc::handle_nas_msg(LTE_FDD_ENB_RRC_NAS_MSG_READY_MSG_STRUCT *nas_msg)
{
    LTE_fdd_enb_interface  *interface = LTE_fdd_enb_interface::get_instance();
    LIBLTE_BYTE_MSG_STRUCT *msg;

    if(LTE_FDD_ENB_ERROR_NONE == nas_msg->rb->get_next_rrc_nas_msg(&msg))
    {
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  msg,
                                  "Received NAS message for RNTI=%u and RB=%s",
                                  nas_msg->user->get_c_rnti(),
                                  LTE_fdd_enb_rb_text[nas_msg->rb->get_rb_id()]);

        // Send the NAS message
        send_dl_info_transfer(nas_msg->user, nas_msg->rb, msg);

        // Delete the NAS message
        nas_msg->rb->delete_next_rrc_nas_msg();
    }else{
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "Received NAS message with no message queued");
    }
}
void LTE_fdd_enb_rrc::handle_cmd(LTE_FDD_ENB_RRC_CMD_READY_MSG_STRUCT *cmd)
{
    LTE_fdd_enb_interface  *interface = LTE_fdd_enb_interface::get_instance();
    LTE_fdd_enb_rb         *srb2      = NULL;
    LIBLTE_BYTE_MSG_STRUCT *msg;

    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              "Received MME command %s for RNTI=%u and RB=%s",
                              LTE_fdd_enb_rrc_cmd_text[cmd->cmd],
                              cmd->user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[cmd->rb->get_rb_id()]);

    switch(cmd->cmd)
    {
    case LTE_FDD_ENB_RRC_CMD_RELEASE:
        send_rrc_con_release(cmd->user, cmd->rb);
        break;
    case LTE_FDD_ENB_RRC_CMD_SECURITY:
        send_security_mode_command(cmd->user, cmd->rb);
        break;
    case LTE_FDD_ENB_RRC_CMD_SETUP_SRB2:
        if(LTE_FDD_ENB_ERROR_NONE == cmd->user->setup_srb2(&srb2))
        {
            // Configure SRB2
            srb2->set_rrc_procedure(cmd->rb->get_rrc_procedure());
            srb2->set_rrc_state(cmd->rb->get_rrc_state());
            srb2->set_mme_procedure(cmd->rb->get_mme_procedure());
            srb2->set_mme_state(cmd->rb->get_mme_state());
            srb2->set_qos(cmd->rb->get_qos());

            if(LTE_FDD_ENB_ERROR_NONE == cmd->rb->get_next_rrc_nas_msg(&msg))
            {
                send_rrc_con_reconfig(cmd->user, cmd->rb, msg);
                delete msg;
            }else{
                send_rrc_con_reconfig(cmd->user, cmd->rb, NULL);
            }
        }else{
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      "Handle CMD can't setup SRB2");
        }
        break;
    default:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "Received MME command %s for RNTI=%u and RB=%s",
                                  LTE_fdd_enb_rrc_cmd_text[cmd->cmd],
                                  cmd->user->get_c_rnti(),
                                  LTE_fdd_enb_rb_text[cmd->rb->get_rb_id()]);
        break;
    }
}

/************************/
/*    State Machines    */
/************************/
void LTE_fdd_enb_rrc::ccch_sm(LIBLTE_BIT_MSG_STRUCT *msg,
                              LTE_fdd_enb_user      *user,
                              LTE_fdd_enb_rb        *rb)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();
    LTE_fdd_enb_rb        *srb1      = NULL;

    // Parse the message
    parse_ul_ccch_message(msg, user, rb);

    switch(rb->get_rrc_procedure())
    {
    case LTE_FDD_ENB_RRC_PROC_RRC_CON_REQ:
        switch(rb->get_rrc_state())
        {
        case LTE_FDD_ENB_RRC_STATE_IDLE:
            if(LTE_FDD_ENB_ERROR_NONE == user->setup_srb1(&srb1))
            {
                rb->set_rrc_state(LTE_FDD_ENB_RRC_STATE_SRB1_SETUP);
                send_rrc_con_setup(user, rb);

                // Setup uplink scheduling
                srb1->set_rrc_procedure(LTE_FDD_ENB_RRC_PROC_RRC_CON_REQ);
                srb1->set_rrc_state(LTE_FDD_ENB_RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE);
                srb1->set_qos(LTE_FDD_ENB_QOS_SIGNALLING);
            }else{
                interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                          LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                          __FILE__,
                                          __LINE__,
                                          "UL-CCCH-Message can't setup srb1");
            }
            break;
        default:
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      "UL-CCCH-Message RRC CON REQ state machine invalid state %s",
                                      LTE_fdd_enb_rrc_state_text[rb->get_rrc_state()]);
            break;
        }
        break;
    case LTE_FDD_ENB_RRC_PROC_RRC_CON_REEST_REQ:
        // FIXME: Not handling RRC Connection Reestablishment Request
    default:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "CCCH state machine invalid procedure %s",
                                  LTE_fdd_enb_rrc_proc_text[rb->get_rrc_procedure()]);
        break;
    }
}
void LTE_fdd_enb_rrc::dcch_sm(LIBLTE_BIT_MSG_STRUCT *msg,
                              LTE_fdd_enb_user      *user,
                              LTE_fdd_enb_rb        *rb)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();

    switch(rb->get_rrc_procedure())
    {
    case LTE_FDD_ENB_RRC_PROC_RRC_CON_REQ:
        switch(rb->get_rrc_state())
        {
        case LTE_FDD_ENB_RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE:
            // Parse the message
            parse_ul_dcch_message(msg, user, rb);
            break;
        case LTE_FDD_ENB_RRC_STATE_RRC_CONNECTED:
            // Parse the message
            parse_ul_dcch_message(msg, user, rb);
            break;
        default:
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      "UL-DCCH-Message RRC CON REQ state machine invalid state %s",
                                      LTE_fdd_enb_rrc_state_text[rb->get_rrc_state()]);
            break;
        }
        break;
    default:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "DCCH state machine invalid procedure %s",
                                  LTE_fdd_enb_rrc_proc_text[rb->get_rrc_procedure()]);
        break;
    }
}

/*************************/
/*    Message Parsers    */
/*************************/
void LTE_fdd_enb_rrc::parse_ul_ccch_message(LIBLTE_BIT_MSG_STRUCT *msg,
                                            LTE_fdd_enb_user      *user,
                                            LTE_fdd_enb_rb        *rb)
{
    LTE_fdd_enb_interface *interface = LTE_fdd_enb_interface::get_instance();

    // Parse the message
    liblte_rrc_unpack_ul_ccch_msg(msg,
                                  &rb->ul_ccch_msg);

    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              "Received %s for RNTI=%u, RB=%s",
                              liblte_rrc_ul_ccch_msg_type_text[rb->ul_ccch_msg.msg_type],
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    switch(rb->ul_ccch_msg.msg_type)
    {
    case LIBLTE_RRC_UL_CCCH_MSG_TYPE_RRC_CON_REQ:
        rb->set_rrc_procedure(LTE_FDD_ENB_RRC_PROC_RRC_CON_REQ);
        break;
    case LIBLTE_RRC_UL_CCCH_MSG_TYPE_RRC_CON_REEST_REQ:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  "Not handling UL-CCCH-Message with msg_type=%s",
                                  liblte_rrc_ul_ccch_msg_type_text[rb->ul_ccch_msg.msg_type]);
        break;
    default:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  msg,
                                  "UL-CCCH-Message received with invalid msg_type=%s",
                                  liblte_rrc_ul_ccch_msg_type_text[rb->ul_ccch_msg.msg_type]);
        break;
    }
}
void LTE_fdd_enb_rrc::parse_ul_dcch_message(LIBLTE_BIT_MSG_STRUCT *msg,
                                            LTE_fdd_enb_user      *user,
                                            LTE_fdd_enb_rb        *rb)
{
    LTE_fdd_enb_interface                    *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_MME_NAS_MSG_READY_MSG_STRUCT  nas_msg_ready;
    LTE_FDD_ENB_MME_RRC_CMD_RESP_MSG_STRUCT   cmd_resp;

    // Parse the message
    liblte_rrc_unpack_ul_dcch_msg(msg,
                                  &rb->ul_dcch_msg);

    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              "Received %s for RNTI=%u, RB=%s",
                              liblte_rrc_ul_dcch_msg_type_text[rb->ul_dcch_msg.msg_type],
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    switch(rb->ul_dcch_msg.msg_type)
    {
    case LIBLTE_RRC_UL_DCCH_MSG_TYPE_RRC_CON_SETUP_COMPLETE:
        rb->set_rrc_state(LTE_FDD_ENB_RRC_STATE_RRC_CONNECTED);

        // Queue the NAS message for MME
        rb->queue_mme_nas_msg(&rb->ul_dcch_msg.msg.rrc_con_setup_complete.dedicated_info_nas);

        // Signal MME
        nas_msg_ready.user = user;
        nas_msg_ready.rb   = rb;
        LTE_fdd_enb_msgq::send(rrc_mme_mq,
                               LTE_FDD_ENB_MESSAGE_TYPE_MME_NAS_MSG_READY,
                               LTE_FDD_ENB_DEST_LAYER_MME,
                               (LTE_FDD_ENB_MESSAGE_UNION *)&nas_msg_ready,
                               sizeof(LTE_FDD_ENB_MME_NAS_MSG_READY_MSG_STRUCT));
        break;
    case LIBLTE_RRC_UL_DCCH_MSG_TYPE_UL_INFO_TRANSFER:
        if(LIBLTE_RRC_UL_INFORMATION_TRANSFER_TYPE_NAS == rb->ul_dcch_msg.msg.ul_info_transfer.dedicated_info_type)
        {
            // Queue the NAS message for MME
            rb->queue_mme_nas_msg(&rb->ul_dcch_msg.msg.ul_info_transfer.dedicated_info);

            // Signal MME
            nas_msg_ready.user = user;
            nas_msg_ready.rb   = rb;
            LTE_fdd_enb_msgq::send(rrc_mme_mq,
                                   LTE_FDD_ENB_MESSAGE_TYPE_MME_NAS_MSG_READY,
                                   LTE_FDD_ENB_DEST_LAYER_MME,
                                   (LTE_FDD_ENB_MESSAGE_UNION *)&nas_msg_ready,
                                   sizeof(LTE_FDD_ENB_MME_NAS_MSG_READY_MSG_STRUCT));
        }else{
            interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                      LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                      __FILE__,
                                      __LINE__,
                                      msg,
                                      "UL-DCCH-Message UL Information Transfer received with invalid dedicated info %s",
                                      liblte_rrc_ul_information_transfer_type_text[rb->ul_dcch_msg.msg.ul_info_transfer.dedicated_info_type]);
        }
        break;
    case LIBLTE_RRC_UL_DCCH_MSG_TYPE_SECURITY_MODE_COMPLETE:
        // Signal RRC
        cmd_resp.user     = user;
        cmd_resp.rb       = rb;
        cmd_resp.cmd_resp = LTE_FDD_ENB_MME_RRC_CMD_RESP_SECURITY;
        LTE_fdd_enb_msgq::send(rrc_mme_mq,
                               LTE_FDD_ENB_MESSAGE_TYPE_MME_RRC_CMD_RESP,
                               LTE_FDD_ENB_DEST_LAYER_MME,
                               (LTE_FDD_ENB_MESSAGE_UNION *)&cmd_resp,
                               sizeof(LTE_FDD_ENB_MME_RRC_CMD_RESP_MSG_STRUCT));
        break;
    case LIBLTE_RRC_UL_DCCH_MSG_TYPE_RRC_CON_RECONFIG_COMPLETE:
        rb->set_qos(LTE_FDD_ENB_QOS_NONE);
        break;
    default:
        interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_ERROR,
                                  LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                                  __FILE__,
                                  __LINE__,
                                  msg,
                                  "UL-DCCH-Message received with invalid msg_type=%s",
                                  liblte_rrc_ul_dcch_msg_type_text[rb->ul_dcch_msg.msg_type]);
        break;
    }
}

/*************************/
/*    Message Senders    */
/*************************/
void LTE_fdd_enb_rrc::send_dl_info_transfer(LTE_fdd_enb_user       *user,
                                            LTE_fdd_enb_rb         *rb,
                                            LIBLTE_BYTE_MSG_STRUCT *msg)
{
    LTE_fdd_enb_interface                 *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT  pdcp_sdu_ready;
    LIBLTE_BIT_MSG_STRUCT                  pdcp_sdu;

    rb->dl_dcch_msg.msg_type                                 = LIBLTE_RRC_DL_DCCH_MSG_TYPE_DL_INFO_TRANSFER;
    rb->dl_dcch_msg.msg.dl_info_transfer.rrc_transaction_id  = rb->get_rrc_transaction_id();
    rb->dl_dcch_msg.msg.dl_info_transfer.dedicated_info_type = LIBLTE_RRC_DL_INFORMATION_TRANSFER_TYPE_NAS;
    memcpy(&rb->dl_dcch_msg.msg.dl_info_transfer.dedicated_info, msg, sizeof(LIBLTE_BYTE_MSG_STRUCT));
    liblte_rrc_pack_dl_dcch_msg(&rb->dl_dcch_msg, &pdcp_sdu);
    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              &pdcp_sdu,
                              "Sending DL Info Transfer for RNTI=%u, RB=%s",
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    // Queue the PDU for PDCP
    rb->queue_pdcp_sdu(&pdcp_sdu);

    // Signal PDCP
    pdcp_sdu_ready.user = user;
    pdcp_sdu_ready.rb   = rb;
    LTE_fdd_enb_msgq::send(rrc_pdcp_mq,
                           LTE_FDD_ENB_MESSAGE_TYPE_PDCP_SDU_READY,
                           LTE_FDD_ENB_DEST_LAYER_PDCP,
                           (LTE_FDD_ENB_MESSAGE_UNION *)&pdcp_sdu_ready,
                           sizeof(LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT));
}
void LTE_fdd_enb_rrc::send_rrc_con_reconfig(LTE_fdd_enb_user       *user,
                                            LTE_fdd_enb_rb         *rb,
                                            LIBLTE_BYTE_MSG_STRUCT *msg)
{
    LTE_fdd_enb_interface                        *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT         pdcp_sdu_ready;
    LIBLTE_RRC_CONNECTION_RECONFIGURATION_STRUCT *rrc_con_recnfg;
    LIBLTE_BIT_MSG_STRUCT                         pdcp_sdu;

    rb->dl_dcch_msg.msg_type              = LIBLTE_RRC_DL_DCCH_MSG_TYPE_RRC_CON_RECONFIG;
    rrc_con_recnfg                        = (LIBLTE_RRC_CONNECTION_RECONFIGURATION_STRUCT *)&rb->dl_dcch_msg.msg.rrc_con_reconfig;
    rrc_con_recnfg->meas_cnfg_present     = false;
    rrc_con_recnfg->mob_ctrl_info_present = false;
    if(NULL == msg)
    {
        rrc_con_recnfg->N_ded_info_nas = 0;
    }else{
        rrc_con_recnfg->N_ded_info_nas = 1;
        memcpy(&rrc_con_recnfg->ded_info_nas_list[0], msg, sizeof(LIBLTE_BYTE_MSG_STRUCT));
    }
    rrc_con_recnfg->rr_cnfg_ded_present                                                                  = true;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list_size                                                 = 1;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list[0].srb_id                                            = 2;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list[0].rlc_cnfg_present                                  = true;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list[0].rlc_default_cnfg_present                          = true;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list[0].lc_cnfg_present                                   = true;
    rrc_con_recnfg->rr_cnfg_ded.srb_to_add_mod_list[0].lc_default_cnfg_present                           = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list_size                                                 = 1;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].eps_bearer_id_present                             = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].eps_bearer_id                                     = user->get_eps_bearer_id();
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].drb_id                                            = 1;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg_present                                 = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.discard_timer_present                   = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.discard_timer                           = LIBLTE_RRC_DISCARD_TIMER_INFINITY;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.rlc_am_status_report_required_present   = false;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.rlc_um_pdcp_sn_size_present             = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.rlc_um_pdcp_sn_size                     = LIBLTE_RRC_PDCP_SN_SIZE_12_BITS;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].pdcp_cnfg.hdr_compression_rohc                    = false;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].rlc_cnfg_present                                  = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].rlc_cnfg.rlc_mode                                 = LIBLTE_RRC_RLC_MODE_UM_BI;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].rlc_cnfg.ul_um_bi_rlc.sn_field_len                = LIBLTE_RRC_SN_FIELD_LENGTH_SIZE10;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].rlc_cnfg.dl_um_bi_rlc.sn_field_len                = LIBLTE_RRC_SN_FIELD_LENGTH_SIZE10;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].rlc_cnfg.dl_um_bi_rlc.t_reordering                = LIBLTE_RRC_T_REORDERING_MS50;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_id_present                                     = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_id                                             = 3;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg_present                                   = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params_present                = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params.priority               = 13;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params.prioritized_bit_rate   = LIBLTE_RRC_PRIORITIZED_BIT_RATE_INFINITY;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params.bucket_size_duration   = LIBLTE_RRC_BUCKET_SIZE_DURATION_MS100;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params.log_chan_group_present = true;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.ul_specific_params.log_chan_group         = 2;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_add_mod_list[0].lc_cnfg.log_chan_sr_mask_present                  = false;
    rrc_con_recnfg->rr_cnfg_ded.drb_to_release_list_size                                                 = 0;
    rrc_con_recnfg->rr_cnfg_ded.mac_main_cnfg_present                                                    = false;
    rrc_con_recnfg->rr_cnfg_ded.sps_cnfg_present                                                         = false;
    rrc_con_recnfg->rr_cnfg_ded.phy_cnfg_ded_present                                                     = false;
    rrc_con_recnfg->rr_cnfg_ded.rlf_timers_and_constants_present                                         = false;
    rrc_con_recnfg->sec_cnfg_ho_present                                                                  = false;
    liblte_rrc_pack_dl_dcch_msg(&rb->dl_dcch_msg, &pdcp_sdu);
    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              &pdcp_sdu,
                              "Sending RRC Connection Reconfiguration for RNTI=%u, RB=%s",
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    // Queue the PDU for PDCP
    rb->queue_pdcp_sdu(&pdcp_sdu);

    // Signal PDCP
    pdcp_sdu_ready.user = user;
    pdcp_sdu_ready.rb   = rb;
    LTE_fdd_enb_msgq::send(rrc_pdcp_mq,
                           LTE_FDD_ENB_MESSAGE_TYPE_PDCP_SDU_READY,
                           LTE_FDD_ENB_DEST_LAYER_PDCP,
                           (LTE_FDD_ENB_MESSAGE_UNION *)&pdcp_sdu_ready,
                           sizeof(LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT));
}
void LTE_fdd_enb_rrc::send_rrc_con_release(LTE_fdd_enb_user *user,
                                           LTE_fdd_enb_rb   *rb)
{
    LTE_fdd_enb_interface                 *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT  pdcp_sdu_ready;
    LIBLTE_BIT_MSG_STRUCT                  pdcp_sdu;

    rb->dl_dcch_msg.msg_type                               = LIBLTE_RRC_DL_DCCH_MSG_TYPE_RRC_CON_RELEASE;
    rb->dl_dcch_msg.msg.rrc_con_release.rrc_transaction_id = rb->get_rrc_transaction_id();
    rb->dl_dcch_msg.msg.rrc_con_release.release_cause      = LIBLTE_RRC_RELEASE_CAUSE_OTHER;
    liblte_rrc_pack_dl_dcch_msg(&rb->dl_dcch_msg, &pdcp_sdu);
    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              &pdcp_sdu,
                              "Sending RRC Connection Release for RNTI=%u, RB=%s",
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    // Queue the PDU for PDCP
    rb->queue_pdcp_sdu(&pdcp_sdu);

    // Signal PDCP
    pdcp_sdu_ready.user = user;
    pdcp_sdu_ready.rb   = rb;
    LTE_fdd_enb_msgq::send(rrc_pdcp_mq,
                           LTE_FDD_ENB_MESSAGE_TYPE_PDCP_SDU_READY,
                           LTE_FDD_ENB_DEST_LAYER_PDCP,
                           (LTE_FDD_ENB_MESSAGE_UNION *)&pdcp_sdu_ready,
                           sizeof(LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT));
}
void LTE_fdd_enb_rrc::send_rrc_con_setup(LTE_fdd_enb_user *user,
                                         LTE_fdd_enb_rb   *rb)
{
    LTE_fdd_enb_interface                 *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT  pdcp_sdu_ready;
    LIBLTE_RRC_CONNECTION_SETUP_STRUCT    *rrc_con_setup;
    LIBLTE_BIT_MSG_STRUCT                  pdcp_sdu;

    rb->dl_ccch_msg.msg_type                                                                  = LIBLTE_RRC_DL_CCCH_MSG_TYPE_RRC_CON_SETUP;
    rrc_con_setup                                                                             = (LIBLTE_RRC_CONNECTION_SETUP_STRUCT *)&rb->dl_ccch_msg.msg.rrc_con_setup;
    rrc_con_setup->rrc_transaction_id                                                         = rb->get_rrc_transaction_id();
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list_size                                           = 1;
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list[0].srb_id                                      = 1;
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list[0].rlc_cnfg_present                            = true;
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list[0].rlc_default_cnfg_present                    = true;
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list[0].lc_cnfg_present                             = true;
    rrc_con_setup->rr_cnfg.srb_to_add_mod_list[0].lc_default_cnfg_present                     = true;
    rrc_con_setup->rr_cnfg.drb_to_add_mod_list_size                                           = 0;
    rrc_con_setup->rr_cnfg.drb_to_release_list_size                                           = 0;
    rrc_con_setup->rr_cnfg.mac_main_cnfg_present                                              = true;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.default_value                                        = false;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg_present                    = true;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg.max_harq_tx_present        = true;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg.max_harq_tx                = LIBLTE_RRC_MAX_HARQ_TX_N1;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg.periodic_bsr_timer_present = false;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg.retx_bsr_timer             = LIBLTE_RRC_RETRANSMISSION_BSR_TIMER_SF1280;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.ulsch_cnfg.tti_bundling               = false;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.drx_cnfg_present                      = false;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.phr_cnfg_present                      = false;
    rrc_con_setup->rr_cnfg.mac_main_cnfg.explicit_value.time_alignment_timer                  = LIBLTE_RRC_TIME_ALIGNMENT_TIMER_SF500;
    rrc_con_setup->rr_cnfg.sps_cnfg_present                                                   = false;
    rrc_con_setup->rr_cnfg.phy_cnfg_ded_present                                               = false;
    rrc_con_setup->rr_cnfg.rlf_timers_and_constants_present                                   = false;
    liblte_rrc_pack_dl_ccch_msg(&rb->dl_ccch_msg, &pdcp_sdu);
    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              &pdcp_sdu,
                              "Sending RRC Connection Setup for RNTI=%u, RB=%s",
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    // Queue the PDU for PDCP
    rb->queue_pdcp_sdu(&pdcp_sdu);

    // Signal PDCP
    pdcp_sdu_ready.user = user;
    pdcp_sdu_ready.rb   = rb;
    LTE_fdd_enb_msgq::send(rrc_pdcp_mq,
                           LTE_FDD_ENB_MESSAGE_TYPE_PDCP_SDU_READY,
                           LTE_FDD_ENB_DEST_LAYER_PDCP,
                           (LTE_FDD_ENB_MESSAGE_UNION *)&pdcp_sdu_ready,
                           sizeof(LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT));
}
void LTE_fdd_enb_rrc::send_security_mode_command(LTE_fdd_enb_user *user,
                                                 LTE_fdd_enb_rb   *rb)
{
    LTE_fdd_enb_interface                 *interface = LTE_fdd_enb_interface::get_instance();
    LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT  pdcp_sdu_ready;
    LIBLTE_BIT_MSG_STRUCT                  pdcp_sdu;

    rb->dl_dcch_msg.msg_type                                  = LIBLTE_RRC_DL_DCCH_MSG_TYPE_SECURITY_MODE_COMMAND;
    rb->dl_dcch_msg.msg.security_mode_cmd.rrc_transaction_id  = rb->get_rrc_transaction_id();
    rb->dl_dcch_msg.msg.security_mode_cmd.sec_algs.cipher_alg = LIBLTE_RRC_CIPHERING_ALGORITHM_EEA0;
    rb->dl_dcch_msg.msg.security_mode_cmd.sec_algs.int_alg    = LIBLTE_RRC_INTEGRITY_PROT_ALGORITHM_EIA2;
    liblte_rrc_pack_dl_dcch_msg(&rb->dl_dcch_msg, &pdcp_sdu);
    interface->send_debug_msg(LTE_FDD_ENB_DEBUG_TYPE_INFO,
                              LTE_FDD_ENB_DEBUG_LEVEL_RRC,
                              __FILE__,
                              __LINE__,
                              &pdcp_sdu,
                              "Sending Security Mode Command for RNTI=%u, RB=%s",
                              user->get_c_rnti(),
                              LTE_fdd_enb_rb_text[rb->get_rb_id()]);

    // Configure PDCP for security
    rb->set_pdcp_config(LTE_FDD_ENB_PDCP_CONFIG_SECURITY);

    // Queue the PDU for PDCP
    rb->queue_pdcp_sdu(&pdcp_sdu);

    // Signal PDCP
    pdcp_sdu_ready.user = user;
    pdcp_sdu_ready.rb   = rb;
    LTE_fdd_enb_msgq::send(rrc_pdcp_mq,
                           LTE_FDD_ENB_MESSAGE_TYPE_PDCP_SDU_READY,
                           LTE_FDD_ENB_DEST_LAYER_PDCP,
                           (LTE_FDD_ENB_MESSAGE_UNION *)&pdcp_sdu_ready,
                           sizeof(LTE_FDD_ENB_PDCP_SDU_READY_MSG_STRUCT));
}
