#!/usr/bin/env python3
"""
VANTAGE Toggle TUI (Standalone)
--------------------------------
A simple, secure terminal UI for toggling VANTAGE feature modules and core/security options.

- Run: ./vantage_toggles_tui.py
- Uses npyscreen for the interface
- Edits ~/bashrc.postcustom and ~/bashrc.precustom
- Preserves comments and structure, makes backups
- Linux-first, security-focused

Author: VANTAGE Team
"""
import npyscreen
import os
import re
import shutil
import logging

BASHRC_PRECUSTOM = os.path.expanduser('~/bashrc.precustom')
BASHRC_POSTCUSTOM = os.path.expanduser('~/bashrc.postcustom')
BACKUP_SUFFIX = '.bak'

logging.basicConfig(filename=os.path.expanduser('~/logs/toggle_tui.log'),
                    level=logging.INFO,
                    format='%(asctime)s %(levelname)s %(message)s')


def parse_exports(filepath, keys=None):
    exports = {}
    try:
        with open(filepath, 'r') as f:
            for i, line in enumerate(f):
                m = re.match(r'\s*export\s+([A-Za-z0-9_]+)=(\d+)', line)
                if m:
                    var, val = m.group(1), m.group(2)
                    if (keys is None) or (var in keys):
                        exports[var] = (val, i, line.rstrip('\n'))
    except Exception as e:
        logging.error(f"Failed to parse {filepath}: {e}")
    return exports


def update_exports(filepath, updates):
    try:
        with open(filepath, 'r') as f:
            lines = f.readlines()
        for var, new_val in updates.items():
            for i, line in enumerate(lines):
                if re.match(rf'\s*export\s+{re.escape(var)}=', line):
                    lines[i] = re.sub(r'(export\s+' + re.escape(var) + r'=)\d+', rf'\g<1>{new_val}', line)
        shutil.copy2(filepath, filepath + BACKUP_SUFFIX)
        with open(filepath, 'w') as f:
            f.writelines(lines)
        logging.info(f"Updated {filepath}: {updates}")
        return True
    except Exception as e:
        logging.error(f"Failed to update {filepath}: {e}")
        return False


FEATURE_TOGGLES = [
    'VANTAGE_OBFUSCATE_ENABLED',
    'VANTAGE_FZF_ENABLED',
    'VANTAGE_ML_ENABLED',
    'VANTAGE_OSINT_ENABLED',
    'VANTAGE_CYBERSEC_ENABLED',
    'VANTAGE_GITSTAR_ENABLED',
    'VANTAGE_CHAT_ENABLED',
]

# Configuration cache and module system optimization toggles
CONFIG_CACHE_TOGGLES = [
    'VANTAGE_CONFIG_CACHE_ENABLED',
    'VANTAGE_CONFIG_FORCE_REFRESH',
    'VANTAGE_CONFIG_VERIFY_HASH',
    'VANTAGE_MODULE_DEBUG',
    'VANTAGE_MODULE_AUTOLOAD',
    'VANTAGE_MODULE_CACHE_ENABLED',
    'VANTAGE_MODULE_VERIFY'
]

CORE_TOGGLES = [
    'U_BINS', 'U_FUNCS', 'U_ALIASES', 'U_AGENTS', 'ENABLE_LESSPIPE', 'U_MODULES_ENABLE',
    'VENV_AUTO', 'VANTAGE_SECURE_RM', 'VANTAGE_QUIET_MODULES', 'VANTAGE_SECURE_BASH_HISTORY',
    'VANTAGE_SECURE_SSH_KNOWN_HOSTS', 'VANTAGE_SECURE_CLEAN_CACHE', 'VANTAGE_SECURE_BROWSER_CACHE',
    'VANTAGE_SECURE_RECENT', 'VANTAGE_SECURE_VIM_UNDO', 'VANTAGE_SECURE_CLIPBOARD',
    'VANTAGE_SECURE_CLEAR_SCREEN', 'U_LAZY_LOAD', 'BASHRC_PROFILE'
]


class ToggleForm(npyscreen.ActionForm):
    def create(self):
        self.add(npyscreen.FixedText, value="VANTAGE Toggle TUI", editable=False, color='STANDOUT')
        self.add(npyscreen.FixedText, value="(Use arrows/space to toggle, ^S to save, ^Q to quit)", editable=False)
        self.add(
            npyscreen.FixedText,
            value="\n[Security Warning] Only trusted users should edit these files!",
            editable=False,
            color='DANGER')

        self.add(npyscreen.FixedText, value="\nFeature Module Toggles:", editable=False, color='LABEL')
        self.feature_toggles = self.add(npyscreen.TitleMultiSelect, max_height=8, name="Feature Modules",
                                        values=FEATURE_TOGGLES, scroll_exit=True)

        self.add(npyscreen.FixedText, value="\nConfiguration Caching and Module System:", editable=False, color='LABEL')
        self.cache_toggles = self.add(npyscreen.TitleMultiSelect, max_height=8, name="Config Cache/Modules",
                                      values=CONFIG_CACHE_TOGGLES, scroll_exit=True)

        self.add(npyscreen.FixedText, value="\nCore/Security Toggles:", editable=False, color='LABEL')
        self.core_toggles = self.add(npyscreen.TitleMultiSelect, max_height=12, name="Core/Security",
                                     values=CORE_TOGGLES, scroll_exit=True)

        self.status = self.add(npyscreen.FixedText, value="", editable=False, color='CAUTION')
        self.help_btn = self.add(npyscreen.ButtonPress, name="Help", when_pressed_function=self.show_help)
        self.reload_btn = self.add(npyscreen.ButtonPress, name="Reload", when_pressed_function=self.reload)
        self.reload()

    def reload(self):
        self.feature_vals = parse_exports(BASHRC_POSTCUSTOM, FEATURE_TOGGLES)
        self.cache_vals = parse_exports(BASHRC_POSTCUSTOM, CONFIG_CACHE_TOGGLES)
        self.core_vals = parse_exports(BASHRC_PRECUSTOM, CORE_TOGGLES)

        self.feature_toggles.value = [i for i, k in enumerate(
            FEATURE_TOGGLES) if self.feature_vals.get(k, ('0',))[0] == '1']
        self.cache_toggles.value = [i for i, k in enumerate(
            CONFIG_CACHE_TOGGLES) if self.cache_vals.get(k, ('0',))[0] == '1']
        self.core_toggles.value = [i for i, k in enumerate(CORE_TOGGLES) if self.core_vals.get(k, ('0',))[0] == '1']

        self.status.value = ""
        self.display()

    def on_ok(self):
        feature_updates = {k: '1' if i in self.feature_toggles.value else '0' for i, k in enumerate(FEATURE_TOGGLES)}
        cache_updates = {k: '1' if i in self.cache_toggles.value else '0' for i, k in enumerate(CONFIG_CACHE_TOGGLES)}
        core_updates = {k: '1' if i in self.core_toggles.value else '0' for i, k in enumerate(CORE_TOGGLES)}

        # Merge feature and cache updates for postcustom
        postcustom_updates = {**feature_updates, **cache_updates}

        if npyscreen.notify_yes_no("Save changes to toggles? (Backups will be made)", title="Confirm Save"):
            ok1 = update_exports(BASHRC_POSTCUSTOM, postcustom_updates)
            ok2 = update_exports(BASHRC_PRECUSTOM, core_updates)
            if ok1 and ok2:
                self.status.value = "Toggles updated successfully."
            else:
                self.status.value = "Error updating toggles. Check logs."
            self.display()

    def on_cancel(self):
        if npyscreen.notify_yes_no("Exit without saving changes?", title="Exit"):
            self.parentApp.setNextForm(None)

    def show_help(self):
        npyscreen.notify_confirm(
            "Use arrows/space to select toggles.\n"
            "Checked = enabled (1), unchecked = disabled (0).\n"
            "^S to save, ^Q to quit.\n"
            "Backups are made before writing.\n"
            "Security: Only trusted users should edit these files!\n\n"
            "Configuration Caching:\n"
            " - VANTAGE_CONFIG_CACHE_ENABLED: Master toggle for config caching\n"
            " - VANTAGE_MODULE_CACHE_ENABLED: Toggle for module caching\n"
            " - VANTAGE_CONFIG_VERIFY_HASH: Validate config files with MD5\n"
            " - VANTAGE_MODULE_VERIFY: Security verification for modules\n",
            title="Help / Security Notice")


class VantageToggleApp(npyscreen.NPSAppManaged):
    def onStart(self):
        self.addForm('MAIN', ToggleForm)


if __name__ == '__main__':
    try:
        app = VantageToggleApp()
        app.run()
    except Exception as e:
        logging.error(f"Fatal error in TUI: {e}")
        print(f"[ERROR] {e}\nSee ~/logs/toggle_tui.log for details.")
