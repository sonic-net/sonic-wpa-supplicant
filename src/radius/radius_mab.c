#include <netdb.h>
#include "mab_include.h"
#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "radius.h"
#include "radius_client.h"
#include "mab_radius.h"

struct radius_ctx {
 struct radius_client_data *radius;
 struct hostapd_radius_servers conf;
 u8 radius_identifier;
 struct in_addr own_ip_addr;
};

extern int wpa_debug_level;

#define MAB_MAX_RADIUS_SERVERS  FD_MAX_RADIUS_SERVERS

int radius_server_info_copy(struct hostapd_radius_server *srv,
                            struct hostapd_ip_addr addr, int priority, 
                            const  mab_key_t *key, unsigned int port)
{
   if ((!srv) || (!key))
     return -1;
 
   srv->addr = addr;
   srv->port = port;
   srv->index = priority;
   if (0 != srv->shared_secret_len)
   { 
     free(srv->shared_secret);
     srv->shared_secret_len = 0;
   } 
   srv->shared_secret = (char *)malloc(key->key_len + 1);
   if (!srv->shared_secret)
     return -1;

   memset(srv->shared_secret, 0, key->key_len + 1);
   strncpy(srv->shared_secret, key->key, key->key_len);
   srv->shared_secret_len = key->key_len;

   return 0;
}

int radius_server_info_input_cmp(struct hostapd_radius_server *s,
                                 int priority, unsigned int port, 
                                 mab_key_t *key)
{
   if ((!s) || (!key))
     return -1;
 
   if ((s->port != port) || (s->index != priority))
     return -1;
   /* server key check */
   if(s->shared_secret_len != key->key_len)
     return -1;
   if (0 != strncmp(s->shared_secret, key->key, key->key_len))
     return -1;
   return 0;
}

int radius_mab_server_id_get(void *req, unsigned char *radius_identifier)
{
   if ((!req) || (!radius_identifier))
     return -1;
   if (!((struct radius_ctx *)req)->radius)
     return -1;

	*radius_identifier = radius_client_get_id(((struct radius_ctx *)req)->radius);

   return 0;
}

int radius_mab_current_auth_server_key_get(void *req, unsigned char *shared_secret,
                                           size_t *shared_secret_len)
{
   if ((!req) || (!shared_secret) || (!shared_secret_len))
     return -1;

   struct hostapd_radius_server *auth_server = ((((struct radius_ctx *)req)->conf).auth_server);

   if ((!auth_server) || (!(auth_server->shared_secret)) ||
       (!(auth_server->shared_secret_len)))
     return -1;

   strncpy(shared_secret, auth_server->shared_secret, auth_server->shared_secret_len);
   *shared_secret_len = auth_server->shared_secret_len;

   return 0;
}

int radius_mab_server_send(void *req_attr, void *in)
{
  access_req_info_t *req = (access_req_info_t *)req_attr;
	struct radius_msg *msg = (struct radius_msg *)in;

	if (radius_client_send(((struct radius_ctx *)req->cxt)->radius, msg, RADIUS_AUTH, req->supp_mac) < 0)
               return -1;

    return 0;
}

int radius_acs_mab_server_send(void *req_attr, void *in)
{
  acs_access_req_info_t *req = (acs_access_req_info_t *)req_attr;
	struct radius_msg *msg = (struct radius_msg *)in;

	if (radius_client_send(((struct radius_ctx *)req->cxt)->radius, msg, RADIUS_AUTH, req->supp_mac) < 0)
               return -1;

    return 0;
}
int radius_mab_resp_req_map_validate(void *cxt, void *data, int msg_len)
{
  struct radius_ctx *ptr = cxt;
  radius_resp_req_map_validate((void *)ptr->radius, data, msg_len);
  return 0;
}

/* Process the RADIUS frames from Authentication Server */
RadiusRxResult radius_mab_receive_auth(struct radius_msg *msg,
                                   struct radius_msg *req,
                                   const unsigned char *shared_secret,
                                   size_t shared_secret_len,
                                   void *data)
{
/*  struct radius_msg *resp; */

  printf("Received RADIUS Authentication message; code=%d\n",
  		radius_msg_get_hdr(msg)->code);

   unsigned int correlator = 0;
   if (radius_get_req_correlator(req, &correlator))
   {
        goto send_res;
   }

   /* post to the mab queue */
	wpa_printf(MSG_DEBUG,
      "Updating PAC with radius response (correlator = %d)",
       correlator);
    mabRadiusResponseCallback((void *)msg, correlator);

send_res:
        return RADIUS_RX_QUEUED;
}


void mab_read_process(void *eloop_ctx, unsigned char *buf, int len)
{
  mab_radius_cmd_msg_t *req = (mab_radius_cmd_msg_t *)buf;
  unsigned int i = 0;
  unsigned int n = (len/sizeof(*req));
 
  radius_mab_cmd_entry_t radius_mab_cmd_tbl[] = {
  {"server-add",  mab_radius_server_add},
  {"server-delete", mab_radius_server_delete},
  {"server-reload", mab_radius_server_reload},
  {"access-req", mab_radius_client_access_req_send},
  {"acs-access-req", mab_radius_client_acs_access_req_send},
  {"server-terminate",  mab_radius_server_eloop_terminate},
  {"clear-radius-msgs",  mab_radius_client_clear_radius_msgs}
  };

    wpa_printf(MSG_DEBUG, "%s  Received %d bytes for processing", __func__, len);
  if ((!req) || (!req->cmd))
    return;

  wpa_printf(MSG_DEBUG, "%s  Received %d commands for processing", __func__, n);
  while (n)
  {
    wpa_printf(MSG_DEBUG, "%s  Received %s command", __func__, req->cmd);
    for (i = 0; i < (sizeof(radius_mab_cmd_tbl)/sizeof(radius_mab_cmd_entry_t)); i++)
    {
      if (0 == strcmp(radius_mab_cmd_tbl[i].cmd, req->cmd))
      {
        wpa_printf(MSG_DEBUG, "%s  invoking the associated handler for  %s ", __func__, req->cmd);
        radius_mab_cmd_tbl[i].hdlr(req->data, &req->cmd_data);
        break;
      }
    }
    n--;
    req++;
  }
}

static void handle_mab_read(int sock, void *eloop_ctx, void *sock_ctx)
{
  int len, n;
  unsigned char buf[1024];
  struct sockaddr_in c_addr;

   
    wpa_printf(MSG_DEBUG, "%s [fd - %d], Received some data on fd..Starting to read", __func__, sock);
    len = sizeof(c_addr);
   
    n = recvfrom(sock, (unsigned char *)buf, 1024, 
                MSG_WAITALL, ( struct sockaddr *) &c_addr,
                &len);
  if (n < 0) {
    wpa_printf(MSG_ERROR, "recvfrom: %s", strerror(errno));
    return;
  }
    wpa_printf(MSG_DEBUG, "%s [fd - %d], Received %d bytes", __func__, sock, n);

  mab_read_process(eloop_ctx, buf, n);
}


void mab_eloop_register(int fd, void *data)
{
  wpa_printf(MSG_DEBUG, "mab_eloop_register: eloop init - start");

  eloop_init();

  if (eloop_register_read_sock(fd, handle_mab_read, data, NULL)) {
    wpa_printf(MSG_ERROR, "Could not register mab eloop read socket");
    return;
  }

  wpa_printf(MSG_DEBUG, "mab_eloop_register: eloop run - start");
  eloop_run();
  wpa_printf(MSG_DEBUG, "mab_eloop_register: eloop run - end");
}

void radius_mab_cmd_req_send(int client_fd, char *req, size_t req_len)
{
  struct sockaddr_in saddr;
  struct hostent *local_host;	/* need netdb.h for this */
  int n = 0;

  wpa_printf(MSG_DEBUG, "%s , fd - %d: initiate send ",__func__,  client_fd);

  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(9395);
  local_host = gethostbyname("127.0.0.1");
  saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

  n = sendto(client_fd, (const char *)req, req_len,
        MSG_CONFIRM, (const struct sockaddr *) &saddr, 
            sizeof(saddr));

  wpa_printf(MSG_DEBUG, "%s , fd - %d: successfully sent %d bytes ",__func__,  client_fd, n);
}

int mab_radius_client_alloc(void **data)
{

	struct radius_ctx *cxt = (struct radius_ctx *)malloc(sizeof(*cxt));
         
        if (!cxt)
             return -1;

        memset(cxt, 0, sizeof(*cxt));
        
	cxt->conf.auth_servers = (struct hostapd_radius_server *)malloc(MAB_MAX_RADIUS_SERVERS *sizeof(struct hostapd_radius_server));
	if (cxt->conf.auth_servers == NULL)
		goto rollback;

	cxt->conf.acct_servers = (struct hostapd_radius_server *)malloc(MAB_MAX_RADIUS_SERVERS *sizeof(struct hostapd_radius_server));
	if (cxt->conf.acct_servers == NULL)
		goto rollback;

	memset(cxt->conf.auth_servers, 0, MAB_MAX_RADIUS_SERVERS*sizeof(struct hostapd_radius_server));
    cxt->conf.auth_server = NULL;
    
	memset(cxt->conf.acct_servers, 0, MAB_MAX_RADIUS_SERVERS*sizeof(struct hostapd_radius_server));
    cxt->conf.acct_server = NULL;
    
        *data = (void *)cxt;
	return 0;

rollback:

 if (cxt)
 {
    if (cxt->radius)
       free(cxt->radius);

    if (cxt->conf.auth_servers)
        free(cxt->conf.auth_servers);

    if (cxt->conf.acct_servers)
        free(cxt->conf.acct_servers);
  
    if (cxt)
       free(cxt);
  }
	return -1;
}

static void radius_periodic_dummy(void *eloop_ctx, void *timeout_ctx)
{
  eloop_register_timeout(1, 0, radius_periodic_dummy, NULL, NULL);
  return;
}

int radius_mab_client_register(void *data)
{

	struct radius_ctx *cxt = (struct radius_ctx *)data;
         
        if (!cxt)
             return -1;

    
  wpa_printf(MSG_DEBUG, "%s  initializing the radius client ", __func__);
	cxt->radius = radius_client_init(cxt, &cxt->conf);

	if (cxt->radius == NULL) {
       wpa_printf(MSG_ERROR, "%s  Failed to initialize RADIUS client", __func__);
		goto rollback;
	}

        /* Register a dummy timeout handler to let eloop wake every sec
         * and evaluate terminate condition
         */
        eloop_register_timeout(1, 0, radius_periodic_dummy, NULL, NULL);

    wpa_printf(MSG_DEBUG, "%s  registering the radius client ", __func__);
	if (radius_client_register(cxt->radius, RADIUS_AUTH, radius_mab_receive_auth,
       		                   cxt) < 0) {
		wpa_printf(MSG_ERROR,"Failed to register RADIUS authentication handler");
		goto rollback;
	}

	wpa_printf(MSG_DEBUG,"RADIUS registration success");
	return 0;

rollback:

 if (cxt)
 {
    if (cxt->radius)
       free(cxt->radius);
  }
	return -1;
}


int radius_mab_client_deregister(void *data)
{
	struct radius_ctx *cxt = (struct radius_ctx *)data;
         
        if (!cxt)
             return -1;

   if (cxt->radius)
   {
     radius_client_deinit(cxt->radius);
     cxt->radius = NULL;
   }

	printf("RADIUS de registration success\n");
	return 0;

}

int radius_server_addr_idx_find(struct hostapd_radius_server *servers, struct hostapd_ip_addr addr)
{
  unsigned int i = 0;

  if (!servers)
     return -1;
 
  for (i = 0; i < MAB_MAX_RADIUS_SERVERS; i++)
  { 
     if (0 == memcmp(&servers->addr, &addr, sizeof(addr)))
       return i;
     servers++;
  }
  return -1;
}

int radius_server_free_idx_find(struct hostapd_radius_server *servers, int index)
{
  unsigned int i = 0;
  struct hostapd_ip_addr addr;

  memset(&addr, 0, sizeof(addr));

  if (!servers)
     return -1;
 
  for (i = 0; i < MAB_MAX_RADIUS_SERVERS; i++)
  { 
    if ((0 == memcmp(&servers->addr, &addr, sizeof(addr))) ||
        (servers->index > index))
       return i;

    servers++;
  }
  return -1;
}

int radius_server_sort(struct hostapd_radius_server *servers, unsigned int count)
{
  struct hostapd_radius_server s = {0};
  struct hostapd_ip_addr addr;
  unsigned int i, j;

  if ((!servers) || (count > MAB_MAX_RADIUS_SERVERS))
     return -1;

  memset(&addr, 0, sizeof(addr));

  for (i = 0; i < (count -1); i++)
  {
     for (j = 0; j < (count-(i+1)); j++)
     {
        if ((servers[j].index < servers[j+1].index) ||
            (0 == memcmp(&servers->addr, &addr, sizeof(addr))))
        {
           s = servers[j];
           servers[j] = servers[j+1];
           servers[j+1] = s;
        }
     }
  }
  return 0;
}

int radius_server_insert(struct hostapd_radius_server *servers, unsigned int idx, 
                         struct hostapd_ip_addr addr, int priority,
                         const  mab_key_t *key, unsigned int port)
{
  struct hostapd_radius_server *s, *d;
  unsigned int i = 0;

  if ((!servers) || (idx >= MAB_MAX_RADIUS_SERVERS)) 
     return -1;
 
  d = servers + (MAB_MAX_RADIUS_SERVERS - 1);
  s = servers + (MAB_MAX_RADIUS_SERVERS - 2);

  for (i = MAB_MAX_RADIUS_SERVERS - 1; i > idx; i--)
  {
     memcpy(d, s, sizeof(*s));
     s--;
     d--;
  }

  s = servers + idx;
  memset(s, 0, sizeof(*s));
  return radius_server_info_copy(s, addr, priority, key, port);
}

int radius_server_remove(struct hostapd_radius_server *servers, unsigned int idx) 
{
  unsigned int i = 0;
  struct hostapd_radius_server *s, *d;

  if ((!servers) || (idx >= MAB_MAX_RADIUS_SERVERS)) 
     return -1;
 
  s = servers + idx;

  /* free the memory allocated for shared secret */
  if (s->shared_secret)
     free(s->shared_secret);
  memset(s, 0, sizeof(*s));

  d = s;
  s++;

  for (i = idx; i < MAB_MAX_RADIUS_SERVERS -1; i++)
  {
    *d = *s;
     memset(s, 0, sizeof(*s));
     d++;
     s++;
  }

  return 0;
}

int dbg = 0;

void dbg_flag(int flag)
{
  dbg = flag;
}


int radius_server_update(bool op, void *data, mab_radius_server_t *server)
{
  struct radius_ctx *cxt = (struct radius_ctx *)data;

  if (!cxt)
    return -1;

  struct hostapd_radius_server *srv = NULL, *s = NULL;
  struct hostapd_ip_addr addr;
  unsigned int port, priority = 0, idx = 0;
  int *num_servers = NULL;

  memset(&addr, 0, sizeof(addr));

  /* get the ip address */
  if (hostapd_parse_ip_addr(server->serv_addr, &addr))
    return -1;

  if (server->serv_port)
     port = atoi(server->serv_port);

  if (server->serv_priority)
     priority = atoi(server->serv_priority);

  if (0 == strncmp(server->radius_type, "auth", strlen("auth")))
  {
    srv = &(cxt->conf.auth_servers[0]);
    num_servers = &(cxt->conf.num_auth_servers);
    if (!server->serv_port)
    {
      port = 1812;
    }
  }

  if (0 == strncmp(server->radius_type, "acct", strlen("acct")))
  {
    srv = &(cxt->conf.acct_servers[0]);
    num_servers = &(cxt->conf.num_acct_servers);
    if (!server->serv_port)
    {
      port = 1813;
    }
  }

  if (op)
  {
    if (MAB_MAX_RADIUS_SERVERS == (*num_servers))
      return -1;

    /* Check Radius server alreay exists and update any change in server data */
    idx = radius_server_addr_idx_find(srv, addr);
    if ((-1 != idx) && (MAB_MAX_RADIUS_SERVERS > idx))
    {
      s = (srv + idx);
      if (0 == radius_server_info_input_cmp (s, priority, port, &server->key))
        return 0;
      /* If no change in priority update the radius params in the existing server entry
       * If change in priority, remove the entry and create new radius server entry
       */
      if (s->index == priority)
      {
        radius_server_info_copy(s, addr, priority, &server->key, port);
      }
      else
      {
        radius_server_info_copy(s, addr, priority, &server->key, port);
        radius_server_sort(srv, MAB_MAX_RADIUS_SERVERS);
      }
    }
    else
    {
      idx = radius_server_free_idx_find(srv, priority);
      if (-1 == idx)
        return -1;

      radius_server_insert(srv, idx, addr, priority, &server->key, port);
      /* Sort the Radius servers, when add new Radius server */
      radius_server_sort(srv, MAB_MAX_RADIUS_SERVERS);
      (*num_servers)++;
      printf ("%s num_servers %d \n", __func__, *num_servers);
    }
  }
  else
  {
    if (0 == (*num_servers))
      return -1;

    idx = radius_server_addr_idx_find(srv, addr);
    if (-1 == idx)
      return -1;

    radius_server_remove(srv, idx);
    (*num_servers)--;
  }

  /* after every update always, place the highest priority server as current server */
  if (0 == strncmp(server->radius_type, "auth", strlen("auth")))
  {
    if (*num_servers)
      cxt->conf.auth_server = cxt->conf.auth_servers;
    else
      cxt->conf.auth_server = NULL;

  }

  if (0 == strncmp(server->radius_type, "acct", strlen("acct")))
  {
    if (*num_servers)
      cxt->conf.acct_server = cxt->conf.acct_servers;
    else
      cxt->conf.acct_server = NULL;
  }


   if (op)
   {
     cxt->conf.msg_dumps++;
   }
   else
   {
     cxt->conf.msg_dumps--;
   }

  return 0;
}

int radius_client_get_stats(void *data, char *buf, size_t buflen)
{
  struct radius_ctx *cxt; 

  if ((data == NULL) || (buf == NULL))
  {
      return -1;
  }

  cxt = (struct radius_ctx *)data;

  return radius_client_get_mib(cxt->radius, buf, buflen);
}


void radius_mab_client_flush(void *data)
{
  struct radius_ctx *cxt; 

  if (data == NULL) 
  {
      return;
  }

  cxt = (struct radius_ctx *)data;

  radius_client_flush(cxt->radius, 0);
}



void mab_radius_server_add(void *data, void *req)
{
  mab_radius_server_t *server = req;
  struct radius_ctx *cxt = (struct radius_ctx *)data;

  wpa_printf(MSG_DEBUG, "%s  inserting server entry %s", __func__, server->serv_addr);
  /* insert the entry  */
  radius_server_update(TRUE, data, server);
  
  if (cxt->radius)
  {
    radius_mab_client_flush(cxt);
    radius_close_auth_sockets(cxt->radius);
    radius_close_acct_sockets(cxt->radius);
  }
  else
  {
    /* register the entry with radius client */
    if (0 != radius_mab_client_register(data))
    {
            wpa_printf(MSG_ERROR,"radius mab client_register failed..");
    }
  }
}

void mab_radius_server_delete(void *data, void *req)
{ 
  mab_radius_server_t *server = req;
  struct radius_ctx *cxt = (struct radius_ctx *)data;

  /* delete the existing entry and re-insert the new entry */
  radius_server_update(FALSE, data, server);

  if (cxt->radius)
  {
    radius_mab_client_flush(cxt);
    radius_close_auth_sockets(cxt->radius);
    radius_close_acct_sockets(cxt->radius);
  }

  if (0 == cxt->conf.num_auth_servers)
  {
    /* de-register the entry with radius client */
    wpa_printf(MSG_DEBUG, "%s  removing server entry %s", __func__, server->serv_addr);
    if (0 != radius_mab_client_deregister(data))
    {
            wpa_printf(MSG_ERROR,"radius mab client_deregister failed..");
    }
  }
}

void mab_radius_server_reload(void *data, void *req)
{ 
  struct radius_ctx *cxt = (struct radius_ctx *)data;

  if (cxt->radius)
  {
    radius_mab_client_flush(cxt);
    radius_close_auth_sockets(cxt->radius);
    radius_close_acct_sockets(cxt->radius);
  }
}

void mab_radius_client_access_req_send(void *data, void *req)
{
  mab_radius_access_req_t *access_req = req;
  authmgrClientStatusInfo_t   clientStatus;
  access_req_info_t *req_info = (access_req_info_t *)(access_req->req_attr);
  uint32 physPort;

  if (0 != ((struct radius_ctx *)(req_info->cxt))->conf.num_auth_servers)
  {
    /* invoke client access req send */
    radius_mab_server_send(access_req->req_attr, access_req->msg);
  }
  else
  {
    /* notify the inability to authenticate client as failure */
    memset(&clientStatus, 0, sizeof(authmgrClientStatusInfo_t));
    memcpy(&clientStatus.info.authInfo.macAddr, &(req_info->supp_mac), 6);
    MAB_PORT_GET(physPort, req_info->correlator);
    mabPortClientAuthStatusUpdate(physPort, clientStatus.info.authInfo.macAddr.addr, 
        "auth_fail", (void*) &clientStatus);
  }

  if (access_req->req_attr)
     free (access_req->req_attr);
}

void mab_radius_client_acs_access_req_send(void *data, void *req)
{
  mab_radius_access_req_t *access_req = req;
  /* invoke client access req send */
  radius_acs_mab_server_send(access_req->req_attr, access_req->msg);

  if (access_req->req_attr)
     free (access_req->req_attr);
}

void mab_radius_server_eloop_terminate(void *data, void *req)
{
  /* invoke eloop terminate */
    eloop_terminate();
}

void mab_radius_client_clear_radius_msgs(void *data, void *req)
{
  unsigned char *cli_mac_addr = req;
  struct radius_ctx *cxt = (struct radius_ctx *)data;

  if (cxt->radius)
  {
    wpa_printf(MSG_DEBUG, "%s: Clear the RADIUS messages for client " MACSTR,
               __func__, MAC2STR(cli_mac_addr));
    radius_client_flush_auth(cxt->radius, cli_mac_addr);
  }
}

void mab_radius_server_debug_level_set(int level)
{
  switch (level)
  {
     case MSG_MSGDUMP :
     case MSG_DEBUG :
     case MSG_INFO :
     case MSG_WARNING :
     case MSG_ERROR :
       wpa_debug_level = level;
     default:
       break;
  }
}

int mab_radius_server_debug_level_get()
{
  return wpa_debug_level;
}
