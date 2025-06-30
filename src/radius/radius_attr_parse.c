#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>
#include <arpa/inet.h>
#include "radius_attr_parse.h"
#include "utils/includes.h"
#include "utils/common.h"
#include "utils/wpabuf.h"
#include "wpa_debug.h"

#define EAP_RRMD5       4  /* MD5-Challenge */
#define CHALLENGE_LEN   16

int radiusUsernameAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  memset(attrInfo->userName,'\0', sizeof(attrInfo->userName));
  memcpy(attrInfo->userName,
      (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->userNameLen = radiusAttr->length - sizeof(radiusAttr_t);
  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_USER_NAME;

  return 0;
} 

int radiusServiceTypeAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int temp = 0;
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_SERVICE_TYPE;
  memcpy(&temp,
      ((char *)radiusAttr + sizeof(radiusAttr_t)),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->accessLevel = ntohl(temp);

  return 0;
} 

int radiusNasPortAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int nas_port = 0;
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);

  memcpy(&nas_port,
      ((char *)radiusAttr + sizeof(radiusAttr_t)),
      radiusAttr->length - sizeof(radiusAttr_t));
  nas_port = ntohl(nas_port);

  return nas_port;
} 



int radiusReplyMsgAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_REPLY_MSG;
  return 0;
} 


int radiusClassAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_CLASS;
  memset(attrInfo->serverClass,0,
      SERVER_CLASS_LEN);
  memcpy(attrInfo->serverClass,
      (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->serverClassLen =
    radiusAttr->length - sizeof(radiusAttr_t);

  return 0;
} 

int radiusStateAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_STATE;
  memset(attrInfo->serverState,0,
      SERVER_STATE_LEN);
  memcpy(attrInfo->serverState,
      (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->serverStateLen =
    radiusAttr->length - sizeof(radiusAttr_t);

  return 0;
} 

int radiusSessionTimeoutAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int attrVal = 0;
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_SESSION_TIMEOUT;
  /* Set the sessionTimeout value which will be picked up by the timer state machine */
  memcpy((char *)&attrVal, (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->sessionTimeout = ntohl(attrVal);

  return 0;
} 

int radiusTerminationActionAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int attrVal = 0;

  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_TERMINATION_ACTION;
  /* Set the terminationAction value which will be picked up by the timer state machine */
  memcpy((char *)&attrVal, (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  attrInfo->terminationAction = ntohl(attrVal);

  return 0;
} 


int radiusEapMsgAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  eapPacket_t *eapPkt;

  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_EAP_MESSAGE;
  /* If this is the first EAP msg in the frame, save the ID and set flag. */
  if (attrInfo->rcvdEapAttr == false)
  {
    eapPkt = (eapPacket_t *)((char *)radiusAttr + sizeof(radiusAttr_t));
    attrInfo->idFromServer = eapPkt->id;
    attrInfo->rcvdEapAttr = true;
  }

  return 0;
} 

int radiusTunnelTypeAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int attrVal = 0;
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_TUNNEL_TYPE;
  /* get the tunnel type */
  memcpy((char *)&attrVal, (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  if(((ntohl(attrVal)) & 0x00FFFFFF) == RADIUS_TUNNEL_TYPE_VLAN)
  {
    attrInfo->vlanAttrFlags |= 0x1;
  }

  return 0;
} 

int radiusTunnelMediumAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  unsigned int attrVal = 0;
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_TUNNEL_MEDIUM_TYPE;
  /* get the tunnel medium type */
  memcpy((char *)&attrVal, (char *)radiusAttr + sizeof(radiusAttr_t),
      radiusAttr->length - sizeof(radiusAttr_t));
  if(((ntohl(attrVal)) & 0x00FFFFFF) == RADIUS_TUNNEL_MEDIUM_TYPE_802)
  {
    attrInfo->vlanAttrFlags |= 0x2;
  }

  return 0;
} 

int radiusTunnelGrpIdAttrGet(radiusAttr_t *radiusAttr, attrInfo_t *attrInfo)
{
  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(attrInfo);

  attrInfo->attrFlags |=  RADIUS_FLAG_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID;
  char  tagField;
  unsigned int  len=1;
  unsigned int  stringLen=0;

  memset(attrInfo->vlanString, 0, sizeof(attrInfo->vlanString));
  /* ignore the tag 1 byte */
  memcpy((char *)&tagField,(char *)radiusAttr + sizeof(radiusAttr_t),1);

  if(tagField > 0x1F)   /* RFC 2868 Section 3.6 */
  {
    len = 0;
  }

  stringLen=(RADIUS_VLAN_ASSIGNED_LEN < ((radiusAttr->length - sizeof(radiusAttr_t))-len))?
    RADIUS_VLAN_ASSIGNED_LEN:((radiusAttr->length - sizeof(radiusAttr_t))-len);

  memcpy((char *)attrInfo->vlanString, (char *)radiusAttr + sizeof(radiusAttr_t)+len,
      stringLen);

  attrInfo->vlanAttrFlags |= 0x4;

  return 0;
} 

int radiusAttrNasPortValidate(unsigned int nas_port, radiusAttr_t *radiusAttr)
{
  if (nas_port != radiusNasPortAttrGet(radiusAttr, NULL))
     return -1;
 return 0;
}

int radiusChallengeCopy(radiusAttr_t *radiusAttr, challenge_info_t *get_data)
{
  radiusAttr_t *eapTlv;
  unsigned char *eapBuf, *tempChallenge;
  unsigned char ch;

  RADIUS_IF_NULLPTR_RETURN(radiusAttr);
  RADIUS_IF_NULLPTR_RETURN(get_data);

  eapBuf = get_data->supp_data;

  memcpy(eapBuf, (unsigned char *)radiusAttr + sizeof(radiusAttr_t),
         radiusAttr->length - sizeof(radiusAttr_t));

  eapTlv = (radiusAttr_t *)((unsigned char *) eapBuf + sizeof(eapPacket_t));
  ch = (unsigned char )EAP_RRMD5;
  if (memcmp(&eapTlv->type,(unsigned char *)&ch,sizeof(unsigned char))==0)
  {
          ch = (unsigned char )CHALLENGE_LEN;
          if (memcmp(&eapTlv->length,(unsigned char *)&ch,sizeof(unsigned char))<=0)
          {
                  *get_data->challenge_len = (unsigned int)eapTlv->length;
                  tempChallenge = (unsigned char *) ((unsigned char *) eapTlv + sizeof(radiusAttr_t));
                  memset(get_data->challenge, 0, CHALLENGE_LEN);
                  memcpy(get_data->challenge,tempChallenge, *get_data->challenge_len);
          }
          else
          {
                  /* length exceeds the limit.
                     fail to serve challenge.*/
                  return -1;
          }
  }
  return 0;
}

int radiusAttrChallengeCopy(radiusAttr_t *radiusAttr, challenge_info_t *get_data)
{
  if (!radiusAttr || !get_data || !get_data->attrInfo)
    return -1;

  radiusEapMsgAttrGet(radiusAttr, get_data->attrInfo);

 /* copy the challenge */
  radiusChallengeCopy(radiusAttr, get_data);
 
  return 0;
}

int radiusAttrMapEntryGet(unsigned int attrType, radiusAttrParseFn_t *fn)
{
  static radiusAttrParseFnMap_t radiusAttrMapTbl[] = {
  { RADIUS_ATTR_TYPE_USER_NAME, radiusUsernameAttrGet},
  { RADIUS_ATTR_TYPE_SERVICE_TYPE, radiusServiceTypeAttrGet},
  { RADIUS_ATTR_TYPE_REPLY_MESSAGE, radiusReplyMsgAttrGet},
  { RADIUS_ATTR_TYPE_CLASS, radiusClassAttrGet},
  { RADIUS_ATTR_TYPE_SESSION_TIMEOUT, radiusSessionTimeoutAttrGet},
  { RADIUS_ATTR_TYPE_TERMINATION_ACTION, radiusTerminationActionAttrGet},
  { RADIUS_ATTR_TYPE_EAP_MESSAGE, radiusEapMsgAttrGet},
  { RADIUS_ATTR_TYPE_TUNNEL_TYPE, radiusTunnelTypeAttrGet},
  { RADIUS_ATTR_TYPE_TUNNEL_MEDIUM_TYPE, radiusTunnelMediumAttrGet},
  { RADIUS_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID, radiusTunnelGrpIdAttrGet},
  { RADIUS_ATTR_TYPE_NAS_PORT, radiusNasPortAttrGet},
  { RADIUS_ATTR_TYPE_STATE, radiusStateAttrGet},
  };

  unsigned int i = 0;

  for (i = 0; i < (sizeof(radiusAttrMapTbl)/sizeof(radiusAttrParseFnMap_t)); i++)
  {
    if (attrType == radiusAttrMapTbl[i].attrType)
    {
      *fn = radiusAttrMapTbl[i].fn;
      return 0;
    }
  }

  return -1;
}


