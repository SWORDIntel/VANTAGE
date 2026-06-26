#!/usr/bin/env python3
"""
mkvenv.py — Create or reuse a Python virtual environment in the current directory.

Default directory: .venv
Usage:
    $ python mkvenv.py
    $ source .venv/bin/activate
"""

import os
import subprocess
import venv


def create_or_use_venv(venv_dir=".venv"):
    if os.path.exists(venv_dir):
        print(f"[i] Reusing existing virtual environment at: {venv_dir}")
    else:
        print(f"[+] Creating virtual environment in: {venv_dir}")
        try:
            venv.create(venv_dir, with_pip=True)
            print("[✓] Virtual environment created.")
        except Exception as e:
            print(f"[✗] Failed to create virtual environment: {e}")
            return

    python_bin = os.path.join(venv_dir, "bin", "python")
    try:
        print("[*] Upgrading pip…")
        subprocess.check_call([python_bin, "-m", "pip", "install", "--upgrade", "pip"])
        print("[✓] pip upgraded successfully.")
    except subprocess.CalledProcessError as e:
        print(f"[!] pip upgrade failed: {e}")

    print(f"\n[→] Activate it using:\n  source {venv_dir}/bin/activate")


if __name__ == "__main__":
    create_or_use_venv()
