#ifndef RADIUS_ATTR_PARSE_H
#define RADIUS_ATTR_PARSE_H

#include <stdbool.h>

#define L7_AF_INET           1  
#define L7_AF_INET6          2



#define RADIUS_VALUE_LENGTH             253

#define RADIUS_ATTR_TYPE_USER_NAME                   1
#define RADIUS_ATTR_TYPE_USER_PASSWORD               2
#define RADIUS_ATTR_TYPE_CHAP_PASSWORD               3
#define RADIUS_ATTR_TYPE_NAS_IP_ADDRESS              4
#define RADIUS_ATTR_TYPE_NAS_PORT                    5
#define RADIUS_ATTR_TYPE_SERVICE_TYPE                6
#define RADIUS_ATTR_TYPE_FRAMED_PROTOCOL             7
#define RADIUS_ATTR_TYPE_FRAMED_IP_ADDRESS           8
#define RADIUS_ATTR_TYPE_FRAMED_IP_NETMASK           9
#define RADIUS_ATTR_TYPE_FRAMED_ROUTING             10
#define RADIUS_ATTR_TYPE_FILTER_ID                  11
#define RADIUS_ATTR_TYPE_FRAMED_MTU                 12
#define RADIUS_ATTR_TYPE_FRAMED_COMPRESSION         13
#define RADIUS_ATTR_TYPE_LOGIN_IP_HOST              14
#define RADIUS_ATTR_TYPE_LOGIN_SERVICE              15
#define RADIUS_ATTR_TYPE_LOGIN_TCP_PORT             16

#define RADIUS_ATTR_TYPE_REPLY_MESSAGE              18
#define RADIUS_ATTR_TYPE_CALLBACK_NUMBER            19
#define RADIUS_ATTR_TYPE_CALLBACK_ID                20

#define RADIUS_ATTR_TYPE_FRAMED_ROUTE               22
#define RADIUS_ATTR_TYPE_FRAMED_IPX_NETWORK         23
#define RADIUS_ATTR_TYPE_STATE                      24
#define RADIUS_ATTR_TYPE_CLASS                      25
#define RADIUS_ATTR_TYPE_VENDOR                     26
#define RADIUS_ATTR_TYPE_SESSION_TIMEOUT            27
#define RADIUS_ATTR_TYPE_IDLE_TIMEOUT               28
#define RADIUS_ATTR_TYPE_TERMINATION_ACTION         29
#define RADIUS_ATTR_TYPE_CALLED_STATION_ID          30
#define RADIUS_ATTR_TYPE_CALLING_STATION_ID         31
#define RADIUS_ATTR_TYPE_NAS_IDENTIFIER             32
#define RADIUS_ATTR_TYPE_PROXY_STATE                33
#define RADIUS_ATTR_TYPE_LOGIN_LAT_SERVICE          34
#define RADIUS_ATTR_TYPE_LOGIN_LAT_NODE             35
#define RADIUS_ATTR_TYPE_LOGIN_LAT_GROUP            36
#define RADIUS_ATTR_TYPE_FRAMED_APPLETALK_LINK      37
#define RADIUS_ATTR_TYPE_FRAMED_APPLETALK_NETWORK   38
#define RADIUS_ATTR_TYPE_FRAMED_APPLETALK_ZONE      39
#define RADIUS_ATTR_TYPE_ACCT_STATUS_TYPE           40
#define RADIUS_ATTR_TYPE_ACCT_DELAY_TIME            41
#define RADIUS_ATTR_TYPE_ACCT_INPUT_OCTETS          42
#define RADIUS_ATTR_TYPE_ACCT_OUTPUT_OCTETS         43
#define RADIUS_ATTR_TYPE_ACCT_SESSION_ID            44
#define RADIUS_ATTR_TYPE_ACCT_AUTHENTIC             45
#define RADIUS_ATTR_TYPE_ACCT_SESSION_TIME          46
#define RADIUS_ATTR_TYPE_ACCT_INPUT_PACKETS         47
#define RADIUS_ATTR_TYPE_ACCT_OUTPUT_PACKETS        48
#define RADIUS_ATTR_TYPE_ACCT_TERMINATE_CAUSE       49
#define RADIUS_ATTR_TYPE_ACCT_MULTI_SESSION_ID      50
#define RADIUS_ATTR_TYPE_ACCT_LINK_COUNT            51
#define RADIUS_ATTR_TYPE_ACCT_G_IBYTES              52
#define RADIUS_ATTR_TYPE_ACCT_G_OBYTES              53

#define RADIUS_ATTR_TYPE_EVENT_TIMESTAMP            55

#define RADIUS_ATTR_TYPE_CHAP_CHALLENGE             60
#define RADIUS_ATTR_TYPE_NAS_PORT_TYPE              61
#define RADIUS_ATTR_TYPE_PORT_LIMIT                 62
#define RADIUS_ATTR_TYPE_LOGIN_LAST_PORT            63
#define RADIUS_ATTR_TYPE_TUNNEL_TYPE                64
#define RADIUS_ATTR_TYPE_TUNNEL_MEDIUM_TYPE         65

#define RADIUS_ATTR_TYPE_CONNECT_INFO               77

#define RADIUS_ATTR_TYPE_EAP_MESSAGE                79
#define RADIUS_ATTR_TYPE_MESSAGE_AUTHENTICATOR      80
#define RADIUS_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID    81
#define RADIUS_ATTR_TYPE_NAS_PORT_ID                87
#define RADIUS_ATTR_TYPE_NAS_FILTER_RULE            92 
#define RADIUS_ATTR_TYPE_NAS_IPV6_ADDRESS           95
#define RADIUS_ATTR_TYPE_EAP_KEY_NAME               102
#define RADIUS_ATTR_TYPE_FRAMED_IPV6_ADDRESS        168

#define RADIUS_ATTR_TYPE_VLAN_ID                    199


#define RADIUS_ATTR_SIZE_SERVICE_TYPE             6
/*
** The Service-Type value codes
*/
#define RADIUS_SERVICE_TYPE_LOGIN                 1
#define RADIUS_SERVICE_TYPE_FRAMED                2
#define RADIUS_SERVICE_TYPE_CALLBACK_LOGIN        3
#define RADIUS_SERVICE_TYPE_CALLBACK_FRAMED       4
#define RADIUS_SERVICE_TYPE_OUTBOUND              5
#define RADIUS_SERVICE_TYPE_ADMIN                 6
#define RADIUS_SERVICE_TYPE_NAS_PROMPT            7
#define RADIUS_SERVICE_TYPE_AUTHEN_ONLY           8
#define RADIUS_SERVICE_TYPE_CALLBACK_NAS_PROMPT   9
#define RADIUS_SERVICE_TYPE_CALL_CHECK            10

/*
** The Framed Protocol value types
*/
#define RADIUS_FRAMED_PROTOCOL_PPP      1
#define RADIUS_FRAMED_PROTOCOL_SLIP     2
#define RADIUS_FRAMED_PROTOCOL_ARAP     3
#define RADIUS_FRAMED_PROTOCOL_SLP_MLP  4
#define RADIUS_FRAMED_PROTOCOL_IPX_SLIP 5

/*
** The Framed Routing value types
*/
#define RADIUS_FRAMED_ROUTING_NONE        0
#define RADIUS_FRAMED_ROUTING_SEND        1
#define RADIUS_FRAMED_ROUTING_LISTEN      2
#define RADIUS_FRAMED_ROUTING_SEND_LISTEN 3

/*
** The Framed Compression value types
*/
#define RADIUS_FRAMED_COMPRESSION_NONE        0
#define RADIUS_FRAMED_COMPRESSION_VJ_TCPIP    1
#define RADIUS_FRAMED_COMPRESSION_IPX_HDR     2

/*
** The Login Service value codes
*/
#define RADIUS_LOGIN_SERVICE_TELNET      0
#define RADIUS_LOGIN_SERVICE_RLOGIN      1
#define RADIUS_LOGIN_SERVICE_TCP_CLEAR   2
#define RADIUS_LOGIN_SERVICE_PORT_MASTER 3
#define RADIUS_LOGIN_SERVICE_LAST        4

/*
** The Termination Action value codes
*/
#define RADIUS_TERMINATION_ACTION_DEFAULT  0
#define RADIUS_TERMINATION_ACTION_RADIUS   1

/*
** NAS Port Types
*/
#define RADIUS_NAS_PORT_TYPE_ASYNC              0
#define RADIUS_NAS_PORT_TYPE_SYNC               1
#define RADIUS_NAS_PORT_TYPE_ISDN_SYNC          2
#define RADIUS_NAS_PORT_TYPE_ISDN_SYNC_V120     3
#define RADIUS_NAS_PORT_TYPE_ISDN_SYNC_V110     4
#define RADIUS_NAS_PORT_TYPE_VIRTUAL            5
#define RADIUS_NAS_PORT_TYPE_PIAFS              6
#define RADIUS_NAS_PORT_TYPE_HDLC_CLEAR_CHANNEL 7
#define RADIUS_NAS_PORT_TYPE_X25                8
#define RADIUS_NAS_PORT_TYPE_X75                9
#define RADIUS_NAS_PORT_TYPE_G3_FAX             10
#define RADIUS_NAS_PORT_TYPE_SDSL               11
#define RADIUS_NAS_PORT_TYPE_ADSL_CAP           12
#define RADIUS_NAS_PORT_TYPE_ADSL_DMT           13
#define RADIUS_NAS_PORT_TYPE_IDSL               14
#define RADIUS_NAS_PORT_TYPE_ETHERNET           15
#define RADIUS_NAS_PORT_TYPE_XDSL               16
#define RADIUS_NAS_PORT_TYPE_CABLE              17
#define RADIUS_NAS_PORT_TYPE_WIRELESS_OTHER     18
#define RADIUS_NAS_PORT_TYPE_WIRELESS_802_11    19

/*
** The Account-Status-Type value codes
*/
#define RADIUS_ACCT_STATUS_TYPE_START             1
#define RADIUS_ACCT_STATUS_TYPE_STOP              2
#define RADIUS_ACCT_STATUS_TYPE_INTERIM           3

/*
** Radius Account Authentic value codes
*/
#define RADIUS_ACCT_AUTHENTIC_RADIUS    1
#define RADIUS_ACCT_AUTHENTIC_LOCAL     2
#define RADIUS_ACCT_AUTHENTIC_REMOTE    3

/*
** The Accounting Termination Cause value codes
*/
#define RADIUS_ACCT_TERM_CAUSE_USER_REQUEST                   1
#define RADIUS_ACCT_TERM_CAUSE_LOST_CARRIER                   2
#define RADIUS_ACCT_TERM_CAUSE_LOST_SERVICE                   3
#define RADIUS_ACCT_TERM_CAUSE_IDLE_TIMEOUT                   4
#define RADIUS_ACCT_TERM_CAUSE_SESSION_TIMEOUT                5
#define RADIUS_ACCT_TERM_CAUSE_ADMIN_RESET                    6
#define RADIUS_ACCT_TERM_CAUSE_ADMIN_REBOOT                   7
#define RADIUS_ACCT_TERM_CAUSE_PORT_ERROR                     8
#define RADIUS_ACCT_TERM_CAUSE_NAS_ERROR                      9
#define RADIUS_ACCT_TERM_CAUSE_NAS_REQUEST                    10
#define RADIUS_ACCT_TERM_CAUSE_NAS_REBOOT                     11
#define RADIUS_ACCT_TERM_CAUSE_PORT_UNNEEDED                  12
#define RADIUS_ACCT_TERM_CAUSE_PORT_PREEMPTED                 13
#define RADIUS_ACCT_TERM_CAUSE_PORT_SUSPENDED                 14
#define RADIUS_ACCT_TERM_CAUSE_SERVICE_UNAVAILABLE            15
#define RADIUS_ACCT_TERM_CAUSE_CALLBACK                       16
#define RADIUS_ACCT_TERM_CAUSE_USER_ERROR                     17
#define RADIUS_ACCT_TERM_CAUSE_HOST_REQUEST                   18
#define RADIUS_ACCT_TERM_CAUSE_SUPPLICANT_RESTART             19
#define RADIUS_ACCT_TERM_CAUSE_REAUTHENTICATION_FAILURE       20
#define RADIUS_ACCT_TERM_CAUSE_PORT_REINITIALIZED             21
#define RADIUS_ACCT_TERM_CAUSE_PORT_ADMINISTRATIVELY_DISABLED 22

/*
** The types of the attribute values in storage representation
*/
#define RADIUS_ATTR_VALUE_TYPE_STRING     1
#define RADIUS_ATTR_VALUE_TYPE_INTEGER    2
#define RADIUS_ATTR_VALUE_TYPE_IP_ADDR    3
#define RADIUS_ATTR_VALUE_TYPE_DATE       4

/*
** The LVL7 vendor-specific wireless attribute types
*/
#define LVL7_MAX_INPUT_OCTETS_VATTR                         124
#define LVL7_MAX_OUTPUT_OCTETS_VATTR                        125
#define LVL7_MAX_TOTAL_OCTETS_VATTR                         126
#define LVL7_CAPTIVE_PORTAL_GROUPS_VATTR                    127

/*
 ** Microsoft vendor specific attribute
*/
#define RADIUS_VENDOR_ID_MICROSOFT                      311
#if 0
#define RADIUS_VENDOR_ATTR_MS_MPPE_SEND_KEY             16
#define RADIUS_VENDOR_ATTR_MS_MPPE_RECV_KEY             17
#endif
/*
 ** Wireless ISP - roaming (WISPr) vendor specific attributes
*/
#define RADIUS_VENDOR_ID_WISPR                          14122

#define RADIUS_VENDOR_ATTR_WISPR_BANDWIDTH_MAX_UP       7
#define RADIUS_VENDOR_ATTR_WISPR_BANDWIDTH_MAX_DOWN     8


/* The type of attribute values for Tunnel Type attribute
*/
#define RADIUS_TUNNEL_TYPE_VLAN     13


/* The type of attribute values for Tunnel Medium type attribute
*/
#define RADIUS_TUNNEL_MEDIUM_TYPE_802    6

#define RADIUS_ATTR_TYPE_TUNNEL_TYPE_SPECIFIED              0x1
#define RADIUS_ATTR_TYPE_TUNNEL_MEDIUM_TYPE_SPECIFIED       0x2
#define RADIUS_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID_SPECIFIED  0x4
#define RADIUS_REQUIRED_TUNNEL_ATTRIBUTES_SPECIFIED         0x7

#define RADIUS_TOKEN_LENGTH              32

#define RADIUS_AUTHENTICATOR_LENGTH      16

#define RADIUS_MS_KEY_SIZE               256

#define RADIUS_MS_KEY_SALT_LEN           2

#define RADIUS_VENDOR_ID_SIZE            4

#define RADIUS_VEND_ATTR_VEND_TYPE_SIZE  1
#define RADIUS_VEND_ATTR_VEND_LEN_SIZE   1
#define RADIUS_VEND_ATTR_HEAD_SIZE       (RADIUS_VENDOR_ID_SIZE + \
                                          RADIUS_VEND_ATTR_VEND_TYPE_SIZE + \
                                          RADIUS_VEND_ATTR_VEND_LEN_SIZE)

/*
 * Cisco vendor specific attributes
 */
#define RADIUS_VENDOR_ID_CISCO           9
#define RADIUS_VENDOR_ATTR_CISCO_AV_PAIR 1
#define RADIUS_VENDOR_ROLES_ATTR         "shell:roles"
#define RADIUS_VENDOR_PRIV_LVL_ATTR      "shell:priv-lvl="



#define VENDOR_ATTR_LEN 256

/*
 * ** The Termination Action value codes
 * */
#define TERMINATION_ACTION_DEFAULT  0
#define TERMINATION_ACTION_RADIUS   1
  
  
#define SERVER_STATE_LEN 253
#define SERVER_CLASS_LEN 253
  

#define FILTER_NAME_LEN 256
#define ACL_NAME_LEN 256
#define MAX_ACL_RULES_SIZE 4096

#define RADIUS_VLAN_ASSIGNED_LEN  32 /* Radius Assigned vlan length */
#define ACL_NAME_LEN_MAX  128

 /* Supports 1 Ipv4 and 1 Ipv6 and each name should be appended by \n as the delimiter and 1 character for null termination. */
#define MAX_REDIRECT_ACL_SIZE 2 * (ACL_NAME_LEN_MAX + 1 + 1)

#define ACLS_MAX 4 /* none, ipv4, ipv6 and mac */


#define MAX_ACL_RULES_STRING_SIZE 255 
#define MAX_ACL_RULES  128
#define RULES_BUFF_MAX    MAX_ACL_RULES*MAX_ACL_RULES_STRING_SIZE 

#define MAX_FRAMED_IPV6_ADDRESS_SIZE 16

#define REDIRECT_URL_LEN 256
#define EAP_KEY_NAME_LEN 256 
#define RADIUS_MS_KEY_SIZE 256 /* This value should be same as that of the RADIUS_MS_KEY_SIZE value */
#define VENDOR_ATTR_LEN 256

#define VENDOR_ATTR_LEN       256

#define  RADIUS_VENDOR_9   0
#define  RADIUS_VENDOR_311   1

#define MAX_VENDOR_IDS 2 /* 0 -none, cisco , msft */


#define LOGF syslog


#define EAP_AWARE_CLIENT 1
#define EAP_UNAWARE_CLIENT 2

#define   MAB_AUTH_TYPE_EAP_MD5 1
#define   MAB_AUTH_TYPE_PAP 2
#define   MAB_AUTH_TYPE_CHAP 3

#define RADIUS_VENDOR_DEVICE_CLASS_VOICE    "device-traffic-class=voice"
#define RADIUS_VENDOR_DEVICE_CLASS_SWITCH    "device-traffic-class=switch"


           
/* MACsec attribute */
#define RADIUS_LINKSEC_POLICY_PATTERN       "linksec-policy="
#define RADIUS_VENDOR_MUST_SECURE_ATTR      "must-secure"
#define RADIUS_VENDOR_SHOULD_SECURE_ATTR    "should-secure"
#define RADIUS_VENDOR_MUST_NOT_SECURE_ATTR  "must-not-secure"
                  
#define RADIUS_ACS_SEC_DACL_NAME            "xACSACLx-IP"
#define RADIUS_DACL_PATTERN                 ":inacl"             
#define RADIUS_REDIRECT_URL_PATTERN         "url-redirect="
#define RADIUS_REDIRECT_ACL_PATTERN         "url-redirect-acl="
#define RADIUS_CISCOSECURE_ATTR             "ACS:CiscoSecure-Defined-ACL"
#define RADIUS_CISCOSECURE_V6_ATTR          "ipv6:CiscoSecure-Defined-ACL" 

#define RADIUS_ATTR_VAL_IPADM               "aaa:service=ip_admission" 
#define RADIUS_ATTR_VAL_IPDOWN              "aaa:event=acl-download" 
              
#define RADIUS_VENDOR_9_VOICE 1<<0
#define RADIUS_VENDOR_9_DACL 1<<1
#define RADIUS_VENDOR_9_SWITCH 1<<2
#define RADIUS_VENDOR_9_REDIRECT_URL 1<<3
#define RADIUS_VENDOR_9_REDIRECT_ACL 1<<4
#define RADIUS_VENDOR_9_ACS_SEC_DACL 1<<5
#define RADIUS_VENDOR_9_LINKSEC_POLICY 1<<6
                      
#define RADIUS_VENDOR_311_MS_MPPE_SEND_KEY 1<<0
#define RADIUS_VENDOR_311_MS_MPPE_RECV_KEY 1<<1

#define RADIUS_VENDOR9_ACS_SEC_DACL_MASK_POS 1 <<0
#define RADIUS_VENDOR9_REDIRECT_ACL_MASK_POS 1 <<1
#define RADIUS_IF_NULLPTR_RETURN(_p) \
    if (NULL == _p) \
  { \
        return -1; \
  }

typedef enum clientStatus_e
{
  NEW_CLIENT = 1, 
  AUTH_FAIL,
  AUTH_SUCCESS,
  AUTH_TIMEOUT,
  AUTH_SERVER_COMM_FAILURE,
  CLIENT_DISCONNECTED,
  METHOD_CHANGE,
  RADIUS_SERVERS_DEAD,
  RADIUS_FIRST_PASS_DACL_DATA,
  RADIUS_DACL_INFO,
  MKA_PEER_TIMEOUT
} clientStatus_t;

typedef enum radiusAttrFlags_s
{       
  RADIUS_FLAG_ATTR_TYPE_STATE = (1 << 0),
  RADIUS_FLAG_ATTR_TYPE_SERVICE_TYPE = (1 << 1),
  RADIUS_FLAG_ATTR_TYPE_CLASS = (1 << 2),
  RADIUS_FLAG_ATTR_TYPE_SESSION_TIMEOUT = (1 << 3),
  RADIUS_FLAG_ATTR_TYPE_TERMINATION_ACTION = (1 << 4),
  RADIUS_FLAG_ATTR_TYPE_EAP_MESSAGE = (1 << 5),
  RADIUS_FLAG_ATTR_TYPE_VENDOR = (1 << 6),
  RADIUS_FLAG_ATTR_TYPE_FILTER_ID = (1 << 7),
  RADIUS_FLAG_ATTR_TYPE_TUNNEL_TYPE = (1 << 8),
  RADIUS_FLAG_ATTR_TYPE_TUNNEL_MEDIUM_TYPE = (1 << 9),
  RADIUS_FLAG_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID = (1 << 10),
  RADIUS_FLAG_ATTR_TYPE_FRAMED_IP_ADDRESS   = (1 << 11),
  RADIUS_FLAG_ATTR_TYPE_FRAMED_IPV6_ADDRESS = (1 << 12),
  RADIUS_FLAG_ATTR_TYPE_ACS_SEC_DACL_TYPE = (1 << 13),
  RADIUS_FLAG_ATTR_TYPE_EAP_KEY_NAME = (1 << 14),
  RADIUS_FLAG_ATTR_USER_NAME = (1 << 15),
  RADIUS_FLAG_ATTR_REPLY_MSG = (1 << 16)
}radiusAttrFlags_t;

typedef enum macsecLinkSecPolicy_s
{               
  MACSEC_LINKSEC_POLICY_INVALID = 0,

  /* Secure connection if both peers are MACsec capable */
  MACSEC_LINKSEC_POLICY_SHOULD_SECURE, 

  /* Host traffic will be dropped unless it successfully negotiates MACsec. */
  MACSEC_LINKSEC_POLICY_MUST_SECURE,

  /* MACsec disabled. */
  MACSEC_LINKSEC_POLICY_MUST_NOT_SECURE
} macsecLinksecPolicy_t;

typedef struct in4_addr_s
{
  unsigned int   s_addr;    /* 32 bit IPv4 address in network byte order */
}in4_addr_t;

/***************************************
 *  *
 *   * 128-bit IP6 address.
 *    ***************************************/
typedef struct in6_addr_s
{
  union
  {
    unsigned char    addr8[16];
    unsigned short   addr16[8];
    unsigned int     addr32[4];
  } in6;
}in6_addr_t;


typedef struct inet_addr_s
{ 
  unsigned char  family;  /* AF_INET, AF_INET6, ... */
  union 
  {
    struct in4_addr_s   ipv4;
    struct in6_addr_s   ipv6;
  } addr;
} inet_addr_t; 

typedef struct radiusAttr_s
{
    unsigned char  type;
    unsigned char  length;    /* length of entire attribute including the type and length fields */
}radiusAttr_t;

/* EAP packet header */
typedef struct eapPacket_s
{ 
  unsigned char   code;
  unsigned char   id;
  unsigned short  length;
} eapPacket_t;

typedef struct vendorInfo_s
{
   unsigned int   vendorId;
    unsigned int   vendorMask;
}vendorInfo_t;

typedef struct daclRules_s
{
  unsigned int count;
  unsigned int byte_count;
  unsigned char *rulesBuff;
}daclRules_t;


typedef struct attr_named_lists_s
{
  unsigned char mask;
  unsigned char name1[256];
  unsigned char name2[256];
}attr_named_lists_t;

typedef struct keyInfo_s
{
    unsigned char  key[RADIUS_MS_KEY_SIZE];
      unsigned char  keyLen;
} keyInfo_t;

typedef struct attrInfo_s
{ 
  unsigned char   userName[65];
  unsigned int   userNameLen;

  unsigned char   serverState[SERVER_STATE_LEN];
  unsigned int   serverStateLen;

  unsigned char   serverClass[SERVER_CLASS_LEN];
  unsigned int   serverClassLen;

  unsigned int   sessionTimeout;
  unsigned int   terminationAction;

  unsigned int   accessLevel;    
  unsigned char   idFromServer;   /* Most recent ID in EAP pkt received from Auth Server (0-255) */
  unsigned char   vlanString[RADIUS_VLAN_ASSIGNED_LEN+1];
  unsigned int   vlanId; /* parsed VLAN id from vlan string */
  unsigned int   attrFlags;
  unsigned int   vlanAttrFlags;
  bool     rcvdEapAttr;
}attrInfo_t;

typedef struct challenge_info_s
{
 unsigned int nas_port;
 char *challenge;
 unsigned int *challenge_len;
 attrInfo_t *attrInfo;
 unsigned char *supp_data; 
}challenge_info_t;

typedef struct access_req_info_s
{
 char *user_name;
 unsigned int user_name_len;

 char *chap_password;
 unsigned int chap_password_len;

 char *challenge;
 unsigned int challenge_len;

 unsigned int mab_auth_type;

 char *calledId;
 unsigned int calledId_len;

 char *callingId;
 unsigned int callingId_len;

 unsigned int nas_port;

 char *nas_portid;
 unsigned int nas_portid_len;

 char *supp_eap_data;
 unsigned int correlator;

 attrInfo_t *attrInfo; 
 unsigned char supp_mac[6];

 inet_addr_t   nas_ip;
 unsigned char nas_id[64];

 void *msg_req;
 void *cxt;
}access_req_info_t;

typedef struct acs_access_req_info_s
{
 char *user_name;
 unsigned int user_name_len;

 char *serverState;
 unsigned int server_state_len;

 unsigned int correlator;

 unsigned char supp_mac[6];
 void *msg_req;
 void *cxt;
}acs_access_req_info_t;

typedef struct clientAuthInfo_s
{
  unsigned char   addr[6];
  unsigned int    eapolVersion;
  char            bam_used[64];
  attrInfo_t attrInfo;
  char userName[65];
  unsigned int userNameLength;
}clientAuthInfo_t;
  
typedef struct clientStatusReply_s
{
  char intf[66];
  char method[64];
  unsigned int status;
  union 
  {
    clientAuthInfo_t authInfo;
    char enableStatus[64];
  }info;
}clientStatusReply_t;

typedef enum radius_mab_cmd_s
{
  RADIUS_MAB_CMD_NONE = 0,
  RADIUS_MAB_SERVER_ADD,
  RADIUS_MAB_SERVER_MODIFY,
  RADIUS_MAB_SERVER_DELETE,
  RADIUS_MAB_GLOBAL_CFG,
  RADIUS_MAB_SERVERS_RELOAD,
  RADIUS_MAB_SERVER_ELOOP_TERMINATE
}radius_mab_cmd_t;


typedef struct mab_key_s
{ 
  unsigned char key[65];
  unsigned int key_len;
}mab_key_t;
  
  
typedef struct mab_radius_server_s
{
  unsigned char radius_type[64]; /* auth or accounting */
  unsigned char serv_addr[128];
  unsigned char serv_port[32];
  unsigned char serv_priority[32];
  mab_key_t key;
}mab_radius_server_t;

typedef struct mab_radius_access_req_s
{
 void *req_attr;
 void *msg;
}mab_radius_access_req_t;

typedef struct mab_radius_cmd_msg_s
{
  unsigned char cmd[32];
  union {
    mab_radius_server_t server;
    mab_radius_access_req_t access_req;
    unsigned char mab_cli_mac_addr[6 /*ETH_ALEN*/];
  }cmd_data;
  void *data;
}mab_radius_cmd_msg_t;

typedef void(*mabRadiusCmdFn_t) (void *cxt, void *cmd_data);

typedef struct radius_mab_cmd_entry_s
{
  unsigned char *cmd;
  mabRadiusCmdFn_t hdlr;
}radius_mab_cmd_entry_t;

typedef int(*radiusAttrParseFn_t) (radiusAttr_t *radiusAttr, attrInfo_t *attrInfo);

int radiusAttrMapEntryGet(unsigned int attrType, radiusAttrParseFn_t *fn);

typedef struct radiusAttrParseFnMap_s
{
    unsigned int attrType;
    radiusAttrParseFn_t fn;
}radiusAttrParseFnMap_t;

int radiusVendorAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo);

int radiusClientAcceptProcess(void *msg, attrInfo_t *attrInfo);
int radiusClientChallengeProcess(void *radiusMsg, challenge_info_t *get_data);
int radiusAttrNasPortValidate(unsigned int nas_port, radiusAttr_t *radiusAttr);
int radiusChallengeCopy(radiusAttr_t *radiusAttr, challenge_info_t *get_data);
int radiusAttrChallengeCopy(radiusAttr_t *radiusAttr, challenge_info_t *get_data);
int radius_mab_server_send(void *req, void *msg);
int radius_acs_mab_server_send(void *req, void *msg);
int radius_mab_server_id_get(void *req, unsigned char *radius_identifier);
int radius_mab_current_auth_server_key_get(void *req, unsigned char *shared_secret,
                                           size_t *shared_secret_len);
int radius_server_update(bool op, void *data, mab_radius_server_t *server);
#if 0
int radius_server_update(bool op, void *data, const char *radius_type,
                         const char *serv_addr, const char  *serv_priority,
                         const char *radius_key, const char *serv_port);
#endif
int radius_client_get_stats(void *data, char *buf, size_t buflen);

void mab_eloop_register(int fd, void *data);
int mab_radius_client_alloc(void **data);
void mab_radius_server_run(void *data);
int radius_mab_resp_req_map_validate(void *cxt, void *data, int msg_len);
void radius_mab_cmd_req_send(int client_fd, char *req, size_t req_len);
void radius_mab_client_flush(void *data);
void mab_radius_server_add(void *data, void *server);
void mab_radius_server_delete(void *data, void *server);
void mab_radius_server_reload(void *data, void *server);
void mab_radius_server_eloop_terminate(void *data, void *server);
void mab_radius_client_access_req_send(void *data, void *req);
void mab_radius_client_acs_access_req_send(void *data, void *req);
void mab_radius_client_clear_radius_msgs(void *data, void *req);
int radius_get_dwld_acl_type_get(void *attrInfo);
void** radius_sec_acl_req_rules_mem_get(unsigned int type, void *attrInfo);
unsigned char* radius_sec_acl_req_name_get(unsigned int type, void *attrInfo);
void mab_radius_server_debug_level_set(int level);
int  mab_radius_server_debug_level_get();
int radius_named_list_mask_clear(unsigned int pos, void *p);
int radius_dwld_pos_mask_clear_validate(unsigned int pos, void *p);
#endif
