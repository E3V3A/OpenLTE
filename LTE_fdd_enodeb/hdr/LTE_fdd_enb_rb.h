/*******************************************************************************

    Copyright 2014 Ben Wojtowicz

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

    File: LTE_fdd_enb_rb.h

    Description: Contains all the definitions for the LTE FDD eNodeB
                 radio bearer class.

    Revision History
    ----------    -------------    --------------------------------------------
    05/04/2014    Ben Wojtowicz    Created file
    06/15/2014    Ben Wojtowicz    Added more states and procedures, QoS, MME,
                                   RLC, and uplink scheduling functionality.
    08/03/2014    Ben Wojtowicz    Added MME procedures/states, RRC NAS support,
                                   RRC transaction id, PDCP sequence numbers,
                                   and RLC transmit variables.
    09/03/2014    Ben Wojtowicz    Added more MME states and ability to store
                                   the contention resolution identity.

*******************************************************************************/

#ifndef __LTE_FDD_ENB_RB_H__
#define __LTE_FDD_ENB_RB_H__

/*******************************************************************************
                              INCLUDES
*******************************************************************************/

#include "LTE_fdd_enb_interface.h"
#include "liblte_rlc.h"
#include "liblte_rrc.h"
#include <list>

/*******************************************************************************
                              DEFINES
*******************************************************************************/


/*******************************************************************************
                              FORWARD DECLARATIONS
*******************************************************************************/

class LTE_fdd_enb_user;

/*******************************************************************************
                              TYPEDEFS
*******************************************************************************/

typedef enum{
    LTE_FDD_ENB_RB_SRB0 = 0,
    LTE_FDD_ENB_RB_SRB1,
    LTE_FDD_ENB_RB_SRB2,
    LTE_FDD_ENB_RB_N_ITEMS,
}LTE_FDD_ENB_RB_ENUM;
static const char LTE_fdd_enb_rb_text[LTE_FDD_ENB_RB_N_ITEMS][20] = {"SRB0",
                                                                     "SRB1",
                                                                     "SRB2"};

typedef enum{
    LTE_FDD_ENB_MME_PROC_IDLE = 0,
    LTE_FDD_ENB_MME_PROC_ATTACH,
    LTE_FDD_ENB_MME_PROC_N_ITEMS,
}LTE_FDD_ENB_MME_PROC_ENUM;
static const char LTE_fdd_enb_mme_proc_text[LTE_FDD_ENB_MME_PROC_N_ITEMS][100] = {"IDLE",
                                                                                  "ATTACH"};

typedef enum{
    LTE_FDD_ENB_MME_STATE_IDLE = 0,
    LTE_FDD_ENB_MME_STATE_ID_REQUEST_IMSI,
    LTE_FDD_ENB_MME_STATE_REJECT,
    LTE_FDD_ENB_MME_STATE_AUTHENTICATE,
    LTE_FDD_ENB_MME_STATE_AUTH_REJECTED,
    LTE_FDD_ENB_MME_STATE_ENABLE_SECURITY,
    LTE_FDD_ENB_MME_STATE_RELEASE,
    LTE_FDD_ENB_MME_STATE_N_ITEMS,
}LTE_FDD_ENB_MME_STATE_ENUM;
static const char LTE_fdd_enb_mme_state_text[LTE_FDD_ENB_MME_STATE_N_ITEMS][100] = {"IDLE",
                                                                                    "ID REQUEST IMSI",
                                                                                    "REJECT",
                                                                                    "AUTHENTICATE",
                                                                                    "AUTH REJECTED",
                                                                                    "ENABLE SECURITY",
                                                                                    "RELEASE"};

typedef enum{
    LTE_FDD_ENB_RRC_PROC_IDLE = 0,
    LTE_FDD_ENB_RRC_PROC_RRC_CON_REQ,
    LTE_FDD_ENB_RRC_PROC_RRC_CON_REEST_REQ,
    LTE_FDD_ENB_RRC_PROC_N_ITEMS,
}LTE_FDD_ENB_RRC_PROC_ENUM;
static const char LTE_fdd_enb_rrc_proc_text[LTE_FDD_ENB_RRC_PROC_N_ITEMS][100] = {"IDLE",
                                                                                  "RRC CON REQ",
                                                                                  "RRC CON REEST REQ"};

typedef enum{
    LTE_FDD_ENB_RRC_STATE_IDLE = 0,
    LTE_FDD_ENB_RRC_STATE_SRB1_SETUP,
    LTE_FDD_ENB_RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE,
    LTE_FDD_ENB_RRC_STATE_RRC_CONNECTED,
    LTE_FDD_ENB_RRC_STATE_N_ITEMS,
}LTE_FDD_ENB_RRC_STATE_ENUM;
static const char LTE_fdd_enb_rrc_state_text[LTE_FDD_ENB_RRC_STATE_N_ITEMS][100] = {"IDLE",
                                                                                    "SRB1 SETUP",
                                                                                    "WAIT FOR CON SETUP COMPLETE",
                                                                                    "RRC CONNECTED"};

typedef enum{
    LTE_FDD_ENB_PDCP_CONFIG_N_A = 0,
    LTE_FDD_ENB_PDCP_CONFIG_N_ITEMS,
}LTE_FDD_ENB_PDCP_CONFIG_ENUM;
static const char LTE_fdd_enb_pdcp_config_text[LTE_FDD_ENB_PDCP_CONFIG_N_ITEMS][20] = {"N/A"};

typedef enum{
    LTE_FDD_ENB_RLC_CONFIG_TM = 0,
    LTE_FDD_ENB_RLC_CONFIG_UM,
    LTE_FDD_ENB_RLC_CONFIG_AM,
    LTE_FDD_ENB_RLC_CONFIG_N_ITEMS,
}LTE_FDD_ENB_RLC_CONFIG_ENUM;
static const char LTE_fdd_enb_rlc_config_text[LTE_FDD_ENB_RLC_CONFIG_N_ITEMS][20] = {"TM",
                                                                                     "UM",
                                                                                     "AM"};

typedef enum{
    LTE_FDD_ENB_MAC_CONFIG_TM = 0,
    LTE_FDD_ENB_MAC_CONFIG_N_ITEMS,
}LTE_FDD_ENB_MAC_CONFIG_ENUM;
static const char LTE_fdd_enb_mac_config_text[LTE_FDD_ENB_MAC_CONFIG_N_ITEMS][20] = {"TM"};

typedef enum{
    LTE_FDD_ENB_QOS_NONE = 0,
    LTE_FDD_ENB_QOS_SIGNALLING,
    LTE_FDD_ENB_QOS_N_ITEMS,
}LTE_FDD_ENB_QOS_ENUM;
static const char LTE_fdd_enb_qos_text[LTE_FDD_ENB_QOS_N_ITEMS][20] = {"None",
                                                                       "Signalling"};

typedef struct{
    uint32 tti_frequency;
    uint32 bits_per_subfn;
}LTE_FDD_ENB_QOS_STRUCT;

/*******************************************************************************
                              CLASS DECLARATIONS
*******************************************************************************/

class LTE_fdd_enb_rb
{
public:
    // Constructor/Destructor
    LTE_fdd_enb_rb(LTE_FDD_ENB_RB_ENUM _rb, LTE_fdd_enb_user *_user);
    ~LTE_fdd_enb_rb();

    // Identity
    LTE_FDD_ENB_RB_ENUM get_rb_id(void);

    // MME
    void queue_mme_nas_msg(LIBLTE_BYTE_MSG_STRUCT *nas_msg);
    LTE_FDD_ENB_ERROR_ENUM get_next_mme_nas_msg(LIBLTE_BYTE_MSG_STRUCT **nas_msg);
    LTE_FDD_ENB_ERROR_ENUM delete_next_mme_nas_msg(void);
    void set_mme_procedure(LTE_FDD_ENB_MME_PROC_ENUM procedure);
    LTE_FDD_ENB_MME_PROC_ENUM get_mme_procedure(void);
    void set_mme_state(LTE_FDD_ENB_MME_STATE_ENUM state);
    LTE_FDD_ENB_MME_STATE_ENUM get_mme_state(void);

    // RRC
    LIBLTE_RRC_UL_CCCH_MSG_STRUCT ul_ccch_msg;
    LIBLTE_RRC_UL_DCCH_MSG_STRUCT ul_dcch_msg;
    LIBLTE_RRC_DL_CCCH_MSG_STRUCT dl_ccch_msg;
    LIBLTE_RRC_DL_DCCH_MSG_STRUCT dl_dcch_msg;
    void queue_rrc_pdu(LIBLTE_BIT_MSG_STRUCT *pdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_rrc_pdu(LIBLTE_BIT_MSG_STRUCT **pdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_rrc_pdu(void);
    void queue_rrc_nas_msg(LIBLTE_BYTE_MSG_STRUCT *nas_msg);
    LTE_FDD_ENB_ERROR_ENUM get_next_rrc_nas_msg(LIBLTE_BYTE_MSG_STRUCT **nas_msg);
    LTE_FDD_ENB_ERROR_ENUM delete_next_rrc_nas_msg(void);
    void set_rrc_procedure(LTE_FDD_ENB_RRC_PROC_ENUM procedure);
    LTE_FDD_ENB_RRC_PROC_ENUM get_rrc_procedure(void);
    void set_rrc_state(LTE_FDD_ENB_RRC_STATE_ENUM state);
    LTE_FDD_ENB_RRC_STATE_ENUM get_rrc_state(void);
    uint8 get_rrc_transaction_id(void);
    void set_rrc_transaction_id(uint8 transaction_id);

    // PDCP
    void queue_pdcp_pdu(LIBLTE_BIT_MSG_STRUCT *pdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_pdcp_pdu(LIBLTE_BIT_MSG_STRUCT **pdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_pdcp_pdu(void);
    void queue_pdcp_sdu(LIBLTE_BIT_MSG_STRUCT *sdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_pdcp_sdu(LIBLTE_BIT_MSG_STRUCT **sdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_pdcp_sdu(void);
    LTE_FDD_ENB_PDCP_CONFIG_ENUM get_pdcp_config(void);
    uint16 get_pdcp_rx_sn(void);
    void set_pdcp_rx_sn(uint16 rx_sn);
    uint16 get_pdcp_tx_sn(void);
    void set_pdcp_tx_sn(uint16 tx_sn);

    // RLC
    void queue_rlc_pdu(LIBLTE_BIT_MSG_STRUCT *pdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_rlc_pdu(LIBLTE_BIT_MSG_STRUCT **pdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_rlc_pdu(void);
    void queue_rlc_sdu(LIBLTE_BIT_MSG_STRUCT *sdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_rlc_sdu(LIBLTE_BIT_MSG_STRUCT **sdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_rlc_sdu(void);
    LTE_FDD_ENB_RLC_CONFIG_ENUM get_rlc_config(void);
    uint16 get_rlc_vrr(void);
    void set_rlc_vrr(uint16 vrr);
    uint16 get_rlc_vrmr(void);
    uint16 get_rlc_vrh(void);
    void set_rlc_vrh(uint16 vrh);
    void rlc_add_to_reception_buffer(LIBLTE_RLC_AMD_PDU_STRUCT *amd_pdu);
    void rlc_get_reception_buffer_status(LIBLTE_RLC_STATUS_PDU_STRUCT *status);
    LTE_FDD_ENB_ERROR_ENUM rlc_reassemble(LIBLTE_BIT_MSG_STRUCT *sdu);
    uint16 get_rlc_vta(void);
    void set_rlc_vta(uint16 vta);
    uint16 get_rlc_vtms(void);
    uint16 get_rlc_vts(void);
    void set_rlc_vts(uint16 vts);
    void rlc_add_to_transmission_buffer(LIBLTE_RLC_AMD_PDU_STRUCT *amd_pdu);
    void rlc_update_transmission_buffer(uint32 ack_sn);
    void rlc_start_t_poll_retransmit(void);
    void rlc_stop_t_poll_retransmit(void);
    void handle_t_poll_retransmit_timer_expiry(uint32 timer_id);

    // MAC
    void queue_mac_sdu(LIBLTE_BIT_MSG_STRUCT *sdu);
    LTE_FDD_ENB_ERROR_ENUM get_next_mac_sdu(LIBLTE_BIT_MSG_STRUCT **sdu);
    LTE_FDD_ENB_ERROR_ENUM delete_next_mac_sdu(void);
    LTE_FDD_ENB_MAC_CONFIG_ENUM get_mac_config(void);
    void start_ul_sched_timer(uint32 m_seconds);
    void handle_ul_sched_timer_expiry(uint32 timer_id);
    void set_con_res_id(uint64 con_res_id);
    uint64 get_con_res_id(void);
    void set_send_con_res_id(bool send_con_res_id);
    bool get_send_con_res_id(void);

    // Generic
    void set_qos(LTE_FDD_ENB_QOS_ENUM _qos);
    LTE_FDD_ENB_QOS_ENUM get_qos(void);
    uint32 get_qos_tti_freq(void);
    uint32 get_qos_bits_per_subfn(void);

private:
    // Identity
    LTE_FDD_ENB_RB_ENUM  rb;
    LTE_fdd_enb_user    *user;

    // MME
    boost::mutex                        mme_nas_msg_queue_mutex;
    std::list<LIBLTE_BYTE_MSG_STRUCT *> mme_nas_msg_queue;
    LTE_FDD_ENB_MME_PROC_ENUM           mme_procedure;
    LTE_FDD_ENB_MME_STATE_ENUM          mme_state;

    // RRC
    boost::mutex                        rrc_pdu_queue_mutex;
    boost::mutex                        rrc_nas_msg_queue_mutex;
    std::list<LIBLTE_BIT_MSG_STRUCT *>  rrc_pdu_queue;
    std::list<LIBLTE_BYTE_MSG_STRUCT *> rrc_nas_msg_queue;
    LTE_FDD_ENB_RRC_PROC_ENUM           rrc_procedure;
    LTE_FDD_ENB_RRC_STATE_ENUM          rrc_state;
    uint8                               rrc_transaction_id;

    // PDCP
    boost::mutex                       pdcp_pdu_queue_mutex;
    boost::mutex                       pdcp_sdu_queue_mutex;
    std::list<LIBLTE_BIT_MSG_STRUCT *> pdcp_pdu_queue;
    std::list<LIBLTE_BIT_MSG_STRUCT *> pdcp_sdu_queue;
    LTE_FDD_ENB_PDCP_CONFIG_ENUM       pdcp_config;
    uint16                             pdcp_rx_sn;
    uint16                             pdcp_tx_sn;

    // RLC
    boost::mutex                                  rlc_pdu_queue_mutex;
    boost::mutex                                  rlc_sdu_queue_mutex;
    std::list<LIBLTE_BIT_MSG_STRUCT *>            rlc_pdu_queue;
    std::list<LIBLTE_BIT_MSG_STRUCT *>            rlc_sdu_queue;
    std::map<uint16, LIBLTE_BIT_MSG_STRUCT *>     rlc_reception_buffer;
    std::map<uint16, LIBLTE_RLC_AMD_PDU_STRUCT *> rlc_transmission_buffer;
    LTE_FDD_ENB_RLC_CONFIG_ENUM                   rlc_config;
    uint16                                        rlc_vrr;
    uint16                                        rlc_vrmr;
    uint16                                        rlc_vrh;
    uint16                                        rlc_first_segment_sn;
    uint16                                        rlc_last_segment_sn;
    uint16                                        rlc_vta;
    uint16                                        rlc_vtms;
    uint16                                        rlc_vts;

    // MAC
    boost::mutex                       mac_sdu_queue_mutex;
    std::list<LIBLTE_BIT_MSG_STRUCT *> mac_sdu_queue;
    LTE_FDD_ENB_MAC_CONFIG_ENUM        mac_config;
    uint64                             mac_con_res_id;
    uint32                             ul_sched_timer_m_seconds;
    uint32                             ul_sched_timer_id;
    uint32                             t_poll_retransmit_timer_id;
    bool                               mac_send_con_res_id;

    // Generic
    void queue_msg(LIBLTE_BIT_MSG_STRUCT *msg, boost::mutex *mutex, std::list<LIBLTE_BIT_MSG_STRUCT *> *queue);
    void queue_msg(LIBLTE_BYTE_MSG_STRUCT *msg, boost::mutex *mutex, std::list<LIBLTE_BYTE_MSG_STRUCT *> *queue);
    LTE_FDD_ENB_ERROR_ENUM get_next_msg(boost::mutex *mutex, std::list<LIBLTE_BIT_MSG_STRUCT *> *queue, LIBLTE_BIT_MSG_STRUCT **msg);
    LTE_FDD_ENB_ERROR_ENUM get_next_msg(boost::mutex *mutex, std::list<LIBLTE_BYTE_MSG_STRUCT *> *queue, LIBLTE_BYTE_MSG_STRUCT **msg);
    LTE_FDD_ENB_ERROR_ENUM delete_next_msg(boost::mutex *mutex, std::list<LIBLTE_BIT_MSG_STRUCT *> *queue);
    LTE_FDD_ENB_ERROR_ENUM delete_next_msg(boost::mutex *mutex, std::list<LIBLTE_BYTE_MSG_STRUCT *> *queue);
    LTE_FDD_ENB_QOS_STRUCT avail_qos[LTE_FDD_ENB_QOS_N_ITEMS];
    LTE_FDD_ENB_QOS_ENUM   qos;
};

#endif /* __LTE_FDD_ENB_RB_H__ */
