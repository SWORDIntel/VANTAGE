# VANTAGE Installation Notes

This file is a compatibility landing page. The maintained installation guide lives at [`docs/installation.md`](docs/installation.md).

## Quick Start

```bash
git clone https://github.com/SWORDIntel/VANTAGE.git
cd VANTAGE
bash install.sh
source ~/.bashrc
```

## Other Supported Paths

- Kitty-first install: `bash install.sh kitty`
- Unattended install: `bash install.sh install`
- Reinstall: `bash install.sh reinstall`
- Uninstall: `bash install.sh uninstall`

## Validation

```bash
make test-fast
make test
make test RUN_OPTIONAL=1
```

Optional Markov dependencies:

```bash
python3 -m pip install -r requirements.txt
```
