import argparse
import os
import re
import sys
from pathlib import Path

import yaml


_SAFE_ENV_RE = re.compile(r"^[A-Z_][A-Z0-9_]*$")
_SECTION_KEY_MAP: dict[str, dict[str, str]] = {
    "performance": {
        "lazy_load": "LAZY_LOAD",
        "parallel_loading": "SENTINEL_PARALLEL_LOADING",
        "parallel_max_jobs": "SENTINEL_PARALLEL_MAX_JOBS",
    },
    "security": {
        "module_verify": "SENTINEL_MODULE_VERIFY",
        "tools_allowed_commands": "SENTINEL_TOOLS_ALLOWED_COMMANDS",
        "secure_bash_history": "SENTINEL_SECURE_BASH_HISTORY",
        "secure_ssh_known_hosts": "SENTINEL_SECURE_SSH_KNOWN_HOSTS",
        "secure_clean_cache": "SENTINEL_SECURE_CLEAN_CACHE",
        "secure_browser_cache": "SENTINEL_SECURE_BROWSER_CACHE",
        "secure_recent": "SENTINEL_SECURE_RECENT",
        "secure_vim_undo": "SENTINEL_SECURE_VIM_UNDO",
        "secure_clipboard": "SENTINEL_SECURE_CLIPBOARD",
        "clear_screen_on_logout": "SENTINEL_CLEAR_SCREEN_ON_LOGOUT",
        "secure_dirs": "SENTINEL_SECURE_DIRS",
        "workspace_temp": "SENTINEL_WORKSPACE_TEMP",
    },
    "modules": {
        "debug": "SENTINEL_MODULE_DEBUG",
        "autoload": "SENTINEL_MODULE_AUTOLOAD",
        "cache_enabled": "SENTINEL_MODULE_CACHE_ENABLED",
        "enabled_modules": "SENTINEL_MODULES_ENABLED",
        "lazy_load_modules": "SENTINEL_LAZY_LOAD_MODULES",
        "core_modules": "SENTINEL_CORE_MODULES",
    },
    "obfuscation": {
        "enabled": "SENTINEL_OBFUSCATE_ENABLED",
        "output_dir": "OBFUSCATE_OUTPUT_DIR",
    },
    "hashcat": {
        "bin": "HASHCAT_BIN",
        "wordlists_dir": "HASHCAT_WORDLISTS_DIR",
        "output_dir": "HASHCAT_OUTPUT_DIR",
    },
    "distcc": {
        "hosts": "DISTCC_HOSTS",
        "ccache_size": "CCACHE_SIZE",
    },
}


def _shell_single_quote(value: str) -> str:
    """
    Return a single-quoted shell string, safely escaping single quotes.
    e.g. abc'd -> 'abc'"'"'d'
    """
    return "'" + value.replace("'", "'\"'\"'") + "'"


def _safe_env_name(name: str) -> str | None:
    # Uppercase and replace unsafe characters with underscores
    n = re.sub(r"[^A-Z0-9_]", "_", name.upper())
    if not _SAFE_ENV_RE.match(n):
        return None
    return n

def parse_yaml(file_path: str | os.PathLike[str] | Path):
    file_path = Path(file_path)
    with file_path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def _iter_kv(settings, prefix: str | None = None):
    """
    Iterate key/value pairs. For nested dicts, flatten using PREFIX_KEY naming.
    """
    if isinstance(settings, dict):
        for k, v in settings.items():
            k_str = str(k)
            if prefix:
                yield from _iter_kv(v, f"{prefix}_{k_str}")
            else:
                yield from _iter_kv(v, k_str)
    else:
        # Leaf
        yield (prefix, settings)

def export_variables(config, *, emit_output: bool = True) -> list[str]:
    """
    Generate safe `export KEY='VALUE'` lines.

    Backwards compatibility:
    - For standard top-level sections with leaf keys, export by leaf key only
      (matches the historical behavior).
    - For nested dicts, export using flattened names (e.g. FABRIC_TELEMETRY_ENABLED).
    """
    lines: list[str] = []

    def emit(var_name: str, value) -> None:
        safe_name = _safe_env_name(var_name)
        if not safe_name:
            # Refuse to emit unsafe env var names (prevents injection via config keys)
            return

        if value is None:
            value_str = ""
        elif isinstance(value, bool):
            value_str = "1" if value else "0"
        elif isinstance(value, (int, float)):
            value_str = str(value)
        elif isinstance(value, (list, tuple)):
            value_str = " ".join(str(x) for x in value)
        else:
            value_str = str(value)

        # Replace ~ with $HOME for shell compatibility
        value_str = value_str.replace("~", "$HOME")
        lines.append(f"export {safe_name}={_shell_single_quote(value_str)}")

    for section, settings in (config or {}).items():
        section_name = str(section)
        if not isinstance(settings, dict):
            emit(section_name, settings)
            continue

        # SECURITY/COMPAT: Fabric config is exported with a FABRIC_ prefix to
        # prevent collisions with other sections (many have "enabled", etc.).
        if section_name == "fabric":
            base = "FABRIC"
            for key, value in settings.items():
                k = str(key)
                if isinstance(value, dict):
                    for flat_key, flat_val in _iter_kv(value, prefix=f"{base}_{k}"):
                        if flat_key:
                            emit(flat_key, flat_val)
                else:
                    emit(f"{base}_{k}", value)
            continue

        section_map = _SECTION_KEY_MAP.get(section_name, {})
        for key, value in settings.items():
            key_name = str(key)
            mapped_name = section_map.get(key_name)
            if mapped_name:
                emit(mapped_name, value)
                continue

            if isinstance(value, dict):
                for flat_key, flat_val in _iter_kv(value, prefix=key_name):
                    if flat_key:
                        prefix = section_name if section_name not in {"general", "paths"} else None
                        emit(f"{prefix}_{flat_key}" if prefix else flat_key, flat_val)
            else:
                if section_name in {"general", "paths"}:
                    emit(key_name, value)
                elif section_name in {"performance"} and key_name == "lazy_load":
                    emit(key_name, value)
                else:
                    emit(f"{section_name}_{key_name}", value)

    if emit_output and lines:
        sys.stdout.write("\n".join(lines) + "\n")

    return lines

def _default_root() -> Path:
    env_root = os.environ.get("SENTINEL_ROOT", "")
    if env_root:
        p = Path(env_root).expanduser()
        if p.exists():
            return p
    # /installer/config.py -> project root is parent of installer/
    return Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--config", default=None)
    parser.add_argument("--output", default=None)
    args, _unknown = parser.parse_known_args()

    root = _default_root()
    config_path = Path(args.config).expanduser() if args.config else (root / "config.yaml")
    dist_path = root / "config.yaml.dist"

    if config_path.exists():
        config = parse_yaml(config_path)
    elif dist_path.exists():
        config = parse_yaml(dist_path)
    else:
        print("echo 'Configuration file not found.' >&2")
        return 1

    lines = export_variables(config, emit_output=False)
    content = "\n".join(lines) + ("\n" if lines else "")

    if args.output:
        out_path = Path(args.output).expanduser()
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(content, encoding="utf-8")
        return 0

    sys.stdout.write(content)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
