# VANTAGE - The Hyper-Converged Terminal Operating System

**VANTAGE** has been completely overhauled into a next-generation terminal operating system. We have migrated away from complex Bash-heavy multiplexing towards a completely native, blazing fast **WezTerm Plugin**, powered by a compiled **Rust Backend**, an **eBPF Firewall**, a **Quantum-Inspired Database (HYBRID_DB)**, and **Context-Aware AI**.

## 🚀 The 5 Core Pillars (Completed Architecture)

1. **Official WezTerm Plugin UI:** VANTAGE is no longer a hacked-together config file. It is a standardized WezTerm plugin loaded instantly via `wezterm.plugin.require()`. We feature rich interactive `InputSelector` overlays for OSINT, Git, Workspaces, and Health Diagnostics.
2. **Rust Micro-Binary Core:** Heavy shell subprocesses have been replaced by a compiled Rust binary (`vantage-core`). It statically links the MSNET gossip protocol with pure-Rust constant-time cryptography (ChaCha20-Poly1305, HMAC-SHA256).
3. **HYBRID_DB (QIHSE + KEYSTONE):** VANTAGE natively integrates a massive heterogeneous database. It dynamically routes queries using a UMA Dispatcher to either Keystone's deterministic scalar interpolation or QIHSE's quantum-inspired random fourier feature mappings.
4. **eBPF/XDP Firewall:** VANTAGE drops malicious IPs in kernel-space before they ever hit the networking stack, utilizing a 100,000-entry BPF hash map directly queried by the XDP hook `vantage_xdp.c`.
5. **Idle Context Scanner:** An ultra-lightweight C daemon (`indexer_daemon.c`) runs at ~0% CPU, asynchronously tracking your Bash directories, clustering them in RAM, and executing a flush to ZFS only when idle for >30 seconds.

## 📦 Installation & End-to-End Build

VANTAGE provides an end-to-end Makefile to build the Rust core, the C indexer daemon, and the eBPF objects in one swoop.

```bash
cd VANTAGE
make all
```

To run the full end-to-end integration benchmark:
```bash
make bench
```

## ⌨️ Native Hotkeys

- `CTRL+SHIFT+M`: Master VANTAGE UI Menu
- `CTRL+SHIFT+O`: Interactive OSINT Dashboard
- `CTRL+SHIFT+G`: Interactive Git UI
- `CTRL+SHIFT+H`: Interactive Health Diagnostics
- `CTRL+SHIFT+W`: Interactive Workspace Manager
- `CTRL+SHIFT+D`: Workspace Builder (PenTest / Dev Layouts)
- `CTRL+SPACE`: TurboQuant AI Command Suggestions
- `CTRL+SHIFT+S`: Save Context Session / Workspace State
- `CTRL+SHIFT+L`: Load Context Session / Workspace State
- `CTRL+PageUp/Down`: Smooth scrolling through terminal buffers
