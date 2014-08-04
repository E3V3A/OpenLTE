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

    File: liblte_rlc.h

    Description: Contains all the definitions for the LTE Radio Link Control
                 Layer library.

    Revision History
    ----------    -------------    --------------------------------------------
    06/15/2014    Ben Wojtowicz    Created file.
    08/03/2014    Ben Wojtowicz    Added NACK support.

*******************************************************************************/

#ifndef __LIBLTE_RLC_H__
#define __LIBLTE_RLC_H__

/*******************************************************************************
                              INCLUDES
*******************************************************************************/

#include "liblte_common.h"

/*******************************************************************************
                              DEFINES
*******************************************************************************/

#define LIBLTE_RLC_AM_WINDOW_SIZE 512

/*******************************************************************************
                              TYPEDEFS
*******************************************************************************/


/*******************************************************************************
                              PARAMETER DECLARATIONS
*******************************************************************************/

/*********************************************************************
    Parameter: Extension Bit (E)

    Description: The E field indicates whether Data field follows or
                 a set of E field and LI fields follows.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.4
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_E_FIELD_HEADER_NOT_EXTENDED = 0,
    LIBLTE_RLC_E_FIELD_HEADER_EXTENDED,
    LIBLTE_RLC_E_FIELD_N_ITEMS,
}LIBLTE_RLC_E_FIELD_ENUM;
static const char liblte_rlc_e_field_text[LIBLTE_RLC_E_FIELD_N_ITEMS][20] = {"Header Not Extended",
                                                                             "Header Extended"};
// Structs
// Functions

/*********************************************************************
    Parameter: Framing Info (FI)

    Description: The FI field indicates whether a RLC SDU is
                 segmented at the beginning and/or at the end of the
                 Data field.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.6
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_FI_FIELD_FULL_SDU = 0,
    LIBLTE_RLC_FI_FIELD_FIRST_SDU_SEGMENT,
    LIBLTE_RLC_FI_FIELD_LAST_SDU_SEGMENT,
    LIBLTE_RLC_FI_FIELD_MIDDLE_SDU_SEGMENT,
    LIBLTE_RLC_FI_FIELD_N_ITEMS,
}LIBLTE_RLC_FI_FIELD_ENUM;
static const char liblte_rlc_fi_field_text[LIBLTE_RLC_FI_FIELD_N_ITEMS][20] = {"Full SDU",
                                                                               "First SDU Segment",
                                                                               "Last SDU Segment",
                                                                               "Middle SDU Segment"};
// Structs
// Functions

/*********************************************************************
    Parameter: Last Segment Flag (LSF)

    Description: The LSF field indicates whether or not the last byte
                 of the AMD PDU segment corresponds to the last byte
                 of an AMD PDU.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.8
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_LSF_FIELD_NOT_LAST_SEGMENT = 0,
    LIBLTE_RLC_LSF_FIELD_LAST_SEGMENT,
    LIBLTE_RLC_LSF_FIELD_N_ITEMS,
}LIBLTE_RLC_LSF_FIELD_ENUM;
static const char liblte_rlc_lsf_field_text[LIBLTE_RLC_LSF_FIELD_N_ITEMS][20] = {"Not Last Segment",
                                                                                 "Last Segment"};
// Structs
// Functions

/*********************************************************************
    Parameter: Data/Control (D/C)

    Description: The D/C field indicates whether the RLC PDU is a
                 RLC data PDU or RLC control PDU.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.9
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_DC_FIELD_CONTROL_PDU = 0,
    LIBLTE_RLC_DC_FIELD_DATA_PDU,
    LIBLTE_RLC_DC_FIELD_N_ITEMS,
}LIBLTE_RLC_DC_FIELD_ENUM;
static const char liblte_rlc_dc_field_text[LIBLTE_RLC_DC_FIELD_N_ITEMS][20] = {"Control PDU",
                                                                               "Data PDU"};
// Structs
// Functions

/*********************************************************************
    Parameter: Re-segmentation Flag (RF)

    Description: The RF field indicates whether the RLC PDU is an
                 AMD PDU or AMD PDU segment.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.10
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_RF_FIELD_AMD_PDU = 0,
    LIBLTE_RLC_RF_FIELD_AMD_PDU_SEGMENT,
    LIBLTE_RLC_RF_FIELD_N_ITEMS,
}LIBLTE_RLC_RF_FIELD_ENUM;
static const char liblte_rlc_rf_field_text[LIBLTE_RLC_RF_FIELD_N_ITEMS][20] = {"AMD PDU",
                                                                               "AMD PDU Segment"};
// Structs
// Functions

/*********************************************************************
    Parameter: Polling Bit (P)

    Description: The P field indicates whether or not the transmitting
                 side of an AM RLC entity requests a status report
                 from its peer AM RLC entity.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.11
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_P_FIELD_STATUS_REPORT_NOT_REQUESTED = 0,
    LIBLTE_RLC_P_FIELD_STATUS_REPORT_REQUESTED,
    LIBLTE_RLC_P_FIELD_N_ITEMS,
}LIBLTE_RLC_P_FIELD_ENUM;
static const char liblte_rlc_p_field_text[LIBLTE_RLC_P_FIELD_N_ITEMS][40] = {"Status Report Not Requested",
                                                                             "Status Report Requested"};
// Structs
// Functions

/*********************************************************************
    Parameter: Control PDU Type (CPT)

    Description: The CPT field indicates the type of the RLC control
                 PDU.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.13
*********************************************************************/
// Defines
#define LIBLTE_RLC_CPT_FIELD_STATUS_PDU 0
// Enums
// Structs
// Functions

/*********************************************************************
    Parameter: Extension Bit 1 (E1)

    Description: The E1 field indicates whether or not a set of
                 NACK_SN, E1 and E2 follows.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.15
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_E1_FIELD_NOT_EXTENDED = 0,
    LIBLTE_RLC_E1_FIELD_EXTENDED,
    LIBLTE_RLC_E1_FIELD_N_ITEMS,
}LIBLTE_RLC_E1_FIELD_ENUM;
static const char liblte_rlc_e1_field_text[LIBLTE_RLC_E1_FIELD_N_ITEMS][20] = {"Not Extended",
                                                                               "Extended"};
// Structs
// Functions

/*********************************************************************
    Parameter: Extension Bit 2 (E2)

    Description: The E2 field indicates whether or not a set of
                 SOstart and SOend follows.

    Document Reference: 36.322 v10.0.0 Section 6.2.2.17
*********************************************************************/
// Defines
// Enums
typedef enum{
    LIBLTE_RLC_E2_FIELD_NOT_EXTENDED = 0,
    LIBLTE_RLC_E2_FIELD_EXTENDED,
    LIBLTE_RLC_E2_FIELD_N_ITEMS,
}LIBLTE_RLC_E2_FIELD_ENUM;
static const char liblte_rlc_e2_field_text[LIBLTE_RLC_E2_FIELD_N_ITEMS][20] = {"Not Extended",
                                                                               "Extended"};
// Structs
// Functions

/*******************************************************************************
                              PDU DECLARATIONS
*******************************************************************************/

/*********************************************************************
    PDU Type: Unacknowledged Mode Data PDU

    Document Reference: 36.322 v10.0.0 Section 6.2.1.3
*********************************************************************/
// Defines
// Enums
// Structs
// Functions
// FIXME

/*********************************************************************
    PDU Type: Acknowledged Mode Data PDU

    Document Reference: 36.322 v10.0.0 Sections 6.2.1.4 & 6.2.1.5
*********************************************************************/
// Defines
// Enums
// Structs
typedef struct{
    LIBLTE_RLC_DC_FIELD_ENUM dc;
    LIBLTE_RLC_RF_FIELD_ENUM rf;
    LIBLTE_RLC_P_FIELD_ENUM  p;
    LIBLTE_RLC_FI_FIELD_ENUM fi;
    uint16                   sn;
}LIBLTE_RLC_AMD_PDU_HEADER_STRUCT;
typedef struct{
    LIBLTE_RLC_AMD_PDU_HEADER_STRUCT hdr;
    LIBLTE_BIT_MSG_STRUCT            data;
}LIBLTE_RLC_AMD_PDU_STRUCT;
// Functions
LIBLTE_ERROR_ENUM liblte_rlc_pack_amd_pdu(LIBLTE_RLC_AMD_PDU_STRUCT *amd,
                                          LIBLTE_BIT_MSG_STRUCT     *pdu);
LIBLTE_ERROR_ENUM liblte_rlc_pack_amd_pdu(LIBLTE_RLC_AMD_PDU_STRUCT *amd,
                                          LIBLTE_BIT_MSG_STRUCT     *data,
                                          LIBLTE_BIT_MSG_STRUCT     *pdu);
LIBLTE_ERROR_ENUM liblte_rlc_unpack_amd_pdu(LIBLTE_BIT_MSG_STRUCT     *pdu,
                                            LIBLTE_RLC_AMD_PDU_STRUCT *amd);

/*********************************************************************
    PDU Type: Status PDU

    Document Reference: 36.322 v10.0.0 Section 6.2.1.6
*********************************************************************/
// Defines
// Enums
// Structs
typedef struct{
    uint32 N_nack;
    uint16 ack_sn;
    uint16 nack_sn[LIBLTE_RLC_AM_WINDOW_SIZE];
}LIBLTE_RLC_STATUS_PDU_STRUCT;
// Functions
LIBLTE_ERROR_ENUM liblte_rlc_pack_status_pdu(LIBLTE_RLC_STATUS_PDU_STRUCT *status,
                                             LIBLTE_BIT_MSG_STRUCT        *pdu);
LIBLTE_ERROR_ENUM liblte_rlc_unpack_status_pdu(LIBLTE_BIT_MSG_STRUCT        *pdu,
                                               LIBLTE_RLC_STATUS_PDU_STRUCT *status);

#endif /* __LIBLTE_RLC_H__ */
