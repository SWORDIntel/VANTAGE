# SENTINEL - WezTerm Edition

**SENTINEL** has been completely overhauled into a next-generation terminal operating system. We have migrated away from complex Bash-heavy multiplexing towards a completely native, blazing fast **WezTerm Plugin**, powered by a compiled **Rust Backend** and **Context-Aware AI**.

## 🚀 The 3 Core Pillars (Phase 3 Architecture)

1. **Official WezTerm Plugin:** SENTINEL is no longer a hacked-together config file. It is a standardized WezTerm plugin loaded instantly via `wezterm.plugin.require()`.
2. **Rust Micro-Binary Core:** Heavy shell subprocesses (like status checks and system parsing) have been replaced by a compiled Rust binary (`sentinel-core`) that natively streams JSON state directly to the Lua UI with zero latency.
3. **TurboQuant AI Autocomplete:** By pressing `CTRL+SPACE`, SENTINEL scrapes your screen context, detects your hardware (`nvidia-smi`), and generates contextual shell commands via a native WezTerm dropdown UI. 
    - *Hardware Adaptive*: If you have a dedicated GPU, it uses full GPU KV-Caching offload via Ollama. If you don't, it natively routes through the `framewerx` AEGIS-LAB engines to use CPU/VPU optimized TurboQuant inference!

## 📦 Installation

SENTINEL still provides a rich UI bootstrapper to automatically wire up the `thin_bash` hooks and compile the Rust backend.

1. Install [WezTerm](https://wezfurlong.org/wezterm/install/linux.html) and `cargo`.
2. Run the bootstrap script:
```bash
./install.sh
```
3. Launch `wezterm`.

*(To completely purge the system or reinstall, you can use `./uninstall.sh` or `./reinstall.sh` respectively).*

## ⌨️ Native Hotkeys

- `CTRL+SHIFT+M`: Master SENTINEL UI Menu
- `CTRL+SPACE`: TurboQuant AI Command Suggestions
- `CTRL+SHIFT+S`: Save Context Session / Workspace State
- `CTRL+SHIFT+L`: Load Context Session / Workspace State
- `CTRL+PageUp/Down`: Smooth scrolling through terminal buffers
