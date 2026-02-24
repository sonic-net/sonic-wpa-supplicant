# Copilot Instructions for sonic-wpa-supplicant

## Project Overview

sonic-wpa-supplicant is SONiC's fork of wpa_supplicant and hostapd, providing control plane MACsec (IEEE 802.1AE) support for SONiC switches. MACsec provides hop-by-hop Layer 2 encryption between directly connected switches, ensuring data confidentiality and integrity on the wire.

## Architecture

```
sonic-wpa-supplicant/
├── wpa_supplicant/         # WPA supplicant (client/supplicant side)
│   ├── wpa_supplicant.c    # Main supplicant implementation
│   ├── wpa_cli.c           # CLI tool
│   ├── ctrl_iface.c        # Control interface
│   ├── events.c            # Event handling
│   ├── wpas_kay.c          # MACsec KaY (Key Agreement Entity)
│   └── .config             # Build configuration
├── hostapd/                # Host AP daemon (authenticator side)
│   ├── hostapd.c           # Main hostapd implementation
│   └── .config             # Build configuration
├── src/                    # Shared source
│   ├── pae/                # Port Access Entity (802.1X/MACsec)
│   ├── crypto/             # Cryptographic implementations
│   ├── eap_*               # EAP method implementations
│   ├── common/             # Common utilities
│   └── drivers/            # Driver interfaces
├── debian/                 # Debian packaging
├── build_release           # Release build configuration
├── azure-pipelines.yml     # CI pipeline
└── COPYING                 # BSD license
```

### Key Concepts
- **MACsec (802.1AE)**: Layer 2 encryption protocol for switch-to-switch links
- **MKA (MACsec Key Agreement)**: Protocol for key negotiation between MACsec peers
- **KaY (Key Agreement Entity)**: Component that manages MACsec key agreement
- **wpa_supplicant**: Runs on each switch port participating in MACsec
- **hostapd**: Authenticator role for 802.1X port-based access control
- **EAP**: Extensible Authentication Protocol for 802.1X authentication

## Language & Style

- **Primary language**: C
- **License**: BSD (not GPL — keep license compatible)
- **Indentation**: Tabs
- **Naming conventions**:
  - Functions: `module_action` (e.g., `wpa_supplicant_init`, `ieee802_1x_receive`)
  - Structs: `struct wpa_supplicant`, `struct hostapd_data`
  - Macros: `UPPER_CASE`
- **Build configuration**: `.config` files (similar to Linux kernel Kconfig)
- **Memory**: Manual memory management with `os_malloc`/`os_free` wrappers

## Build Instructions

```bash
# Build wpa_supplicant
cd wpa_supplicant
cp defconfig .config  # or use the SONiC .config
make

# Build hostapd
cd hostapd
cp defconfig .config
make

# Build Debian package
dpkg-buildpackage -rfakeroot -b -us -uc
```

## Testing

- Test MACsec functionality between two SONiC switches with direct connections
- Verify MKA session establishment and key rotation
- Validate MACsec encryption/decryption on data plane traffic
- CI via Azure Pipelines

## PR Guidelines

- **Signed-off-by**: Required on all commits
- **CLA**: Sign Linux Foundation EasyCLA
- **SONiC-specific changes**: Clearly mark SONiC-specific modifications vs upstream wpa_supplicant code
- **Upstream sync**: Consider whether changes should be submitted to upstream wpa_supplicant
- **Security review**: Cryptographic and authentication changes require careful review
- **BSD license**: All new code must be BSD-compatible

## Gotchas

- **Upstream fork**: This is a fork of hostap — track upstream changes and rebase carefully
- **SONiC patches**: SONiC-specific MACsec integration patches are layered on top
- **Cryptographic code**: Changes to crypto implementations can break MACsec security
- **Driver interface**: SONiC uses a custom driver interface for SAI-based MACsec — see `src/drivers/`
- **Key management**: MKA key handling must follow 802.1X-2010 specification precisely
- **Performance**: MACsec encryption is typically hardware-offloaded — software fallback may be slow
- **Config file**: The `.config` build configuration determines which features are compiled in
