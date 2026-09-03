/*
 * IEEE 802.1X-2010 Key Agree Protocol of PAE state machine
 * Copyright (c) 2013, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef IEEE802_1X_KAY_H
#define IEEE802_1X_KAY_H

#include "utils/list.h"
#include "common/defs.h"
#include "common/ieee802_1x_defs.h"

struct macsec_init_params;

#define MI_LEN			12  /* 96-bit Member Identifier */
#define MAX_KEY_LEN		32  /* 32 bytes, 256 bits */
#define MAX_CKN_LEN		32  /* 32 bytes, 256 bits */
#define SALT_LEN		12  /* 96-bit salt */

/* MKA timer, unit: millisecond */
#define MKA_HELLO_TIME		2000
#define MKA_BOUNDED_HELLO_TIME	 500
#define MKA_LIFE_TIME		6000
#define MKA_SAK_RETIRE_TIME	3000

/**
 * struct ieee802_1x_mka_ki - Key Identifier (KI)
 * @mi: Key Server's Member Identifier
 * @kn: Key Number, assigned by the Key Server
 * IEEE 802.1X-2010 9.8 SAK generation, distribution, and selection
 */
struct ieee802_1x_mka_ki {
	u8 mi[MI_LEN];
	u32 kn;
};

struct ieee802_1x_mka_sci {
	u8 addr[ETH_ALEN];
	be16 port;
} STRUCT_PACKED;

struct mka_key {
	u8 key[MAX_KEY_LEN];
	size_t len;
};

struct mka_key_name {
	u8 name[MAX_CKN_LEN];
	size_t len;
};

enum mka_created_mode {
	PSK,
	EAP_EXCHANGE,
};

struct data_key {
	u8 *key;
	int key_len;
	struct ieee802_1x_mka_ki key_identifier;
	enum confidentiality_offset confidentiality_offset;
	u8 an;
	bool transmits;
	bool receives;
	struct os_time created_time;
	u64 next_pn;
	u8 salt[SALT_LEN];

	/* not defined data */
	bool rx_latest;
	bool tx_latest;

	int user;

	struct dl_list list;
};

/* TransmitSC in IEEE Std 802.1AE-2006, Figure 10-6 */
struct transmit_sc {
	struct ieee802_1x_mka_sci sci; /* const SCI sci */
	bool transmitting; /* bool transmitting (read only) */

	struct os_time created_time; /* Time createdTime */

	u32 ssci; /* SSCI - XPN cipher suites only */
	u8 encoding_sa; /* AN encodingSA (read only) */
	u8 enciphering_sa; /* AN encipheringSA (read only) */

	/* not defined data */
	struct dl_list list;
	struct dl_list sa_list;
};

/* TransmitSA in IEEE Std 802.1AE-2006, Figure 10-6 */
struct transmit_sa {
	bool in_use; /* bool inUse (read only) */
	u64 next_pn; /* PN nextPN (read only) */
	struct os_time created_time; /* Time createdTime */

	bool enable_transmit; /* bool EnableTransmit */

	u8 an;
	bool confidentiality;
	struct data_key *pkey;

	struct transmit_sc *sc;
	struct dl_list list; /* list entry in struct transmit_sc::sa_list */
};

/* ReceiveSC in IEEE Std 802.1AE-2006, Figure 10-6 */
struct receive_sc {
	struct ieee802_1x_mka_sci sci; /* const SCI sci */
	bool receiving; /* bool receiving (read only) */

	struct os_time created_time; /* Time createdTime */

	u32 ssci; /* SSCI - XPN cipher suites only */

	/* Number of CAs that currently reference this hardware receive SC via a
	 * live peer with this SCI. A fallback CA shares the same SC as the
	 * primary over the same link, so it is created once and freed only when
	 * the last CA drops its peer. */
	int refcnt;

	struct dl_list list;
	struct dl_list sa_list;
};

/* ReceiveSA in IEEE Std 802.1AE-2006, Figure 10-6 */
struct receive_sa {
	bool in_use; /* bool inUse (read only) */
	u64 next_pn; /* PN nextPN (read only) */
	u64 lowest_pn; /* PN lowestPN (read only) */
	struct os_time created_time;

	bool enable_receive; /* bool enableReceive */

	u8 an;
	struct data_key *pkey;

	struct receive_sc *sc;
	struct dl_list list; /* list entry in struct receive_sc::sa_list */
};

struct ieee802_1x_kay_ctx {
	/* pointer to arbitrary upper level context */
	void *ctx;

	/* abstract wpa driver interface */
	int (*macsec_init)(void *ctx, struct macsec_init_params *params);
	int (*macsec_deinit)(void *ctx);
	int (*macsec_get_max_sa_per_sc)(void *priv, enum max_sa_per_sc *max);
	int (*macsec_get_capability)(void *priv, enum macsec_cap *cap);
	int (*enable_protect_frames)(void *ctx, bool enabled);
	int (*enable_encrypt)(void *ctx, bool enabled);
	int (*set_replay_protect)(void *ctx, bool enabled, u32 window);
	int (*set_current_cipher_suite)(void *ctx, u64 cs);
	int (*enable_controlled_port)(void *ctx, bool enabled);
	int (*get_receive_lowest_pn)(void *ctx, struct receive_sa *sa);
	int (*get_transmit_next_pn)(void *ctx, struct transmit_sa *sa);
	int (*set_transmit_next_pn)(void *ctx, struct transmit_sa *sa);
	int (*set_receive_lowest_pn)(void *ctx, struct receive_sa *sa);
	int (*create_receive_sc)(void *ctx, struct receive_sc *sc,
				 enum validate_frames vf,
				 enum confidentiality_offset co);
	int (*delete_receive_sc)(void *ctx, struct receive_sc *sc);
	int (*create_receive_sa)(void *ctx, struct receive_sa *sa);
	int (*delete_receive_sa)(void *ctx, struct receive_sa *sa);
	int (*enable_receive_sa)(void *ctx, struct receive_sa *sa);
	int (*disable_receive_sa)(void *ctx, struct receive_sa *sa);
	int (*create_transmit_sc)(void *ctx, struct transmit_sc *sc,
				  enum confidentiality_offset co);
	int (*delete_transmit_sc)(void *ctx, struct transmit_sc *sc);
	int (*create_transmit_sa)(void *ctx, struct transmit_sa *sa);
	int (*delete_transmit_sa)(void *ctx, struct transmit_sa *sa);
	int (*enable_transmit_sa)(void *ctx, struct transmit_sa *sa);
	int (*disable_transmit_sa)(void *ctx, struct transmit_sa *sa);
};

struct ieee802_1x_kay {
	bool enable;
	bool active;

	bool authenticated;
	bool secured;
	bool failed;

	struct ieee802_1x_mka_sci actor_sci;
	u8 actor_priority;
	struct ieee802_1x_mka_sci key_server_sci;
	u8 key_server_priority;

	enum macsec_cap macsec_capable;
	bool macsec_desired;
	bool macsec_protect;
	bool macsec_encrypt;
	bool macsec_include_sci;
	bool macsec_replay_protect;
	u32 macsec_replay_window;
	u8 max_sa_per_sc;
	enum validate_frames macsec_validate;
	enum confidentiality_offset macsec_confidentiality;
	u32 mka_hello_time;

	/* State of the single installed SAK in the SecY (one SecY per port),
	 * reported by MKA in every SAK Use body. It belongs to the KaY, not to
	 * a per-CA participant; the principal merely reports it. */
	struct ieee802_1x_mka_ki lki;
	u8 lan;
	bool ltx;
	bool lrx;

	struct ieee802_1x_mka_ki oki;
	u8 oan;
	bool otx;
	bool orx;

	/* not defined in IEEE802.1X */
	struct ieee802_1x_kay_ctx *ctx;
	bool is_obliged_key_server;
	char if_name[IFNAMSIZ];

	unsigned int macsec_csindex;  /* MACsec cipher suite table index */
	u64 macsec_cs_id;
	int mka_algindex;  /* MKA alg table index */

	u32 dist_kn;
	u32 rcvd_keys;
	u8 dist_an;
	time_t dist_time;

	int mka_rekey_period;

	u8 mka_version;
	u8 algo_agility[4];

	u64 pn_exhaustion;
	bool port_enable;
	bool rx_enable;
	bool tx_enable;

	struct dl_list participant_list;
	enum macsec_policy policy;

	/* The MKA that currently owns the CP state machine and the SecY (data
	 * path) programming. An explicit pointer so the CP owner can be
	 * switched atomically when a fallback CKN takes over. */
	struct ieee802_1x_mka_participant *principal_participant;

	/* Set when the principal changes, cleared when a deferred rekey is
	 * armed, so a rekey that outlives its principal restarts the window. */
	bool principal_changed;

	/* The SCs model the single SecY (one per port), so they live on the KaY
	 * and are shared by every CA. The transmit SC uses the actor SCI; each
	 * receive SC is keyed by a peer SCI and reference-counted across the
	 * CAs that see that peer (see struct receive_sc::refcnt). */
	struct transmit_sc *txsc;
	struct dl_list rxsc_list;

	struct ieee802_1x_cp_sm *cp;

	struct l2_packet_data *l2_mka;

	enum validate_frames vf;
	enum confidentiality_offset co;
};


u64 mka_sci_u64(struct ieee802_1x_mka_sci *sci);

struct ieee802_1x_kay *
ieee802_1x_kay_init(struct ieee802_1x_kay_ctx *ctx, enum macsec_policy policy,
		    int macsec_ciphersuite, enum confidentiality_offset macsec_offset,
		    bool macsec_include_sci,
		    bool macsec_replay_protect, u32 macsec_replay_window,
		    int mka_rekey_period,
		    u16 port, u8 priority, const char *ifname, const u8 *addr);
void ieee802_1x_kay_deinit(struct ieee802_1x_kay *kay);

struct ieee802_1x_mka_participant *
ieee802_1x_kay_create_mka(struct ieee802_1x_kay *kay,
			  const struct mka_key_name *ckn,
			  const struct mka_key *cak,
			  u32 life, enum mka_created_mode mode,
			  bool is_authenticator);
int ieee802_1x_kay_set_participant_primary(
	struct ieee802_1x_mka_participant *participant, bool primary);
struct ieee802_1x_mka_participant *
ieee802_1x_kay_get_primary_participant(struct ieee802_1x_kay *kay);
void ieee802_1x_kay_delete_mka(struct ieee802_1x_kay *kay,
			       struct mka_key_name *ckn);
void ieee802_1x_kay_mka_participate(struct ieee802_1x_kay *kay,
				    struct mka_key_name *ckn,
				    bool status);
int ieee802_1x_kay_new_sak(struct ieee802_1x_kay *kay);
int ieee802_1x_kay_change_cipher_suite(struct ieee802_1x_kay *kay,
				       unsigned int cs_index);

int ieee802_1x_kay_set_latest_sa_attr(struct ieee802_1x_kay *kay,
				      struct ieee802_1x_mka_ki *lki, u8 lan,
				      bool ltx, bool lrx);
int ieee802_1x_kay_set_old_sa_attr(struct ieee802_1x_kay *kay,
				   struct ieee802_1x_mka_ki *oki,
				   u8 oan, bool otx, bool orx);
int ieee802_1x_kay_create_sas(struct ieee802_1x_kay *kay,
			      struct ieee802_1x_mka_ki *lki);
int ieee802_1x_kay_delete_sas(struct ieee802_1x_kay *kay,
			      struct ieee802_1x_mka_ki *ki);
int ieee802_1x_kay_enable_tx_sas(struct ieee802_1x_kay *kay,
				 struct ieee802_1x_mka_ki *lki);
int ieee802_1x_kay_enable_rx_sas(struct ieee802_1x_kay *kay,
				 struct ieee802_1x_mka_ki *lki);
int ieee802_1x_kay_enable_new_info(struct ieee802_1x_kay *kay);
int ieee802_1x_kay_get_status(struct ieee802_1x_kay *kay, char *buf,
			      size_t buflen);
int ieee802_1x_kay_get_macsec(struct ieee802_1x_kay *kay, char *buf,
			      size_t buflen);
int ieee802_1x_kay_get_mib(struct ieee802_1x_kay *kay, char *buf,
			   size_t buflen);

#ifdef CONFIG_MODULE_TESTS
int ieee802_1x_kay_module_tests(void);
#endif /* CONFIG_MODULE_TESTS */

#endif /* IEEE802_1X_KAY_H */
