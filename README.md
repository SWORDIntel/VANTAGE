# SENTINEL - WezTerm Edition

**SENTINEL** has been completely overhauled. We have migrated away from complex Bash-heavy multiplexing towards a completely native, blazing fast **WezTerm + Lua** architecture.

## The 8 Core Pillars
1. **LLM Chat Sidebar:** Native WezTerm pane splitting to run an AI chat seamlessly alongside your terminal.
2. **Context Sessions:** Automatic workspace detection. Launch into `PenTest` or `AWS` workspaces and your UI natively turns Red to indicate privilege escalation context.
3. **OSINT Dashboard:** Instantly fracture your terminal into a 4-pane layout to run simultaneous investigations.
4. **Command Chains:** Fuzzy-find complex exploit chains and inject them directly into your prompt.
5. **AI Suggestions:** Ask your local LLM to explain highlighted commands or suggest new ones, delivered directly via native WezTerm Toast overlays.
6. **Instant Resurrection:** Using `wezterm-resurrect`, your workflow state is preserved across reboots.
7. **Native Git UI:** Background Git status checks delivered as native terminal overlays.
8. **Global Syncing:** All configurations are now stored cleanly in Lua for easy syncing across all your machines.

## Installation

1. Install [WezTerm](https://wezfurlong.org/wezterm/install/linux.html).
2. Run the bootstrap script:
```bash
./install.sh
```
3. Launch `wezterm`.

## Architecture Note
This repo replaces thousands of lines of legacy bash logic. Heavy logic and state management are now offloaded to `~/.config/wezterm/sentinel/`, while primitive shell features (aliases and autocompletes) remain in `~/.config/sentinel/`.
