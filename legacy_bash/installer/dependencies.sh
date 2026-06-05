#!/usr/bin/env bash
# SENTINEL Installer - Dependency Functions

# Python version check using Python itself (more reliable)
check_python_version() {
    step "Checking Python version"
    if ! command -v python3 &>/dev/null; then
        fail "Python 3 is not installed"
    fi

    if ! python3 -c "import sys; sys.exit(0 if sys.version_info >= (3,6) else 1)"; then
        local py_version
        py_version=$(python3 --version 2>&1 | cut -d' ' -f2)
        fail "Python 3.6+ is required (found ${py_version})"
    fi

    local py_version
    py_version=$(python3 --version 2>&1 | cut -d' ' -f2)
    ok "Python ${py_version} meets requirements"
}

# Find Python executable (prioritize datascience environment)
find_python() {
    # First check if we're already in the datascience environment
    if [[ -n "${VIRTUAL_ENV:-}" ]] && [[ "$VIRTUAL_ENV" == *"datascience"* ]]; then
        if command -v python &>/dev/null; then
            echo "python"
            return 0
        fi
    fi

    # Check for custom compiled Python in datascience directory
    local datascience_pythons=(
        "${HOME}/datascience/envs/dsenv/bin/python"
        "${HOME}/datascience/envs/dsenv/bin/python3"
        "${HOME}/datascience/bin/python"
        "${HOME}/datascience/bin/python3"
    )

    for python_cmd in "${datascience_pythons[@]}"; do
        if [[ -x "$python_cmd" ]] && "$python_cmd" -c "import sys; sys.exit(0 if sys.version_info >= (3,6) else 1)" 2>/dev/null; then
            echo "$python_cmd"
            return 0
        fi
    done

    # Then check for the latest system Python versions
    for python_cmd in python3.12 python3.11 python3.10 python3.9 python3.8 python3.7 python3.6 python3; do
        if command -v "$python_cmd" &>/dev/null; then
            if "$python_cmd" -c "import sys; sys.exit(0 if sys.version_info >= (3,6) else 1)"; then
                echo "$python_cmd"
                return 0
            fi
        fi
    done
    return 1
}

parse_version() {
    # shellcheck disable=SC2001
    echo "${1//./ }"
}

check_version() {
    local cmd_version_str
    cmd_version_str=$($1 --version | head -n1 | grep -oE '[0-9]+(\.[0-9]+)+')
    local required_version_str
    required_version_str=$2

    local cmd_version
    cmd_version=$(parse_version "$cmd_version_str")
    local required_version
    required_version=$(parse_version "$required_version_str")

    local i=0
    for n in $cmd_version; do
        i=$((i+1))
        local req_n
        req_n=$(echo "$required_version" | cut -d' ' -f$i)
        if [[ "$n" -gt "$req_n" ]]; then
            return 0
        fi
        if [[ "$n" -lt "$req_n" ]]; then
            return 1
        fi
    done
    return 0

}

check_dependency() {
    local cmd=$1
    local version=$2
    local url=$3

    if ! command -v "${cmd}" &>/dev/null; then
        fail "Missing system package: ${cmd}. Please install it from ${url} and re-run."
    fi

    if [[ -n "$version" ]]; then
        if ! check_version "${cmd}" "${version}"; then
            local cmd_version
            cmd_version=$($cmd --version | head -n1)
            fail "Unsupported ${cmd} version: ${cmd_version}. Please upgrade to version ${version} or later. You can download it from ${url}"
        fi
    fi
}

check_dependencies() {
    check_dependency "git" "2.7" "https://git-scm.com/"
    check_dependency "make" "3.81" "https://www.gnu.org/software/make/"
    check_dependency "awk" "" ""
    check_dependency "sed" "" ""
    check_dependency "rsync" "3.1" "https://rsync.samba.org/"
    check_dependency "pip3" "9.0" "https://pip.pypa.io/en/stable/installing/"
    ok "All required CLI tools present"
}

# Platform-aware package dependency checking
detect_package_manager() {
    for mgr in apt-get dnf yum pacman zypper apk brew; do
        if command -v "$mgr" &>/dev/null; then
            echo "$mgr"
            return 0
        fi
    done
    return 1
}

attempt_package_install() {
    local manager="$1"; shift
    local packages=("$@")
    [[ ${#packages[@]} -eq 0 ]] && return 1

    case "$manager" in
        apt-get)
            sudo apt-get update && sudo apt-get install -y "${packages[@]}" ;;
        dnf)
            sudo dnf install -y "${packages[@]}" ;;
        yum)
            sudo yum install -y "${packages[@]}" ;;
        pacman)
            sudo pacman -S --noconfirm "${packages[@]}" ;;
        zypper)
            sudo zypper install -y "${packages[@]}" ;;
        apk)
            sudo apk add "${packages[@]}" ;;
        brew)
            brew install "${packages[@]}" ;;
        *)
            return 1 ;;
    esac
}

ensure_venv_support() {
    local manager="$1"
    local python_cmd="${2:-python3}"
    if "$python_cmd" -c "import venv" &>/dev/null; then
        ok "Python venv module available"
        return 0
    fi

    warn "Python venv module missing; needed for isolated installs"
    local candidates=()
    case "$manager" in
        apt-get) candidates=(python3-venv) ;;
        dnf|yum) candidates=(python3-virtualenv python3.12-venv python3-venv) ;;
        pacman)  candidates=(python-virtualenv) ;;
        zypper)  candidates=(python3-virtualenv python3-venv) ;;
        apk)     candidates=(py3-virtualenv) ;;
        brew)    candidates=(python) ;;
    esac

    if [[ -n "$manager" && ${#candidates[@]} -gt 0 ]]; then
        echo "Install command suggestion: sudo ${manager} install ${candidates[*]}"
        if [[ $INTERACTIVE -eq 1 ]]; then
            read -r -t 30 -p "Install missing Python venv support now? [y/N]: " confirm || confirm="n"
            if [[ "$confirm" =~ ^[Yy]([Ee][Ss])?$ ]]; then
                if attempt_package_install "$manager" "${candidates[@]}"; then
                    ok "Installed venv support via ${manager}"
                else
                    fail "Failed to install venv support via ${manager}"
                fi
            fi
        fi
    fi

    if ! "$python_cmd" -c "import venv" &>/dev/null; then
        fail "Python venv module still unavailable; install the appropriate package and re-run the installer"
    fi
}

check_platform_dependencies() {
    local manager
    manager=$(detect_package_manager || true)

    if [[ -n "$manager" ]]; then
        step "Detected package manager: $manager"
    else
        warn "No supported package manager detected; continuing with manual checks"
    fi

    ensure_venv_support "$manager" "${PYTHON_CMD:-python3}"

    # Auto-install fzf only in interactive mode (avoid sudo prompts in CI/tests)
    if ! command -v fzf &>/dev/null && [[ -n "$manager" ]]; then
        if [[ "${INTERACTIVE:-1}" -eq 1 ]]; then
            step "fzf not found; attempting install via $manager"
            if attempt_package_install "$manager" "fzf"; then
                ok "Installed fzf via $manager"
            else
                warn "Failed to install fzf automatically; install manually with: sudo ${manager} install fzf"
            fi
        else
            warn "fzf not found; skipping auto-install in non-interactive mode"
        fi
    fi

    local optional_pkgs=()
    command -v openssl &>/dev/null || optional_pkgs+=("openssl")
    command -v fzf &>/dev/null || optional_pkgs+=("fzf")

    if ((${#optional_pkgs[@]})); then
        warn "Optional packages not found: ${optional_pkgs[*]}"
        if [[ -n "$manager" ]]; then
            echo "These improve functionality. Install with: sudo ${manager} install ${optional_pkgs[*]}"
        else
            echo "Install the optional packages with your platform package manager for best experience."
        fi
    else
        ok "Optional helpers already present"
    fi
}
