#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
MODE="${1:-broad}"
OPTIONAL_FLAG="${2:-}"

if [[ "$MODE" != "fast" && "$MODE" != "broad" && "$MODE" != "optional" ]]; then
    echo "Usage: $0 [fast|broad|optional] [--include-optional]" >&2
    exit 2
fi

if [[ -n "$OPTIONAL_FLAG" && "$OPTIONAL_FLAG" != "--include-optional" ]]; then
    echo "Unknown option: $OPTIONAL_FLAG" >&2
    exit 2
fi

unset BASH_ENV ENV

RUN_HOME="$(mktemp -d "${TMPDIR:-/tmp}/sentinel-test-home.XXXXXX")"
cleanup() {
    rm -rf "$RUN_HOME"
}
trap cleanup EXIT

export HOME="$RUN_HOME"
export XDG_CONFIG_HOME="$HOME/.config"
export SENTINEL_ROOT="$ROOT_DIR"
export SENTINEL_SKIP_PYTHON_VENV="${SENTINEL_SKIP_PYTHON_VENV:-1}"
export PYTHONDONTWRITEBYTECODE=1
export LANG="${LANG:-C.UTF-8}"
export LC_ALL="${LC_ALL:-C.UTF-8}"

mkdir -p "$XDG_CONFIG_HOME"

TOTAL_STEPS=0
PASSED_STEPS=0

log() {
    printf '[test-runner] %s\n' "$*"
}

run_step() {
    local label="$1"
    shift

    TOTAL_STEPS=$((TOTAL_STEPS + 1))
    log "START  $label"

    (
        cd "$ROOT_DIR"
        "$@"
    )

    PASSED_STEPS=$((PASSED_STEPS + 1))
    log "PASS   $label"
}

have_markov_deps() {
    (
        cd "$ROOT_DIR"
        python3 - <<'PY'
import importlib.util
import sys

required = ("markovify", "numpy", "tqdm", "unidecode")
missing = [name for name in required if importlib.util.find_spec(name) is None]
if missing:
    print("missing optional deps:", ", ".join(missing))
    sys.exit(1)
PY
    )
}

run_optional_suite() {
    if ! have_markov_deps; then
        log "SKIP   Markov generator tests require optional deps: markovify numpy tqdm unidecode"
        return 0
    fi

    run_step "Markov generator" bash tests/test_markov_generator.sh
}

run_fast_suite() {
    run_step "Config unit tests" python3 -m unittest -v tests/test_config.py
    run_step "Python integration" bash test_python_integration.sh
    run_step "Module loading" bash test_module_loading.sh
    run_step "Clean module loading" bash test_clean_module_loading.sh
    run_step "Error recovery" bash tests/test_error_recovery.sh
    run_step "New features" bash tests/test_new_features.sh
    run_step "Parallel loading" bash tests/test_parallel_loading.sh
    run_step "Health check" bash tests/test_health_check.sh
}

run_broad_suite() {
    run_fast_suite
    run_step "Fabric and kitty installer integration" bash tests/test_fabric_kitty_integration.sh
    run_step "Lazy loading" bash tests/test_lazy_loading.sh
    run_step "External tools" bash tests/test_external_tools.sh
    run_step "Return code helper" bash tests/ret_test.sh 0
    run_step "Bash syntax check" bash tests/bash_check.sh bash_modules
    run_step "Contrib Python integration" bash -lc 'source bash_modules.d/python_integration.module >/dev/null && python3 contrib/sentinel_integration_test.py'
}

log "root=$ROOT_DIR"
log "mode=$MODE"
log "home=$HOME"

case "$MODE" in
    fast)
        run_fast_suite
        ;;
    broad)
        run_broad_suite
        if [[ "$OPTIONAL_FLAG" == "--include-optional" ]]; then
            run_optional_suite
        else
            log "SKIP   Optional dependency-gated tests not requested"
            log "TIP    Run 'make test RUN_OPTIONAL=1' or 'make test-optional'"
        fi
        ;;
    optional)
        run_optional_suite
        ;;
esac

log "DONE   ${PASSED_STEPS}/${TOTAL_STEPS} steps passed"
