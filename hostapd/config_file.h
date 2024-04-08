/*
 * hostapd / Configuration file parser
 * Copyright (c) 2003-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#ifdef CONFIG_SONIC_HOSTAPD

#define HOSTAPD_JSON_MAX_INTF       512
#define HOSTAPD_SONIC_JSON_PATH     "/etc/hostapd/hostapd_config.json"

typedef struct hostapd_intf_json_s
{
  char if_name[64];
  char file_path[64];
}hostapd_intf_json_t;

typedef struct hostapd_json_data_s
{
  hostapd_intf_json_t  deleted_intf[HOSTAPD_JSON_MAX_INTF];
  unsigned int deleted_count;

  hostapd_intf_json_t modified_intf[HOSTAPD_JSON_MAX_INTF];
  unsigned int modified_count;

  hostapd_intf_json_t new_intf[HOSTAPD_JSON_MAX_INTF];
  unsigned int new_count;
}hostapd_json_data_t;

int hostapd_sonic_json_file_read(hostapd_json_data_t *parsed_data);

#endif

struct hostapd_config * hostapd_config_read(const char *fname);
int hostapd_set_iface(struct hostapd_config *conf,
		      struct hostapd_bss_config *bss, const char *field,
		      char *value);
int hostapd_acl_comp(const void *a, const void *b);
int hostapd_add_acl_maclist(struct mac_acl_entry **acl, int *num,
			    int vlan_id, const u8 *addr);
void hostapd_remove_acl_mac(struct mac_acl_entry **acl, int *num,
			    const u8 *addr);

#endif /* CONFIG_FILE_H */
