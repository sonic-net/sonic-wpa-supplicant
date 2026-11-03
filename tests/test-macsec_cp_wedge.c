/*
 * IEEE 802.1X-2010 CP state machine regression test: CP_TRANSMIT must not
 * become a dead-end when usingTransmitSA never asserts (e.g. the transmit SA
 * was deleted by a rekey/peer-flap teardown before enable_tx_sas() ran).
 *
 * Self-contained: #includes the module under test and stubs its KaY/SecY/
 * eloop/logging deps (stub signatures compiler-checked against the headers).
 * Build/run: make -C tests test-macsec_cp_wedge && ./test-macsec_cp_wedge
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "common/defs.h"
#include "common/ieee802_1x_defs.h"
#include "pae/ieee802_1x_kay.h"
#include "pae/ieee802_1x_secy_ops.h"
#include "utils/eloop.h"

/* ------------------------------------------------------------------ *
 * Stubs for the module's external dependencies. Signatures must match
 * the real prototypes (enforced by including the headers above).
 * ------------------------------------------------------------------ */

void wpa_printf(int level, const char *fmt, ...)
{
	(void) level;
	(void) fmt;
}

void * os_zalloc(size_t size)
{
	void *p = malloc(size);

	if (p)
		memset(p, 0, size);
	return p;
}

/* SecY control ops: no-ops for the state-machine logic under test. */
int secy_cp_control_validate_frames(struct ieee802_1x_kay *kay,
				    enum validate_frames vf)
{ (void) kay; (void) vf; return 0; }
int secy_cp_control_protect_frames(struct ieee802_1x_kay *kay, bool flag)
{ (void) kay; (void) flag; return 0; }
int secy_cp_control_encrypt(struct ieee802_1x_kay *kay, bool enabled)
{ (void) kay; (void) enabled; return 0; }
int secy_cp_control_replay(struct ieee802_1x_kay *kay, bool flag, u32 win)
{ (void) kay; (void) flag; (void) win; return 0; }
int secy_cp_control_current_cipher_suite(struct ieee802_1x_kay *kay, u64 cs)
{ (void) kay; (void) cs; return 0; }
int secy_cp_control_confidentiality_offset(struct ieee802_1x_kay *kay,
					   enum confidentiality_offset co)
{ (void) kay; (void) co; return 0; }
int secy_cp_control_enable_port(struct ieee802_1x_kay *kay, bool flag)
{ (void) kay; (void) flag; return 0; }

/*
 * KaY ops: no-ops. Crucially ieee802_1x_kay_enable_tx_sas() does NOT assert
 * usingTransmitSA here -- that models the field failure where the transmit SA
 * is gone by the time CP_TRANSMIT runs.
 */
int ieee802_1x_kay_set_latest_sa_attr(struct ieee802_1x_kay *kay,
				      struct ieee802_1x_mka_ki *lki, u8 lan,
				      bool ltx, bool lrx)
{ (void) kay; (void) lki; (void) lan; (void) ltx; (void) lrx; return 0; }
int ieee802_1x_kay_set_old_sa_attr(struct ieee802_1x_kay *kay,
				   struct ieee802_1x_mka_ki *oki,
				   u8 oan, bool otx, bool orx)
{ (void) kay; (void) oki; (void) oan; (void) otx; (void) orx; return 0; }
int ieee802_1x_kay_create_sas(struct ieee802_1x_kay *kay,
			      struct ieee802_1x_mka_ki *lki)
{ (void) kay; (void) lki; return 0; }
int ieee802_1x_kay_delete_sas(struct ieee802_1x_kay *kay,
			      struct ieee802_1x_mka_ki *ki)
{ (void) kay; (void) ki; return 0; }
int ieee802_1x_kay_enable_tx_sas(struct ieee802_1x_kay *kay,
				 struct ieee802_1x_mka_ki *lki)
{ (void) kay; (void) lki; return 0; }
int ieee802_1x_kay_enable_rx_sas(struct ieee802_1x_kay *kay,
				 struct ieee802_1x_mka_ki *lki)
{ (void) kay; (void) lki; return 0; }
int ieee802_1x_kay_enable_new_info(struct ieee802_1x_kay *kay)
{ (void) kay; return 0; }

/* eloop: no-ops. The test drives the SM synchronously; no timers needed. */
int eloop_cancel_timeout(eloop_timeout_handler handler, void *eloop_data,
			 void *user_data)
{ (void) handler; (void) eloop_data; (void) user_data; return 0; }
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler, void *eloop_data,
			   void *user_data)
{ (void) secs; (void) usecs; (void) handler; (void) eloop_data;
  (void) user_data; return 0; }

/* ------------------------------------------------------------------ *
 * Module under test (brings in the static CP struct, states and step).
 * ------------------------------------------------------------------ */
#include "pae/ieee802_1x_cp.c"

/* ------------------------------------------------------------------ */

static const char * cp_state_name(enum cp_states s)
{
	switch (s) {
	case CP_BEGIN: return "BEGIN";
	case CP_INIT: return "INIT";
	case CP_CHANGE: return "CHANGE";
	case CP_ALLOWED: return "ALLOWED";
	case CP_AUTHENTICATED: return "AUTHENTICATED";
	case CP_SECURED: return "SECURED";
	case CP_RECEIVE: return "RECEIVE";
	case CP_RECEIVING: return "RECEIVING";
	case CP_READY: return "READY";
	case CP_TRANSMIT: return "TRANSMIT";
	case CP_TRANSMITTING: return "TRANSMITTING";
	case CP_ABANDON: return "ABANDON";
	case CP_RETIRE: return "RETIRE";
	}
	return "?";
}

static int failures;

#define CHECK(cond, msg) do {						\
	if (cond) {							\
		printf("  PASS: %s\n", (msg));				\
	} else {							\
		printf("  FAIL: %s\n", (msg));				\
		failures++;						\
	}								\
} while (0)

/*
 * Drive an initialised CP SM to CP_TRANSMIT as the elected key server, with
 * usingTransmitSA left unset (enable_tx_sas is a no-op stub). Returns the SM.
 */
static struct ieee802_1x_cp_sm * drive_to_transmit(struct ieee802_1x_kay *kay)
{
	struct ieee802_1x_cp_sm *sm = ieee802_1x_cp_sm_init(kay);

	if (!sm)
		return NULL;

	/* Elected key server, secure connectivity, a SAK to install, and the
	 * receive SAs already in use -> SECURED -> RECEIVE -> RECEIVING ->
	 * TRANSMIT. controlled_port_enabled is still false here, which is the
	 * RECEIVING->TRANSMIT trigger for the elected self. */
	sm->connect = SECURE;
	sm->elected_self = true;
	sm->new_sak = true;
	sm->using_receive_sas = true;
	sm->port_enabled = true;

	ieee802_1x_cp_step_run(sm);
	return sm;
}

/*
 * The bug: CP wedged in CP_TRANSMIT never leaves on a new SAK. A correct CP
 * escapes to ABANDON/RECEIVE. FAILS on the unfixed tree, PASSES on the fixed.
 */
static void test_wedge_escapes_on_new_sak(struct ieee802_1x_kay *kay)
{
	struct ieee802_1x_cp_sm *sm = drive_to_transmit(kay);

	printf("test: escape CP_TRANSMIT on new SAK\n");
	CHECK(sm != NULL, "CP SM initialised");
	if (!sm)
		return;

	printf("  reached state %s (usingTransmitSA=%d)\n",
	       cp_state_name(sm->CP_state), sm->using_transmit_sa);
	CHECK(sm->CP_state == CP_TRANSMIT,
	      "precondition: CP is in CP_TRANSMIT with no transmit SA");

	/* Next rekey: KaY distributes a new SAK while CP is stuck in TRANSMIT.
	 * One SM step decides the fate: the fixed CP takes the new escape to
	 * ABANDON; the buggy CP has no CP_TRANSMIT exit but usingTransmitSA and
	 * stays put. */
	sm->new_sak = true;
	sm_CP_Step(sm);

	printf("  after new SAK, state %s\n", cp_state_name(sm->CP_state));
	CHECK(sm->CP_state != CP_TRANSMIT,
	      "CP left CP_TRANSMIT after new SAK (did not wedge)");

	ieee802_1x_cp_sm_deinit(sm);
}

/*
 * Healthy path: when the transmit SA asserts, CP_TRANSMIT must still advance
 * to CP_TRANSMITTING (the escape branch must not steal the transition).
 */
static void test_healthy_path_reaches_transmitting(struct ieee802_1x_kay *kay)
{
	struct ieee802_1x_cp_sm *sm = drive_to_transmit(kay);

	printf("test: healthy path CP_TRANSMIT -> CP_TRANSMITTING\n");
	CHECK(sm != NULL, "CP SM initialised");
	if (!sm)
		return;
	CHECK(sm->CP_state == CP_TRANSMIT, "precondition: CP is in CP_TRANSMIT");

	/* KaY confirms the transmit SA is installed and in use. */
	sm->using_transmit_sa = true;
	sm_CP_Step(sm);

	printf("  after usingTransmitSA, state %s\n",
	       cp_state_name(sm->CP_state));
	CHECK(sm->CP_state == CP_TRANSMITTING,
	      "CP advanced to CP_TRANSMITTING on transmit SA");

	ieee802_1x_cp_sm_deinit(sm);
}

/*
 * ABANDON must clear usingTransmitSA: the escape diverts around TRANSMITTING
 * (the only other place the flag is cleared), and a stale 'true' could enter
 * TRANSMITTING before the new transmit SA is installed.
 */
static void test_abandon_clears_using_transmit_sa(struct ieee802_1x_kay *kay)
{
	struct ieee802_1x_cp_sm *sm = drive_to_transmit(kay);

	printf("test: ABANDON clears usingTransmitSA\n");
	CHECK(sm != NULL, "CP SM initialised");
	if (!sm)
		return;
	CHECK(sm->CP_state == CP_TRANSMIT, "precondition: CP is in CP_TRANSMIT");

	/* Transmit SA asserted, then a new SAK lands in the same window: the CP
	 * diverts CP_TRANSMIT -> ABANDON rather than -> TRANSMITTING. */
	sm->using_transmit_sa = true;
	sm->new_sak = true;
	sm_CP_Step(sm);

	printf("  after new SAK, state %s, usingTransmitSA=%d\n",
	       cp_state_name(sm->CP_state), sm->using_transmit_sa);
	CHECK(sm->CP_state == CP_ABANDON,
	      "CP diverted CP_TRANSMIT -> ABANDON on new SAK");
	CHECK(!sm->using_transmit_sa,
	      "ABANDON cleared usingTransmitSA (no stale true into next cycle)");

	ieee802_1x_cp_sm_deinit(sm);
}

int main(void)
{
	struct ieee802_1x_kay kay;

	memset(&kay, 0, sizeof(kay));

	test_wedge_escapes_on_new_sak(&kay);
	test_healthy_path_reaches_transmitting(&kay);
	test_abandon_clears_using_transmit_sa(&kay);

	printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED",
	       failures, failures == 1 ? "" : "s");
	return failures ? 1 : 0;
}
