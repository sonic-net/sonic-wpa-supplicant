/*
 * Unit test: MKPDUs dropped for an unknown CKN must be counted, logged above
 * MSG_DEBUG, rate-limited, and accompanied by the offending CKN.
 *
 * ieee802_1x_kay_mkpdu_sanity_check() drops an MKPDU whose CKN matches no
 * local CA. The drop happens before ICV verification, so it produces no ICV
 * error either; logged at MSG_DEBUG it was invisible at a deployed log level,
 * and a one-byte CKN corruption tore MACsec down silently.
 *
 * The test drives that path directly and asserts the four properties an
 * operator depends on: the per-port counter advances on every drop, the first
 * drop is always logged, subsequent drops inside the same minute are not, and
 * the log carries the CKN actually received (so a corrupted CKN can be told
 * apart from a peer in a different CA). A matching CKN must do none of this.
 *
 * Self-contained: it #includes the module under test and stubs its KaY / SecY /
 * eloop / crypto / l2_packet dependencies, so no MACsec hardware or wpa runtime
 * is needed. time() is redirected to a test clock to exercise the rate limit
 * without sleeping.
 *
 * Build/run: see tests/Makefile target `test-macsec_unknown_ckn`.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "common/defs.h"
#include "common/ieee802_1x_defs.h"
#include "common/eapol_common.h"
#include "l2_packet/l2_packet.h"
#include "pae/ieee802_1x_kay.h"

/* ------------------------------------------------------------------ *
 * Test clock. The rate limit compares time(NULL) against the last log
 * timestamp; redirecting time() lets the test cross the 60 s boundary
 * without sleeping. Must be defined before the module is included.
 * ------------------------------------------------------------------ */

static time_t test_clock = 1000000;

static time_t test_time(time_t *t)
{
	if (t)
		*t = test_clock;
	return test_clock;
}

#define time(t) test_time(t)

/* ------------------------------------------------------------------ *
 * Captured log output. wpa_printf()/wpa_hexdump() below apply the same
 * level filter the real ones do, with the threshold pinned at MSG_INFO
 * (wpa_supplicant's default). A drop logged at MSG_DEBUG is therefore
 * dropped here too -- which is precisely the pre-fix behaviour.
 * ------------------------------------------------------------------ */

#define TEST_LOG_LEVEL MSG_INFO

/* Mirrors DEFAULT_ICV_LEN, which is private to the module. */
#define DEFAULT_ICV_LEN_STUB 16

static char last_msg[512];
static int msg_count;
static u8 last_dump[MAX_CKN_LEN];
static size_t last_dump_len;
static int dump_count;

static void reset_capture(void)
{
	os_memset(last_msg, 0, sizeof(last_msg));
	os_memset(last_dump, 0, sizeof(last_dump));
	last_dump_len = 0;
	msg_count = 0;
	dump_count = 0;
}

void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;

	if (level < TEST_LOG_LEVEL)
		return;
	va_start(ap, fmt);
	vsnprintf(last_msg, sizeof(last_msg), fmt, ap);
	va_end(ap);
	msg_count++;
}

void wpa_hexdump(int level, const char *title, const void *buf, size_t len)
{
	if (level < TEST_LOG_LEVEL)
		return;
	last_dump_len = len > sizeof(last_dump) ? sizeof(last_dump) : len;
	os_memcpy(last_dump, buf, last_dump_len);
	dump_count++;
}

void wpa_hexdump_key(int level, const char *title, const void *buf, size_t len)
{
	wpa_hexdump(level, title, buf, len);
}

/* ------------------------------------------------------------------ *
 * Real implementations the module needs for behaviour under test.
 * ------------------------------------------------------------------ */

void * os_zalloc(size_t size)
{
	void *p = malloc(size);

	if (p)
		memset(p, 0, size);
	return p;
}

size_t os_strlcpy(char *dest, const char *src, size_t siz)
{
	size_t slen = strlen(src);

	if (siz) {
		size_t n = slen >= siz ? siz - 1 : slen;

		memcpy(dest, src, n);
		dest[n] = '\0';
	}
	return slen;
}

int os_memcmp_const(const void *a, const void *b, size_t len)
{
	return memcmp(a, b, len);
}

int os_get_time(struct os_time *t)
{
	t->sec = test_clock;
	t->usec = 0;
	return 0;
}

int os_get_random(unsigned char *buf, size_t len)
{
	memset(buf, 0xab, len);
	return 0;
}

unsigned long os_random(void)
{
	return 0;
}

/* ------------------------------------------------------------------ *
 * No-op stubs. Signatures are checked by the compiler against the real
 * headers included above. None of these are reached on the path under
 * test; they exist only to satisfy the linker.
 * ------------------------------------------------------------------ */

int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler, void *eloop_data,
			   void *user_data)
{ (void) secs; (void) usecs; (void) handler; (void) eloop_data;
  (void) user_data; return 0; }
int eloop_cancel_timeout(eloop_timeout_handler handler, void *eloop_data,
			 void *user_data)
{ (void) handler; (void) eloop_data; (void) user_data; return 0; }

struct l2_packet_data * l2_packet_init(
	const char *ifname, const u8 *own_addr, unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr, const u8 *buf,
			    size_t len),
	void *rx_callback_ctx, int l2_hdr)
{ (void) ifname; (void) own_addr; (void) protocol; (void) rx_callback;
  (void) rx_callback_ctx; (void) l2_hdr; return NULL; }
void l2_packet_deinit(struct l2_packet_data *l2)
{ (void) l2; }
int l2_packet_send(struct l2_packet_data *l2, const u8 *dst_addr,
		   u16 proto, const u8 *buf, size_t len)
{ (void) l2; (void) dst_addr; (void) proto; (void) buf; (void) len; return 0; }

int aes_wrap(const u8 *kek, size_t kek_len, int n, const u8 *plain, u8 *cipher)
{ (void) kek; (void) kek_len; (void) n; (void) plain; (void) cipher; return 0; }
int aes_unwrap(const u8 *kek, size_t kek_len, int n, const u8 *cipher, u8 *plain)
{ (void) kek; (void) kek_len; (void) n; (void) cipher; (void) plain; return 0; }

int ieee802_1x_cak_aes_cmac(const u8 *msk, size_t msk_bytes, const u8 *mac1,
			    const u8 *mac2, u8 *cak, size_t cak_bytes)
{ (void) msk; (void) msk_bytes; (void) mac1; (void) mac2; (void) cak;
  (void) cak_bytes; return 0; }
int ieee802_1x_ckn_aes_cmac(const u8 *msk, size_t msk_bytes, const u8 *mac1,
			    const u8 *mac2, const u8 *sid, size_t sid_bytes,
			    u8 *ckn)
{ (void) msk; (void) msk_bytes; (void) mac1; (void) mac2; (void) sid;
  (void) sid_bytes; (void) ckn; return 0; }
int ieee802_1x_kek_aes_cmac(const u8 *cak, size_t cak_bytes, const u8 *ckn,
			    size_t ckn_bytes, u8 *kek, size_t kek_bytes)
{ (void) cak; (void) cak_bytes; (void) ckn; (void) ckn_bytes; (void) kek;
  (void) kek_bytes; return 0; }
int ieee802_1x_ick_aes_cmac(const u8 *cak, size_t cak_bytes, const u8 *ckn,
			    size_t ckn_bytes, u8 *ick, size_t ick_bytes)
{ (void) cak; (void) cak_bytes; (void) ckn; (void) ckn_bytes; (void) ick;
  (void) ick_bytes; return 0; }
/* Fills its output: leaving it untouched makes callers look like they read
 * uninitialised memory, which is an artefact of stubbing, not of the module. */
int ieee802_1x_icv_aes_cmac(const u8 *ick, size_t ick_bytes, const u8 *msg,
			    size_t msg_bytes, u8 *icv)
{ (void) ick; (void) ick_bytes; (void) msg; (void) msg_bytes;
  memset(icv, 0, DEFAULT_ICV_LEN_STUB); return 0; }
int ieee802_1x_sak_aes_cmac(const u8 *cak, size_t cak_bytes, const u8 *ctx,
			    size_t ctx_bytes, u8 *sak, size_t sak_bytes)
{ (void) cak; (void) cak_bytes; (void) ctx; (void) ctx_bytes; (void) sak;
  (void) sak_bytes; return 0; }

/* Minimal but real: returning NULL here would make every caller look like it
 * dereferences a null pointer, which is an artefact of stubbing. */
struct wpabuf * wpabuf_alloc(size_t len)
{
	struct wpabuf *b = os_zalloc(sizeof(*b) + len);

	if (b) {
		b->size = len;
		b->buf = (u8 *) (b + 1);
	}
	return b;
}
void wpabuf_free(struct wpabuf *buf)
{ free(buf); }
void * wpabuf_put(struct wpabuf *buf, size_t len)
{
	void *p = buf->buf + buf->used;

	buf->used += len;
	return p;
}
int wpa_snprintf_hex(char *buf, size_t buf_size, const u8 *data, size_t len)
{ (void) buf; (void) buf_size; (void) data; (void) len; return 0; }

/* SecY control/data ops: never reached on the drop path. */
#define SECY_STUB_KAY(name) \
	int name(struct ieee802_1x_kay *kay) { (void) kay; return 0; }

int secy_cp_control_protect_frames(struct ieee802_1x_kay *kay, bool flag)
{ (void) kay; (void) flag; return 0; }
int secy_cp_control_replay(struct ieee802_1x_kay *kay, bool flag, u32 win)
{ (void) kay; (void) flag; (void) win; return 0; }
int secy_cp_control_current_cipher_suite(struct ieee802_1x_kay *kay, u64 cs)
{ (void) kay; (void) cs; return 0; }
SECY_STUB_KAY(secy_init_macsec)
SECY_STUB_KAY(secy_deinit_macsec)
int secy_get_capability(struct ieee802_1x_kay *kay, enum macsec_cap *cap)
{ (void) kay; *cap = MACSEC_CAP_NOT_IMPLEMENTED; return 0; }
int secy_get_max_sa_per_sc(struct ieee802_1x_kay *kay, enum max_sa_per_sc *max)
{ (void) kay; *max = MAX_SA_PER_SC_2; return 0; }

int secy_create_receive_sc(struct ieee802_1x_kay *kay,
			   struct receive_sc *rxsc)
{ (void) kay; (void) rxsc; return 0; }
int secy_delete_receive_sc(struct ieee802_1x_kay *kay,
			   struct receive_sc *rxsc)
{ (void) kay; (void) rxsc; return 0; }
int secy_create_receive_sa(struct ieee802_1x_kay *kay, struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_delete_receive_sa(struct ieee802_1x_kay *kay, struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_enable_receive_sa(struct ieee802_1x_kay *kay, struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_disable_receive_sa(struct ieee802_1x_kay *kay, struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_get_receive_lowest_pn(struct ieee802_1x_kay *kay,
			       struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_set_receive_lowest_pn(struct ieee802_1x_kay *kay,
			       struct receive_sa *rxsa)
{ (void) kay; (void) rxsa; return 0; }
int secy_create_transmit_sc(struct ieee802_1x_kay *kay,
			    struct transmit_sc *txsc)
{ (void) kay; (void) txsc; return 0; }
int secy_delete_transmit_sc(struct ieee802_1x_kay *kay,
			    struct transmit_sc *txsc)
{ (void) kay; (void) txsc; return 0; }
int secy_create_transmit_sa(struct ieee802_1x_kay *kay,
			    struct transmit_sa *txsa)
{ (void) kay; (void) txsa; return 0; }
int secy_delete_transmit_sa(struct ieee802_1x_kay *kay,
			    struct transmit_sa *txsa)
{ (void) kay; (void) txsa; return 0; }
int secy_enable_transmit_sa(struct ieee802_1x_kay *kay,
			    struct transmit_sa *txsa)
{ (void) kay; (void) txsa; return 0; }
int secy_disable_transmit_sa(struct ieee802_1x_kay *kay,
			     struct transmit_sa *txsa)
{ (void) kay; (void) txsa; return 0; }
int secy_get_transmit_next_pn(struct ieee802_1x_kay *kay,
			      struct transmit_sa *txsa)
{ (void) kay; (void) txsa; return 0; }

/* CP state machine: never driven on the drop path. */
void ieee802_1x_cp_sm_deinit(struct ieee802_1x_cp_sm *sm)
{ (void) sm; }
void ieee802_1x_cp_sm_step(void *cp_ctx)
{ (void) cp_ctx; }
void ieee802_1x_cp_connect_pending(void *cp_ctx)
{ (void) cp_ctx; }
void ieee802_1x_cp_connect_authenticated(void *cp_ctx)
{ (void) cp_ctx; }
void ieee802_1x_cp_connect_secure(void *cp_ctx)
{ (void) cp_ctx; }
void ieee802_1x_cp_set_allreceiving(void *cp_ctx, bool status)
{ (void) cp_ctx; (void) status; }
void ieee802_1x_cp_set_ciphersuite(void *cp_ctx, u64 cs)
{ (void) cp_ctx; (void) cs; }
void ieee802_1x_cp_set_distributedan(void *cp_ctx, u8 an)
{ (void) cp_ctx; (void) an; }
void ieee802_1x_cp_set_distributedki(void *cp_ctx, const struct ieee802_1x_mka_ki *ki)
{ (void) cp_ctx; (void) ki; }
void ieee802_1x_cp_set_electedself(void *cp_ctx, bool status)
{ (void) cp_ctx; (void) status; }
void ieee802_1x_cp_set_offset(void *cp_ctx, enum confidentiality_offset co)
{ (void) cp_ctx; (void) co; }
void ieee802_1x_cp_set_servertransmitting(void *cp_ctx, bool status)
{ (void) cp_ctx; (void) status; }
void ieee802_1x_cp_set_usingreceivesas(void *cp_ctx, bool status)
{ (void) cp_ctx; (void) status; }
void ieee802_1x_cp_set_usingtransmitas(void *cp_ctx, bool status)
{ (void) cp_ctx; (void) status; }
void ieee802_1x_cp_signal_chgdserver(void *cp_ctx)
{ (void) cp_ctx; }
void ieee802_1x_cp_signal_newsak(void *cp_ctx)
{ (void) cp_ctx; }
struct ieee802_1x_cp_sm * ieee802_1x_cp_sm_init(struct ieee802_1x_kay *kay)
{ (void) kay; return NULL; }

/* The module under test. */
#include "pae/ieee802_1x_kay.c"

/* ------------------------------------------------------------------ *
 * Test fixture
 * ------------------------------------------------------------------ */

#define TEST_CKN_LEN 32

/* Differ only in the last byte -- the single-byte corruption this guards. */
static const u8 ckn_local[TEST_CKN_LEN] = {
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x34
};
static const u8 ckn_peer[TEST_CKN_LEN] = {
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35
};

/* eth(14) + eapol hdr(4) + MKA hdr(4) + basic body(60) + ICV(16) */
#define TEST_FRAME_LEN 98

static u8 frame[TEST_FRAME_LEN];

static void build_mkpdu(const u8 *ckn, size_t ckn_len)
{
	struct ieee8023_hdr *eth_hdr = (struct ieee8023_hdr *) frame;
	struct ieee802_1x_hdr *eapol_hdr =
		(struct ieee802_1x_hdr *) (eth_hdr + 1);
	struct ieee802_1x_mka_basic_body *body =
		(struct ieee802_1x_mka_basic_body *) (eapol_hdr + 1);
	size_t body_len = (sizeof(*body) - MKA_HDR_LEN) + ckn_len;
	size_t mka_msg_len = MKA_HDR_LEN + body_len + DEFAULT_ICV_LEN;

	os_memset(frame, 0, sizeof(frame));

	os_memcpy(eth_hdr->dest, pae_group_addr, ETH_ALEN);
	os_memset(eth_hdr->src, 0x22, ETH_ALEN);
	eth_hdr->ethertype = host_to_be16(ETH_P_EAPOL);

	eapol_hdr->version = 3;
	eapol_hdr->type = IEEE802_1X_TYPE_EAPOL_MKA;
	eapol_hdr->length = host_to_be16(mka_msg_len);

	body->version = 3;
	body->priority = 64;
	set_mka_param_body_len(body, body_len);
	os_memcpy(body->algo_agility, mka_algo_agility,
		  sizeof(body->algo_agility));
	os_memcpy(body->ckn, ckn, ckn_len);
}

static struct ieee802_1x_kay kay;
static struct ieee802_1x_mka_participant participant;

static void fixture_init(void)
{
	os_memset(&kay, 0, sizeof(kay));
	dl_list_init(&kay.participant_list);
	os_strlcpy(kay.if_name, "test0", sizeof(kay.if_name));
	test_clock = 1000000;
	reset_capture();
}

static void fixture_add_local_ca(void)
{
	os_memset(&participant, 0, sizeof(participant));
	participant.ckn.len = TEST_CKN_LEN;
	os_memcpy(participant.ckn.name, ckn_local, TEST_CKN_LEN);
	dl_list_add(&kay.participant_list, &participant.list);
}

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d: %s\n", __func__, __LINE__, #cond); \
		return 1; \
	} \
} while (0)

/*
 * A CKN in no local CA must be counted and reported at a level an operator
 * sees by default, and the report must carry the CKN that arrived.
 */
static int test_unknown_ckn_counts_and_logs(void)
{
	fixture_init();
	build_mkpdu(ckn_peer, TEST_CKN_LEN);

	CHECK(ieee802_1x_kay_mkpdu_sanity_check(&kay, frame, sizeof(frame)) ==
	      -1);
	CHECK(kay.mkpdu_unknown_ckn == 1);
	/* Pre-fix this was MSG_DEBUG, i.e. msg_count would be 0 here. */
	CHECK(msg_count == 1);
	CHECK(strstr(last_msg, "test0") != NULL);
	CHECK(dump_count == 1);
	CHECK(last_dump_len == TEST_CKN_LEN);
	/* The CKN received, not the one configured -- that difference is the
	 * whole diagnostic value. */
	CHECK(os_memcmp(last_dump, ckn_peer, TEST_CKN_LEN) == 0);

	printf("PASS %s\n", __func__);
	return 0;
}

/*
 * MKPDUs arrive about every 2 s, so a persistent bad CKN must not flood the
 * log -- but every drop must still be counted.
 */
static int test_unknown_ckn_log_is_rate_limited(void)
{
	int i;

	fixture_init();
	build_mkpdu(ckn_peer, TEST_CKN_LEN);

	/* First drop is always logged. */
	CHECK(ieee802_1x_kay_mkpdu_sanity_check(&kay, frame, sizeof(frame)) ==
	      -1);
	CHECK(msg_count == 1);
	CHECK(dump_count == 1);

	/* 29 more drops inside the same minute: counted, not logged. */
	for (i = 0; i < 29; i++) {
		test_clock += 2;
		CHECK(ieee802_1x_kay_mkpdu_sanity_check(&kay, frame,
						       sizeof(frame)) == -1);
	}
	CHECK(kay.mkpdu_unknown_ckn == 30);
	CHECK(msg_count == 1);
	CHECK(dump_count == 1);

	/* Crossing 60 s since the last log re-arms it. */
	test_clock += 2;
	CHECK(ieee802_1x_kay_mkpdu_sanity_check(&kay, frame, sizeof(frame)) ==
	      -1);
	CHECK(kay.mkpdu_unknown_ckn == 31);
	CHECK(msg_count == 2);
	CHECK(dump_count == 2);

	printf("PASS %s\n", __func__);
	return 0;
}

/*
 * The counter must be a signal, not noise: a CKN that does match a local CA
 * must neither count nor log.
 */
static int test_known_ckn_is_silent(void)
{
	fixture_init();
	fixture_add_local_ca();
	build_mkpdu(ckn_local, TEST_CKN_LEN);

	(void) ieee802_1x_kay_mkpdu_sanity_check(&kay, frame, sizeof(frame));
	CHECK(kay.mkpdu_unknown_ckn == 0);
	CHECK(dump_count == 0);

	printf("PASS %s\n", __func__);
	return 0;
}

int main(void)
{
	int ret = 0;

	ret |= test_unknown_ckn_counts_and_logs();
	ret |= test_unknown_ckn_log_is_rate_limited();
	ret |= test_known_ckn_is_silent();

	if (ret)
		printf("FAILED\n");
	else
		printf("ALL TESTS PASSED\n");
	return ret;
}
