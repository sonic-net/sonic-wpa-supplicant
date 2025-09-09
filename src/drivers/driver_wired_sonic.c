/*
 * Wired Ethernet driver interface for Sonic
 * Copyright (c) 2005-2009, 
 * Copyright (c) 2004
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include "includes.h"
#include "common.h"
#include "driver.h"
#include "driver_wired_common.h"
#include "ap/sta_info.h"
#include "radius/radius_attr_parse.h"
#include "common/eapol_common.h"

#define STATUS_COPY(_status)  static void _status##_##copy(char *intf, clientStatusReply_t *reply, u8 *addr, void *param)
#define STATUS_ENTER(_status, _intf, _reply, _addr, _param)  _status##_##copy(_intf, _reply, _addr,  _param)


#define STATUS_MALLOC(__status, _param, _reply)  \
  do { \
    unsigned int _len = 0; \
    if (AUTH_SUCCESS == __status) \
    { \
      struct sta_info * _sta = NULL; \
      _sta = (struct sta_info *)_param;\
      \
    }\
    _reply = (clientStatusReply_t *)malloc(sizeof(*_reply) + _len); \
             memset(_reply, 0, sizeof(*_reply)+ _len); \
  } while(0); 


typedef struct status_map_s
{
 char *status;
 unsigned int val;
}status_map_t;

static int wpa_pac_send_data (char *buf, int bufLen) 
{
  struct sockaddr_in saddr;
  int fd, ret_val;
  struct hostent *local_host;	/* need netdb.h for this */
  int i;
  char *ptr = buf;
  struct linger sl;
  struct sockaddr_in client;
  socklen_t clientlen = sizeof(client);

	/* Step1: create a TCP socket */
	fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (fd == -1)
	{
		fprintf(stderr, "socket failed [%s]\n", strerror(errno));
		return -1;
	}

	/* Let us initialize the server address structure */
	saddr.sin_family = AF_INET;         
	saddr.sin_port = htons(3434);     
	local_host = gethostbyname("127.0.0.1");
	saddr.sin_addr = *((struct in_addr *)local_host->h_addr);


	/* Step2: connect to the server socket */
	ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	if (ret_val == -1)
	{
		fprintf(stderr, "connect failed [%s]\n", strerror(errno));
		close(fd);
		return -1;
	}

    sl.l_onoff = 1;  /* enable linger option */
    sl.l_linger = 30;    /* timeout interval in seconds */
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)))
    {
	  wpa_printf(MSG_INFO,"unable to set SO_LINGER option socket with fd: %d\n", fd);
    }

    getsockname(fd, (struct sockaddr *) &client, &clientlen);

	wpa_printf (MSG_INFO,"The Socket is now connected fd %d [%s:%u] \n", fd, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

	/* Next step: send some data */
	ret_val = send(fd, buf, bufLen, 0);
	wpa_printf(MSG_INFO,"fd :%d  Successfully sent data (len %d bytes): %s\n", 
				 fd, bufLen, buf);

	/* Last step: close the socket */
	close(fd);
	return 0;
}

STATUS_COPY(AUTH_SUCCESS)
{
	struct sta_info * sta = NULL;
	sta = (struct sta_info *)param;

	if (addr)
	{
		memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = AUTH_SUCCESS; 
	strncpy (reply->intf, intf, strlen(intf));

	/* copy user details */
	strncpy((char *)reply->info.authInfo.userName, (const char *)sta->attr_info.userName, strlen((char *)sta->attr_info.userName));
	reply->info.authInfo.userNameLength = strlen((const char *)sta->attr_info.userName);

	/* since this host-apd code is 2004 version, version is 2 */
	reply->info.authInfo.eapolVersion = 2;

	/* copy bam used for authentication */
	strncpy(reply->info.authInfo.bam_used, "radius", strlen("radius")+1);

	/* copy all the attributes received from radius/ or backend method */
	reply->info.authInfo.attrInfo = sta->attr_info;

}


STATUS_COPY(NEW_CLIENT)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = NEW_CLIENT; 
	//strncpy (reply->status, "new_client", strlen("new_client")); 
	strncpy (reply->intf, intf, strlen(intf)); 

}


STATUS_COPY(AUTH_FAIL)
{
	struct sta_info * sta = NULL;
	sta = (struct sta_info *)param;

	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = AUTH_FAIL; 

	/* copy user details */
	strncpy(reply->info.authInfo.userName, sta->attr_info.userName, strlen(sta->attr_info.userName));
	reply->info.authInfo.userNameLength = strlen(sta->attr_info.userName);

	//strncpy (reply->status, "auth_fail", strlen("auth_fail")); 
	strncpy (reply->intf, intf, strlen(intf)); 

}


STATUS_COPY(AUTH_TIMEOUT)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = AUTH_TIMEOUT; 

	//strncpy (reply->status, "auth_timeout", strlen("auth_timeout")); 
	strncpy (reply->intf, intf, strlen(intf)); 

}


STATUS_COPY(AUTH_SERVER_COMM_FAILURE)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = AUTH_SERVER_COMM_FAILURE; 

	//strncpy (reply->status, "auth_server_comm_failure", strlen("auth_server_comm_failure")); 
	strncpy (reply->intf, intf, strlen(intf)); 

}


STATUS_COPY(CLIENT_DISCONNECTED)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = CLIENT_DISCONNECTED; 

	//strncpy (reply->status, "client_disconnected", strlen("client_disconnected")); 
	strncpy (reply->intf, intf, strlen(intf)); 

}

STATUS_COPY(METHOD_CHANGE)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = METHOD_CHANGE; 
	//strncpy (reply->status, "method_change", strlen("method_change")); 
	strncpy (reply->intf, intf, strlen(intf));
        strncpy (reply->info.enableStatus, (char *)param, strlen((char *)param)); 

}

STATUS_COPY(RADIUS_SERVERS_DEAD)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = RADIUS_SERVERS_DEAD; 

	//strncpy (reply->status, "radius_servers_dead", strlen("radius_servers_dead")); 
	strncpy (reply->intf, intf, strlen(intf));

}

#if 0
STATUS_COPY(RADIUS_FIRST_PASS_DACL_DATA)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")); 
	strncpy (reply->status, "radius_first_pass_dacl_data", strlen("radius_first_pass_dacl_data")); 
	strncpy (reply->intf, intf, strlen(intf));

}
#endif

STATUS_COPY(RADIUS_DACL_INFO)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = RADIUS_DACL_INFO; 
	//strncpy (reply->status, "radius_dacl_info", strlen("radius_dacl_info")); 
	strncpy (reply->intf, intf, strlen(intf));
}



STATUS_COPY(MKA_PEER_TIMEOUT)
{
	memset(reply, 0, sizeof(*reply));
	if (addr)
	{
			memcpy(reply->info.authInfo.addr, addr, 6);
	}
	strncpy (reply->method, "802.1X", strlen("802.1X")+1); 
	reply->status = MKA_PEER_TIMEOUT; 
	//strncpy (reply->status, "mka_peer_timeout", strlen("mka_peer_timeout")); 
	strncpy (reply->intf, intf, strlen(intf));

}

static int client_resp_val_get(char *in)
{
  unsigned int i = 0;

  static status_map_t status_map[] = {
		{"new_client", NEW_CLIENT},
		{"auth_fail", AUTH_FAIL},
		{"auth_success", AUTH_SUCCESS},
		{"auth_timeout", AUTH_TIMEOUT},
		{"auth_server_comm_failure", AUTH_SERVER_COMM_FAILURE},
		{"client_disconnected", CLIENT_DISCONNECTED},
		{"method_change", METHOD_CHANGE},
		{"radius_server_dead", RADIUS_SERVERS_DEAD},
		{"radius_first_pass_dacl_data", RADIUS_FIRST_PASS_DACL_DATA},
		{"radius_dacl_info", RADIUS_DACL_INFO},
		{"mka_peer_timeout", MKA_PEER_TIMEOUT},
	};

	for (i = 0; i < sizeof(status_map)/sizeof(status_map_t); i++)
	{
		if (0 == strcmp(in, status_map[i].status))
		{
			return status_map[i].val;
		}
	}
	return -1;
}

int wired_driver_auth_resp_send(char *intf, u8 *addr, char *status, void *param)
{
  clientStatusReply_t *reply;
  
  unsigned int val = 0;
  int rv = 0;

  val = client_resp_val_get(status);

  STATUS_MALLOC(val, param, reply);
 // reply = (clientStatusReply_t *)malloc(sizeof(*reply));
//  memset(reply, 0, sizeof(*reply));

 switch (val)
 {
  case NEW_CLIENT:
  STATUS_ENTER(NEW_CLIENT, intf, reply, addr, param);
  break;

  case AUTH_FAIL:
  STATUS_ENTER(AUTH_FAIL, intf, reply, addr, param);
  break;

  case AUTH_SUCCESS:
  STATUS_ENTER(AUTH_SUCCESS, intf, reply, addr, param);
  break;

  case AUTH_TIMEOUT:
  STATUS_ENTER(AUTH_TIMEOUT, intf, reply, addr, param);
  break;

  case AUTH_SERVER_COMM_FAILURE:
  STATUS_ENTER(AUTH_SERVER_COMM_FAILURE, intf, reply, addr, param);
  break;

  case CLIENT_DISCONNECTED:
  STATUS_ENTER(CLIENT_DISCONNECTED, intf, reply, addr, param);
  break;

  case METHOD_CHANGE:
  STATUS_ENTER(METHOD_CHANGE, intf, reply, addr, param);
  break;

  case RADIUS_SERVERS_DEAD:
  STATUS_ENTER(RADIUS_SERVERS_DEAD, intf, reply, addr, param);
  break;

  case RADIUS_DACL_INFO:
  STATUS_ENTER(RADIUS_DACL_INFO, intf, reply, addr, param);
  break;

  case MKA_PEER_TIMEOUT:
  STATUS_ENTER(MKA_PEER_TIMEOUT, intf, reply, addr, param);
  break;

  default:
    /* unknown response is received */
    return -1;

 }

  /* send msg */

  rv = wpa_pac_send_data((char *)reply, sizeof(*reply)); 

  if (addr)
  {
    wpa_printf(MSG_INFO,"rv = %d   " MACSTR "  status %s\n", rv,  MAC2STR(addr), status);
  }
  else
  {
    wpa_printf(MSG_INFO,"rv = %d status %s\n", rv,  status);
  }

  if (reply)
     free (reply);

  return rv;
}


