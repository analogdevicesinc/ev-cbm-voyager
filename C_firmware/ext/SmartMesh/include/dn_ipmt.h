/*
Copyright (c) 2015, Dust Networks. All rights reserved.

C library to connect to a SmartMesh IP Mote.

\license See attached DN_LICENSE.txt.
*/

#ifndef dn_ipmt_h
#define dn_ipmt_h

#include "dn_common.h"
#include "dn_endianness.h"
#include "dn_clib_version.h"

//=========================== defines =========================================

#define MAX_FRAME_LENGTH                    128
#define DN_SUBCMDID_NONE                    0xff

//===== well-known IPv6 address of the SmartMesh IP manager
static const uint8_t ipv6Addr_manager[16] = {
   0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02
};

//===== command IDs (requests)
#define CMDID_SETPARAMETER                  0x1
#define CMDID_GETPARAMETER                  0x2
#define CMDID_JOIN                          0x6
#define CMDID_DISCONNECT                    0x7
#define CMDID_RESET                         0x8
#define CMDID_LOWPOWERSLEEP                 0x9
#define CMDID_TESTRADIORX                   0xc
#define CMDID_CLEARNV                       0x10
#define CMDID_REQUESTSERVICE                0x11
#define CMDID_GETSERVICEINFO                0x12
#define CMDID_OPENSOCKET                    0x15
#define CMDID_CLOSESOCKET                   0x16
#define CMDID_BINDSOCKET                    0x17
#define CMDID_SENDTO                        0x18
#define CMDID_SEARCH                        0x24
#define CMDID_TESTRADIOTXEXT                0x28
#define CMDID_ZEROIZE                       0x29
#define CMDID_SOCKETINFO                    0x2b

//===== command IDs (notifications)
#define CMDID_TIMEINDICATION           0xd
#define CMDID_EVENTS                   0xf
#define CMDID_RECEIVE                  0x19
#define CMDID_MACRX                    0x24
#define CMDID_TXDONE                   0x25
#define CMDID_ADVRECEIVED              0x26

//===== parameter IDs
#define PARAMID_MACADDRESS             0x1
#define PARAMID_JOINKEY                0x2
#define PARAMID_NETWORKID              0x3
#define PARAMID_TXPOWER                0x4
#define PARAMID_JOINDUTYCYCLE          0x6
#define PARAMID_EVENTMASK              0xb
#define PARAMID_MOTEINFO               0xc
#define PARAMID_NETINFO                0xd
#define PARAMID_MOTESTATUS             0xe
#define PARAMID_TIME                   0xf
#define PARAMID_CHARGE                 0x10
#define PARAMID_TESTRADIORXSTATS       0x11
#define PARAMID_OTAPLOCKOUT            0x15
#define PARAMID_MOTEID                 0x17
#define PARAMID_IPV6ADDRESS            0x18
#define PARAMID_ROUTINGMODE            0x1d
#define PARAMID_APPINFO                0x1e
#define PARAMID_POWERSRCINFO           0x1f
#define PARAMID_ADVKEY                 0x22
#define PARAMID_AUTOJOIN               0x24

//===== format of requests

// setParameter_macAddress
#define DN_SETPARAMETER_MACADDRESS_REQ_OFFS_MACADDRESS               1
#define DN_SETPARAMETER_MACADDRESS_REQ_LEN                           9

// setParameter_joinKey
#define DN_SETPARAMETER_JOINKEY_REQ_OFFS_JOINKEY                     1
#define DN_SETPARAMETER_JOINKEY_REQ_LEN                              17

// setParameter_networkId
#define DN_SETPARAMETER_NETWORKID_REQ_OFFS_NETWORKID                 1
#define DN_SETPARAMETER_NETWORKID_REQ_LEN                            3

// setParameter_txPower
#define DN_SETPARAMETER_TXPOWER_REQ_OFFS_TXPOWER                     1
#define DN_SETPARAMETER_TXPOWER_REQ_LEN                              2

// setParameter_joinDutyCycle
#define DN_SETPARAMETER_JOINDUTYCYCLE_REQ_OFFS_DUTYCYCLE             1
#define DN_SETPARAMETER_JOINDUTYCYCLE_REQ_LEN                        2

// setParameter_eventMask
#define DN_SETPARAMETER_EVENTMASK_REQ_OFFS_EVENTMASK                 1
#define DN_SETPARAMETER_EVENTMASK_REQ_LEN                            5

// setParameter_OTAPLockout
#define DN_SETPARAMETER_OTAPLOCKOUT_REQ_OFFS_MODE                    1
#define DN_SETPARAMETER_OTAPLOCKOUT_REQ_LEN                          2

// setParameter_routingMode
#define DN_SETPARAMETER_ROUTINGMODE_REQ_OFFS_MODE                    1
#define DN_SETPARAMETER_ROUTINGMODE_REQ_LEN                          2

// setParameter_powerSrcInfo
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_MAXSTCURRENT           1
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_MINLIFETIME            3
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_0         4
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_0      6
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_0       8
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_1         10
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_1      12
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_1       14
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_2         16
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_2      18
#define DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_2       20
#define DN_SETPARAMETER_POWERSRCINFO_REQ_LEN                         22

// setParameter_advKey
#define DN_SETPARAMETER_ADVKEY_REQ_OFFS_ADVKEY                       1
#define DN_SETPARAMETER_ADVKEY_REQ_LEN                               17

// setParameter_autoJoin
#define DN_SETPARAMETER_AUTOJOIN_REQ_OFFS_MODE                       1
#define DN_SETPARAMETER_AUTOJOIN_REQ_LEN                             2

// getParameter_macAddress
#define DN_GETPARAMETER_MACADDRESS_REQ_LEN                           1

// getParameter_networkId
#define DN_GETPARAMETER_NETWORKID_REQ_LEN                            1

// getParameter_txPower
#define DN_GETPARAMETER_TXPOWER_REQ_LEN                              1

// getParameter_joinDutyCycle
#define DN_GETPARAMETER_JOINDUTYCYCLE_REQ_LEN                        1

// getParameter_eventMask
#define DN_GETPARAMETER_EVENTMASK_REQ_LEN                            1

// getParameter_moteInfo
#define DN_GETPARAMETER_MOTEINFO_REQ_LEN                             1

// getParameter_netInfo
#define DN_GETPARAMETER_NETINFO_REQ_LEN                              1

// getParameter_moteStatus
#define DN_GETPARAMETER_MOTESTATUS_REQ_LEN                           1

// getParameter_time
#define DN_GETPARAMETER_TIME_REQ_LEN                                 1

// getParameter_charge
#define DN_GETPARAMETER_CHARGE_REQ_LEN                               1

// getParameter_testRadioRxStats
#define DN_GETPARAMETER_TESTRADIORXSTATS_REQ_LEN                     1

// getParameter_OTAPLockout
#define DN_GETPARAMETER_OTAPLOCKOUT_REQ_LEN                          1

// getParameter_moteId
#define DN_GETPARAMETER_MOTEID_REQ_LEN                               1

// getParameter_ipv6Address
#define DN_GETPARAMETER_IPV6ADDRESS_REQ_LEN                          1

// getParameter_routingMode
#define DN_GETPARAMETER_ROUTINGMODE_REQ_LEN                          1

// getParameter_appInfo
#define DN_GETPARAMETER_APPINFO_REQ_LEN                              1

// getParameter_powerSrcInfo
#define DN_GETPARAMETER_POWERSRCINFO_REQ_LEN                         1

// getParameter_autoJoin
#define DN_GETPARAMETER_AUTOJOIN_REQ_LEN                             1

// join
#define DN_JOIN_REQ_LEN                                              0

// disconnect
#define DN_DISCONNECT_REQ_LEN                                        0

// reset
#define DN_RESET_REQ_LEN                                             0

// lowPowerSleep
#define DN_LOWPOWERSLEEP_REQ_LEN                                     0

// testRadioRx
#define DN_TESTRADIORX_REQ_OFFS_CHANNELMASK                          0
#define DN_TESTRADIORX_REQ_OFFS_TIME                                 2
#define DN_TESTRADIORX_REQ_OFFS_STATIONID                            4
#define DN_TESTRADIORX_REQ_LEN                                       5

// clearNV
#define DN_CLEARNV_REQ_LEN                                           0

// requestService
#define DN_REQUESTSERVICE_REQ_OFFS_DESTADDR                          0
#define DN_REQUESTSERVICE_REQ_OFFS_SERVICETYPE                       2
#define DN_REQUESTSERVICE_REQ_OFFS_VALUE                             3
#define DN_REQUESTSERVICE_REQ_LEN                                    7

// getServiceInfo
#define DN_GETSERVICEINFO_REQ_OFFS_DESTADDR                          0
#define DN_GETSERVICEINFO_REQ_OFFS_TYPE                              2
#define DN_GETSERVICEINFO_REQ_LEN                                    3

// openSocket
#define DN_OPENSOCKET_REQ_OFFS_PROTOCOL                              0
#define DN_OPENSOCKET_REQ_LEN                                        1

// closeSocket
#define DN_CLOSESOCKET_REQ_OFFS_SOCKETID                             0
#define DN_CLOSESOCKET_REQ_LEN                                       1

// bindSocket
#define DN_BINDSOCKET_REQ_OFFS_SOCKETID                              0
#define DN_BINDSOCKET_REQ_OFFS_PORT                                  1
#define DN_BINDSOCKET_REQ_LEN                                        3

// sendTo
#define DN_SENDTO_REQ_OFFS_SOCKETID                                  0
#define DN_SENDTO_REQ_OFFS_DESTIP                                    1
#define DN_SENDTO_REQ_OFFS_DESTPORT                                  17
#define DN_SENDTO_REQ_OFFS_SERVICETYPE                               19
#define DN_SENDTO_REQ_OFFS_PRIORITY                                  20
#define DN_SENDTO_REQ_OFFS_PACKETID                                  21
#define DN_SENDTO_REQ_OFFS_PAYLOAD                                   23
#define DN_SENDTO_REQ_LEN                                            23

// search
#define DN_SEARCH_REQ_LEN                                            0

// testRadioTxExt
#define DN_TESTRADIOTXEXT_REQ_OFFS_TESTTYPE                          0
#define DN_TESTRADIOTXEXT_REQ_OFFS_CHANMASK                          1
#define DN_TESTRADIOTXEXT_REQ_OFFS_REPEATCNT                         3
#define DN_TESTRADIOTXEXT_REQ_OFFS_TXPOWER                           5
#define DN_TESTRADIOTXEXT_REQ_OFFS_SEQSIZE                           6
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_1                           7
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_1                           8
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_2                           10
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_2                           11
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_3                           13
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_3                           14
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_4                           16
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_4                           17
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_5                           19
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_5                           20
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_6                           22
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_6                           23
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_7                           25
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_7                           26
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_8                           28
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_8                           29
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_9                           31
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_9                           32
#define DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_10                          34
#define DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_10                          35
#define DN_TESTRADIOTXEXT_REQ_OFFS_STATIONID                         37
#define DN_TESTRADIOTXEXT_REQ_LEN                                    38

// zeroize
#define DN_ZEROIZE_REQ_LEN                                           0

// socketInfo
#define DN_SOCKETINFO_REQ_OFFS_INDEX                                 0
#define DN_SOCKETINFO_REQ_LEN                                        1

//===== format of replies

// setParameter_macAddress
#define DN_SETPARAMETER_MACADDRESS_REPLY_LEN                         1

// setParameter_joinKey
#define DN_SETPARAMETER_JOINKEY_REPLY_LEN                            1

// setParameter_networkId
#define DN_SETPARAMETER_NETWORKID_REPLY_LEN                          1

// setParameter_txPower
#define DN_SETPARAMETER_TXPOWER_REPLY_LEN                            1

// setParameter_joinDutyCycle
#define DN_SETPARAMETER_JOINDUTYCYCLE_REPLY_LEN                      1

// setParameter_eventMask
#define DN_SETPARAMETER_EVENTMASK_REPLY_LEN                          1

// setParameter_OTAPLockout
#define DN_SETPARAMETER_OTAPLOCKOUT_REPLY_LEN                        1

// setParameter_routingMode
#define DN_SETPARAMETER_ROUTINGMODE_REPLY_LEN                        1

// setParameter_powerSrcInfo
#define DN_SETPARAMETER_POWERSRCINFO_REPLY_LEN                       1

// setParameter_advKey
#define DN_SETPARAMETER_ADVKEY_REPLY_LEN                             1

// setParameter_autoJoin
#define DN_SETPARAMETER_AUTOJOIN_REPLY_LEN                           1

// getParameter_macAddress
#define DN_GETPARAMETER_MACADDRESS_REPLY_OFFS_MACADDRESS             1
#define DN_GETPARAMETER_MACADDRESS_REPLY_LEN                         9

// getParameter_networkId
#define DN_GETPARAMETER_NETWORKID_REPLY_OFFS_NETWORKID               1
#define DN_GETPARAMETER_NETWORKID_REPLY_LEN                          3

// getParameter_txPower
#define DN_GETPARAMETER_TXPOWER_REPLY_OFFS_TXPOWER                   1
#define DN_GETPARAMETER_TXPOWER_REPLY_LEN                            2

// getParameter_joinDutyCycle
#define DN_GETPARAMETER_JOINDUTYCYCLE_REPLY_OFFS_JOINDUTYCYCLE       1
#define DN_GETPARAMETER_JOINDUTYCYCLE_REPLY_LEN                      2

// getParameter_eventMask
#define DN_GETPARAMETER_EVENTMASK_REPLY_OFFS_EVENTMASK               1
#define DN_GETPARAMETER_EVENTMASK_REPLY_LEN                          5

// getParameter_moteInfo
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_APIVERSION               1
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SERIALNUMBER             2
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_HWMODEL                  10
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_HWREV                    11
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERMAJOR               12
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERMINOR               13
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERPATCH               14
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERBUILD               15
#define DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_BOOTSWVER                17
#define DN_GETPARAMETER_MOTEINFO_REPLY_LEN                           18

// getParameter_netInfo
#define DN_GETPARAMETER_NETINFO_REPLY_OFFS_MACADDRESS                1
#define DN_GETPARAMETER_NETINFO_REPLY_OFFS_MOTEID                    9
#define DN_GETPARAMETER_NETINFO_REPLY_OFFS_NETWORKID                 11
#define DN_GETPARAMETER_NETINFO_REPLY_OFFS_SLOTSIZE                  13
#define DN_GETPARAMETER_NETINFO_REPLY_LEN                            15

// getParameter_moteStatus
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_STATE                  1
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_0             2
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_1             3
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_NUMPARENTS             5
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_ALARMS                 6
#define DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_2             10
#define DN_GETPARAMETER_MOTESTATUS_REPLY_LEN                         11

// getParameter_time
#define DN_GETPARAMETER_TIME_REPLY_OFFS_UPTIME                       1
#define DN_GETPARAMETER_TIME_REPLY_OFFS_UTCSECS                      5
#define DN_GETPARAMETER_TIME_REPLY_OFFS_UTCUSECS                     13
#define DN_GETPARAMETER_TIME_REPLY_OFFS_ASN                          17
#define DN_GETPARAMETER_TIME_REPLY_OFFS_ASNOFFSET                    22
#define DN_GETPARAMETER_TIME_REPLY_LEN                               24

// getParameter_charge
#define DN_GETPARAMETER_CHARGE_REPLY_OFFS_QTOTAL                     1
#define DN_GETPARAMETER_CHARGE_REPLY_OFFS_UPTIME                     5
#define DN_GETPARAMETER_CHARGE_REPLY_OFFS_TEMPINT                    9
#define DN_GETPARAMETER_CHARGE_REPLY_OFFS_TEMPFRAC                   10
#define DN_GETPARAMETER_CHARGE_REPLY_LEN                             11

// getParameter_testRadioRxStats
#define DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_OFFS_RXOK             1
#define DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_OFFS_RXFAILED         3
#define DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_LEN                   5

// getParameter_OTAPLockout
#define DN_GETPARAMETER_OTAPLOCKOUT_REPLY_OFFS_MODE                  1
#define DN_GETPARAMETER_OTAPLOCKOUT_REPLY_LEN                        2

// getParameter_moteId
#define DN_GETPARAMETER_MOTEID_REPLY_OFFS_MOTEID                     1
#define DN_GETPARAMETER_MOTEID_REPLY_LEN                             3

// getParameter_ipv6Address
#define DN_GETPARAMETER_IPV6ADDRESS_REPLY_OFFS_IPV6ADDRESS           1
#define DN_GETPARAMETER_IPV6ADDRESS_REPLY_LEN                        17

// getParameter_routingMode
#define DN_GETPARAMETER_ROUTINGMODE_REPLY_OFFS_ROUTINGMODE           1
#define DN_GETPARAMETER_ROUTINGMODE_REPLY_LEN                        2

// getParameter_appInfo
#define DN_GETPARAMETER_APPINFO_REPLY_OFFS_VENDORID                  1
#define DN_GETPARAMETER_APPINFO_REPLY_OFFS_APPID                     3
#define DN_GETPARAMETER_APPINFO_REPLY_OFFS_APPVER                    4
#define DN_GETPARAMETER_APPINFO_REPLY_LEN                            9

// getParameter_powerSrcInfo
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_MAXSTCURRENT         1
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_MINLIFETIME          3
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_0       4
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_0    6
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_0     8
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_1       10
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_1    12
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_1     14
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_2       16
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_2    18
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_2     20
#define DN_GETPARAMETER_POWERSRCINFO_REPLY_LEN                       22

// getParameter_autoJoin
#define DN_GETPARAMETER_AUTOJOIN_REPLY_OFFS_AUTOJOIN                 1
#define DN_GETPARAMETER_AUTOJOIN_REPLY_LEN                           2

// join
#define DN_JOIN_REPLY_LEN                                            0

// disconnect
#define DN_DISCONNECT_REPLY_LEN                                      0

// reset
#define DN_RESET_REPLY_LEN                                           0

// lowPowerSleep
#define DN_LOWPOWERSLEEP_REPLY_LEN                                   0

// testRadioRx
#define DN_TESTRADIORX_REPLY_LEN                                     0

// clearNV
#define DN_CLEARNV_REPLY_LEN                                         0

// requestService
#define DN_REQUESTSERVICE_REPLY_LEN                                  0

// getServiceInfo
#define DN_GETSERVICEINFO_REPLY_OFFS_DESTADDR                        0
#define DN_GETSERVICEINFO_REPLY_OFFS_TYPE                            2
#define DN_GETSERVICEINFO_REPLY_OFFS_STATE                           3
#define DN_GETSERVICEINFO_REPLY_OFFS_VALUE                           4
#define DN_GETSERVICEINFO_REPLY_LEN                                  8

// openSocket
#define DN_OPENSOCKET_REPLY_OFFS_SOCKETID                            0
#define DN_OPENSOCKET_REPLY_LEN                                      1

// closeSocket
#define DN_CLOSESOCKET_REPLY_LEN                                     0

// bindSocket
#define DN_BINDSOCKET_REPLY_LEN                                      0

// sendTo
#define DN_SENDTO_REPLY_LEN                                          0

// search
#define DN_SEARCH_REPLY_LEN                                          0

// testRadioTxExt
#define DN_TESTRADIOTXEXT_REPLY_LEN                                  0

// zeroize
#define DN_ZEROIZE_REPLY_LEN                                         0

// socketInfo
#define DN_SOCKETINFO_REPLY_OFFS_INDEX                               0
#define DN_SOCKETINFO_REPLY_OFFS_SOCKETID                            1
#define DN_SOCKETINFO_REPLY_OFFS_PROTOCOL                            2
#define DN_SOCKETINFO_REPLY_OFFS_BINDSTATE                           3
#define DN_SOCKETINFO_REPLY_OFFS_PORT                                4
#define DN_SOCKETINFO_REPLY_LEN                                      6

//===== format of notifications

// timeIndication
#define DN_TIMEINDICATION_NOTIF_OFFS_UPTIME                          0
#define DN_TIMEINDICATION_NOTIF_OFFS_UTCSECS                         4
#define DN_TIMEINDICATION_NOTIF_OFFS_UTCUSECS                        12
#define DN_TIMEINDICATION_NOTIF_OFFS_ASN                             16
#define DN_TIMEINDICATION_NOTIF_OFFS_ASNOFFSET                       21
#define DN_TIMEINDICATION_NOTIF_LEN                                  23

// events
#define DN_EVENTS_NOTIF_OFFS_EVENTS                                  0
#define DN_EVENTS_NOTIF_OFFS_STATE                                   4
#define DN_EVENTS_NOTIF_OFFS_ALARMSLIST                              5
#define DN_EVENTS_NOTIF_LEN                                          9

// receive
#define DN_RECEIVE_NOTIF_OFFS_SOCKETID                               0
#define DN_RECEIVE_NOTIF_OFFS_SRCADDR                                1
#define DN_RECEIVE_NOTIF_OFFS_SRCPORT                                17
#define DN_RECEIVE_NOTIF_OFFS_PAYLOAD                                19
#define DN_RECEIVE_NOTIF_LEN                                         19

// macRx
#define DN_MACRX_NOTIF_OFFS_PAYLOAD                                  0
#define DN_MACRX_NOTIF_LEN                                           0

// txDone
#define DN_TXDONE_NOTIF_OFFS_PACKETID                                0
#define DN_TXDONE_NOTIF_OFFS_STATUS                                  2
#define DN_TXDONE_NOTIF_LEN                                          3

// advReceived
#define DN_ADVRECEIVED_NOTIF_OFFS_NETID                              0
#define DN_ADVRECEIVED_NOTIF_OFFS_MOTEID                             2
#define DN_ADVRECEIVED_NOTIF_OFFS_RSSI                               4
#define DN_ADVRECEIVED_NOTIF_OFFS_JOINPRI                            5
#define DN_ADVRECEIVED_NOTIF_LEN                                     6

//=========================== typedef =========================================

//=== reply types

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_macAddress_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_joinKey_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_networkId_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_txPower_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_joinDutyCycle_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_eventMask_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_OTAPLockout_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_routingMode_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_powerSrcInfo_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_advKey_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_setParameter_autoJoin_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    macAddress[8];
} dn_ipmt_getParameter_macAddress_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   networkId;
} dn_ipmt_getParameter_networkId_rpt;

typedef struct {
   uint8_t    RC;
   int8_t     txPower;
} dn_ipmt_getParameter_txPower_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    joinDutyCycle;
} dn_ipmt_getParameter_joinDutyCycle_rpt;

typedef struct {
   uint8_t    RC;
   uint32_t   eventMask;
} dn_ipmt_getParameter_eventMask_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    apiVersion;
   uint8_t    serialNumber[8];
   uint8_t    hwModel;
   uint8_t    hwRev;
   uint8_t    swVerMajor;
   uint8_t    swVerMinor;
   uint8_t    swVerPatch;
   uint16_t   swVerBuild;
   uint8_t    bootSwVer;
} dn_ipmt_getParameter_moteInfo_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    macAddress[8];
   uint16_t   moteId;
   uint16_t   networkId;
   uint16_t   slotSize;
} dn_ipmt_getParameter_netInfo_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    state;
   uint8_t    reserved_0;
   uint16_t   reserved_1;
   uint8_t    numParents;
   uint32_t   alarms;
   uint8_t    reserved_2;
} dn_ipmt_getParameter_moteStatus_rpt;

typedef struct {
   uint8_t    RC;
   uint32_t   upTime;
   uint8_t    utcSecs[8];
   uint32_t   utcUsecs;
   uint8_t    asn[5];
   uint16_t   asnOffset;
} dn_ipmt_getParameter_time_rpt;

typedef struct {
   uint8_t    RC;
   uint32_t   qTotal;
   uint32_t   upTime;
   int8_t     tempInt;
   uint8_t    tempFrac;
} dn_ipmt_getParameter_charge_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   rxOk;
   uint16_t   rxFailed;
} dn_ipmt_getParameter_testRadioRxStats_rpt;

typedef struct {
   uint8_t    RC;
   bool       mode;
} dn_ipmt_getParameter_OTAPLockout_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   moteId;
} dn_ipmt_getParameter_moteId_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    ipv6Address[16];
} dn_ipmt_getParameter_ipv6Address_rpt;

typedef struct {
   uint8_t    RC;
   bool       routingMode;
} dn_ipmt_getParameter_routingMode_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   vendorId;
   uint8_t    appId;
   uint8_t    appVer[5];
} dn_ipmt_getParameter_appInfo_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   maxStCurrent;
   uint8_t    minLifetime;
   uint16_t   currentLimit_0;
   uint16_t   dischargePeriod_0;
   uint16_t   rechargePeriod_0;
   uint16_t   currentLimit_1;
   uint16_t   dischargePeriod_1;
   uint16_t   rechargePeriod_1;
   uint16_t   currentLimit_2;
   uint16_t   dischargePeriod_2;
   uint16_t   rechargePeriod_2;
} dn_ipmt_getParameter_powerSrcInfo_rpt;

typedef struct {
   uint8_t    RC;
   bool       autoJoin;
} dn_ipmt_getParameter_autoJoin_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_join_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_disconnect_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_reset_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_lowPowerSleep_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_testRadioRx_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_clearNV_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_requestService_rpt;

typedef struct {
   uint8_t    RC;
   uint16_t   destAddr;
   uint8_t    type;
   uint8_t    state;
   uint32_t   value;
} dn_ipmt_getServiceInfo_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    socketId;
} dn_ipmt_openSocket_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_closeSocket_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_bindSocket_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_sendTo_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_search_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_testRadioTxExt_rpt;

typedef struct {
   uint8_t    RC;
} dn_ipmt_zeroize_rpt;

typedef struct {
   uint8_t    RC;
   uint8_t    index;
   uint8_t    socketId;
   uint8_t    protocol;
   uint8_t    bindState;
   uint16_t   port;
} dn_ipmt_socketInfo_rpt;

//=== notification types

typedef struct {
   uint32_t   uptime;
   uint8_t    utcSecs[8];
   uint32_t   utcUsecs;
   uint8_t    asn[5];
   uint16_t   asnOffset;
} dn_ipmt_timeIndication_nt;

typedef struct {
   uint32_t   events;
   uint8_t    state;
   uint32_t   alarmsList;
} dn_ipmt_events_nt;

typedef struct {
   uint8_t    socketId;
   uint8_t    srcAddr[16];
   uint16_t   srcPort;
   uint8_t    payloadLen;
   uint8_t    payload[MAX_FRAME_LENGTH];
} dn_ipmt_receive_nt;

typedef struct {
   uint8_t    payload[MAX_FRAME_LENGTH];
} dn_ipmt_macRx_nt;

typedef struct {
   uint16_t   packetId;
   uint8_t    status;
} dn_ipmt_txDone_nt;

typedef struct {
   uint16_t   netId;
   uint16_t   moteId;
   int8_t     rssi;
   uint8_t    joinPri;
} dn_ipmt_advReceived_nt;

//=== callback signature
typedef void (*dn_ipmt_notif_cbt)(uint8_t cmdId, uint8_t subCmdId);
typedef void (*dn_ipmt_reply_cbt)(uint8_t cmdId);
typedef void (*dn_ipmt_status_cbt)(uint8_t newStatus); // only used in SmartMesh IP manager

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

//==== admin
void     dn_ipmt_init(dn_ipmt_notif_cbt notifCb, uint8_t* notifBuf, uint8_t notifBufLen, dn_ipmt_reply_cbt replyCb);
void     dn_ipmt_cancelTx();


//==== API
dn_err_t dn_ipmt_setParameter_macAddress(uint8_t* macAddress, dn_ipmt_setParameter_macAddress_rpt* reply);
dn_err_t dn_ipmt_setParameter_joinKey(uint8_t* joinKey, dn_ipmt_setParameter_joinKey_rpt* reply);
dn_err_t dn_ipmt_setParameter_networkId(uint16_t networkId, dn_ipmt_setParameter_networkId_rpt* reply);
dn_err_t dn_ipmt_setParameter_txPower(int8_t txPower, dn_ipmt_setParameter_txPower_rpt* reply);
dn_err_t dn_ipmt_setParameter_joinDutyCycle(uint8_t dutyCycle, dn_ipmt_setParameter_joinDutyCycle_rpt* reply);
dn_err_t dn_ipmt_setParameter_eventMask(uint32_t eventMask, dn_ipmt_setParameter_eventMask_rpt* reply);
dn_err_t dn_ipmt_setParameter_OTAPLockout(bool mode, dn_ipmt_setParameter_OTAPLockout_rpt* reply);
dn_err_t dn_ipmt_setParameter_routingMode(bool mode, dn_ipmt_setParameter_routingMode_rpt* reply);
dn_err_t dn_ipmt_setParameter_powerSrcInfo(uint16_t maxStCurrent, uint8_t minLifetime, uint16_t currentLimit_0, uint16_t dischargePeriod_0, uint16_t rechargePeriod_0, uint16_t currentLimit_1, uint16_t dischargePeriod_1, uint16_t rechargePeriod_1, uint16_t currentLimit_2, uint16_t dischargePeriod_2, uint16_t rechargePeriod_2, dn_ipmt_setParameter_powerSrcInfo_rpt* reply);
dn_err_t dn_ipmt_setParameter_advKey(uint8_t* advKey, dn_ipmt_setParameter_advKey_rpt* reply);
dn_err_t dn_ipmt_setParameter_autoJoin(bool mode, dn_ipmt_setParameter_autoJoin_rpt* reply);
dn_err_t dn_ipmt_getParameter_macAddress(dn_ipmt_getParameter_macAddress_rpt* reply);
dn_err_t dn_ipmt_getParameter_networkId(dn_ipmt_getParameter_networkId_rpt* reply);
dn_err_t dn_ipmt_getParameter_txPower(dn_ipmt_getParameter_txPower_rpt* reply);
dn_err_t dn_ipmt_getParameter_joinDutyCycle(dn_ipmt_getParameter_joinDutyCycle_rpt* reply);
dn_err_t dn_ipmt_getParameter_eventMask(dn_ipmt_getParameter_eventMask_rpt* reply);
dn_err_t dn_ipmt_getParameter_moteInfo(dn_ipmt_getParameter_moteInfo_rpt* reply);
dn_err_t dn_ipmt_getParameter_netInfo(dn_ipmt_getParameter_netInfo_rpt* reply);
dn_err_t dn_ipmt_getParameter_moteStatus(dn_ipmt_getParameter_moteStatus_rpt* reply);
dn_err_t dn_ipmt_getParameter_time(dn_ipmt_getParameter_time_rpt* reply);
dn_err_t dn_ipmt_getParameter_charge(dn_ipmt_getParameter_charge_rpt* reply);
dn_err_t dn_ipmt_getParameter_testRadioRxStats(dn_ipmt_getParameter_testRadioRxStats_rpt* reply);
dn_err_t dn_ipmt_getParameter_OTAPLockout(dn_ipmt_getParameter_OTAPLockout_rpt* reply);
dn_err_t dn_ipmt_getParameter_moteId(dn_ipmt_getParameter_moteId_rpt* reply);
dn_err_t dn_ipmt_getParameter_ipv6Address(dn_ipmt_getParameter_ipv6Address_rpt* reply);
dn_err_t dn_ipmt_getParameter_routingMode(dn_ipmt_getParameter_routingMode_rpt* reply);
dn_err_t dn_ipmt_getParameter_appInfo(dn_ipmt_getParameter_appInfo_rpt* reply);
dn_err_t dn_ipmt_getParameter_powerSrcInfo(dn_ipmt_getParameter_powerSrcInfo_rpt* reply);
dn_err_t dn_ipmt_getParameter_autoJoin(dn_ipmt_getParameter_autoJoin_rpt* reply);
dn_err_t dn_ipmt_join(dn_ipmt_join_rpt* reply);
dn_err_t dn_ipmt_disconnect(dn_ipmt_disconnect_rpt* reply);
dn_err_t dn_ipmt_reset(dn_ipmt_reset_rpt* reply);
dn_err_t dn_ipmt_lowPowerSleep(dn_ipmt_lowPowerSleep_rpt* reply);
dn_err_t dn_ipmt_testRadioRx(uint16_t channelMask, uint16_t time, uint8_t stationId, dn_ipmt_testRadioRx_rpt* reply);
dn_err_t dn_ipmt_clearNV(dn_ipmt_clearNV_rpt* reply);
dn_err_t dn_ipmt_requestService(uint16_t destAddr, uint8_t serviceType, uint32_t value, dn_ipmt_requestService_rpt* reply);
dn_err_t dn_ipmt_getServiceInfo(uint16_t destAddr, uint8_t type, dn_ipmt_getServiceInfo_rpt* reply);
dn_err_t dn_ipmt_openSocket(uint8_t protocol, dn_ipmt_openSocket_rpt* reply);
dn_err_t dn_ipmt_closeSocket(uint8_t socketId, dn_ipmt_closeSocket_rpt* reply);
dn_err_t dn_ipmt_bindSocket(uint8_t socketId, uint16_t port, dn_ipmt_bindSocket_rpt* reply);
dn_err_t dn_ipmt_sendTo(uint8_t socketId, uint8_t* destIP, uint16_t destPort, uint8_t serviceType, uint8_t priority, uint16_t packetId, uint8_t* payload, uint8_t payloadLen, dn_ipmt_sendTo_rpt* reply);
dn_err_t dn_ipmt_search(dn_ipmt_search_rpt* reply);
dn_err_t dn_ipmt_testRadioTxExt(uint8_t testType, uint16_t chanMask, uint16_t repeatCnt, int8_t txPower, uint8_t seqSize, uint8_t pkLen_1, uint16_t delay_1, uint8_t pkLen_2, uint16_t delay_2, uint8_t pkLen_3, uint16_t delay_3, uint8_t pkLen_4, uint16_t delay_4, uint8_t pkLen_5, uint16_t delay_5, uint8_t pkLen_6, uint16_t delay_6, uint8_t pkLen_7, uint16_t delay_7, uint8_t pkLen_8, uint16_t delay_8, uint8_t pkLen_9, uint16_t delay_9, uint8_t pkLen_10, uint16_t delay_10, uint8_t stationId, dn_ipmt_testRadioTxExt_rpt* reply);
dn_err_t dn_ipmt_zeroize(dn_ipmt_zeroize_rpt* reply);
dn_err_t dn_ipmt_socketInfo(uint8_t index, dn_ipmt_socketInfo_rpt* reply);

#ifdef __cplusplus
}
#endif

#endif
