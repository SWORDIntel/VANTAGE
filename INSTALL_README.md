# SENTINEL Installation Notes

This file is a compatibility landing page. The maintained installation guide lives at [`docs/installation.md`](docs/installation.md).

## Quick Start

```bash
git clone https://github.com/SWORDIntel/SENTINEL.git
cd SENTINEL
bash install.sh
source ~/.bashrc
```

## Other Supported Paths

- Kitty-first install: `bash install_kitty.sh`
- Unattended install: `bash install.sh --non-interactive --headless`
- Reinstall: `bash reinstall.sh`
- Uninstall: `bash uninstall.sh`

## Validation

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

Optional Markov dependencies:

```bash
python3 -m pip install -r requirements-markov.txt
```
