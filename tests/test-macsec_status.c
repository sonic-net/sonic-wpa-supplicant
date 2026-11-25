/*
 * Check the MACSEC control reply against the full text and every shorter
 * buffer size. The control interface sends the number of bytes returned.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "utils/includes.h"
#include "utils/common.h"
#include "common/defs.h"
#include "common/ieee802_1x_defs.h"
#include "pae/ieee802_1x_kay.h"

int main(void)
{
	static const char expected[] =
		"Cipher Suite: GCM-AES-128\tEncryption: No\n"
		"Include SCI: No\n"
		"Replay Protect: No\tReplay Window: 0\n"
		"\tTXSC: 00:00:00:00:00:00@0\n"
		"\t\tAN: 0\tActive: No\tPN: 0\n"
		"\tRXSC: 00:00:00:00:00:00@0\n"
		"\t\tAN: 0\tActive: No\tPN: 0\n";
	struct ieee802_1x_kay kay = { 0 };
	struct transmit_sc tx = { 0 };
	struct transmit_sa txsa = { 0 };
	struct receive_sc rx = { 0 };
	struct receive_sa rxsa = { 0 };
	char reply[sizeof(expected) + 2];
	size_t capacity;

	kay.txsc = &tx;
	dl_list_init(&tx.sa_list);
	dl_list_add(&tx.sa_list, &txsa.list);
	dl_list_init(&kay.rxsc_list);
	dl_list_add(&kay.rxsc_list, &rx.list);
	dl_list_init(&rx.sa_list);
	dl_list_add(&rx.sa_list, &rxsa.list);

	for (capacity = 0; capacity < sizeof(reply); capacity++) {
		size_t length = capacity ? capacity - 1 : 0;
		int ret;

		if (length > sizeof(expected) - 1)
			length = sizeof(expected) - 1;
		memset(reply, '!', sizeof(reply));
		ret = ieee802_1x_kay_get_macsec(&kay, reply, capacity);
		if (ret != (int) length || memcmp(reply, expected, length) ||
		    (capacity && reply[length] != '\0') || reply[capacity] != '!') {
			fprintf(stderr,
				"MACSEC reply mismatch at capacity %zu (expected %zu bytes, returned %d)\n",
				capacity, length, ret);
			return 1;
		}
	}
	return 0;
}
