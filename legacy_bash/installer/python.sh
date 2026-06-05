#!/usr/bin/env bash
# SENTINEL Installer - Python Functions

setup_python_venv() {
    if is_done "PYTHON_VENV_READY"; then
        if [[ -f "${HOME}/venv/bin/activate" ]]; then
            ok "Python venv already exists"
            return
        else
            warn "State file marked PYTHON_VENV_READY but venv missing, re-creating"
        fi
    fi
    step "Setting up Python virtual environment and dependencies"
    VENV_DIR="${HOME}/venv"

    # Ensure parent directory exists
    safe_mkdir "$(dirname "$VENV_DIR")"

    if [[ ! -d "$VENV_DIR" ]]; then
        # Check if python3-venv is available
        if ! "$PYTHON_CMD" -c "import venv" &>/dev/null; then
            fail "Python venv module not available. Please install python3-venv package"
        fi

        # Temporarily rename local venv.py to avoid conflict
        local local_venv_py_path="${PROJECT_ROOT}/venv.py"
        local temp_venv_py_path="${PROJECT_ROOT}/venv.py.tmp_rename"
        local renamed_local_venv_py=0
        if [[ -f "$local_venv_py_path" ]]; then
            mv "$local_venv_py_path" "$temp_venv_py_path"
            renamed_local_venv_py=1
            log "Temporarily renamed local venv.py to venv.py.tmp_rename"
        fi

        if ! "$PYTHON_CMD" -m venv "$VENV_DIR"; then
            # Restore local venv.py if it was renamed
            if [[ $renamed_local_venv_py -eq 1 ]]; then
                mv "$temp_venv_py_path" "$local_venv_py_path"
                log "Restored local venv.py"
            fi
            fail "Failed to create Python virtual environment"
        fi

        # Restore local venv.py if it was renamed
        if [[ $renamed_local_venv_py -eq 1 ]]; then
            mv "$temp_venv_py_path" "$local_venv_py_path"
            log "Restored local venv.py"
        fi

        if [[ ! -f "$VENV_DIR/bin/activate" ]]; then
            fail "Virtual environment creation failed - activate script not found"
        fi
        ok "Virtual environment created at $VENV_DIR"
    else
        ok "Virtual environment already exists at $VENV_DIR"
    fi
    # shellcheck source=/dev/null
    source "$VENV_DIR/bin/activate"
    step "Installing required Python packages in venv"
    "$VENV_DIR/bin/pip" install --upgrade pip

    # Install dependencies more efficiently
    local requirements_file="${PROJECT_ROOT}/requirements.txt"
    local failed_packages=()

    if [[ -f "$requirements_file" ]]; then
        log "Installing Python dependencies from $requirements_file"

        # First try to install all packages at once
        if ! "$VENV_DIR/bin/pip" install -r "$requirements_file" 2>/dev/null; then
            warn "Bulk installation failed, trying packages individually"

            # Read requirements file line by line, skipping comments and empty lines
            while IFS= read -r package || [[ -n "$package" ]]; do
                package=$(echo "$package" | sed 's/#.*//' | awk '{$1=$1};1') # Remove comments and trim whitespace
                if [[ -z "$package" ]]; then
                    continue
                fi

                # Check for OpenVINO constraint
                if [[ "$package" == openvino* ]]; then
                    local constraints_file="${PROJECT_ROOT}/constraints.txt"
                    if [[ -f "$constraints_file" ]] && grep -qxF "openvino==0.0.0" "$constraints_file"; then
                        log "Skipping OpenVINO installation due to constraint in $constraints_file"
                        continue
                    fi
                fi

                step "Attempting to install $package..."
                if "$VENV_DIR/bin/pip" install "$package"; then
                    ok "Successfully installed $package"
                else
                    warn "Failed to install $package"
                    failed_packages+=("$package")
                fi
            done < "$requirements_file"
        else
            ok "All packages installed successfully from requirements.txt"
        fi
    else
        log "$requirements_file not found. Falling back to hardcoded package list."
        local default_packages=("npyscreen" "tqdm" "requests" "beautifulsoup4" "numpy" "scipy" "scikit-learn" "joblib" "markovify" "unidecode" "rich")
        for package in "${default_packages[@]}"; do
            step "Attempting to install $package..."
            if "$VENV_DIR/bin/pip" install "$package"; then
                ok "Successfully installed $package"
            else
                warn "Failed to install $package"
                failed_packages+=("$package")
            fi
        done
    fi

    # Report failed packages if any
    if ((${#failed_packages[@]})); then
        warn "Failed to install the following packages: ${failed_packages[*]}"
        echo "You can try installing them manually later with: pip install ${failed_packages[*]}"
    fi

    if [[ "${SENTINEL_ENABLE_TENSORFLOW:-0}" == "1" ]]; then
        step "Attempting to install tensorflow..."
        if "$VENV_DIR/bin/pip" install tensorflow; then
            ok "Tensorflow installed (advanced ML features enabled)"
        else
            warn "Failed to install tensorflow. Skipping."
        fi
    fi
    mark_done "PYTHON_VENV_READY"
    ok "Python dependencies installed in venv"
}

prompt_custom_env() {
    if [[ $INTERACTIVE -eq 0 ]]; then
        log "Non-interactive mode: Skipping custom environment prompt."
        return
    fi

    step "Checking for user-defined custom Python environment"
    local custom_env_activate_script="${HOME}/datascience/envs/dsenv/bin/activate"

    # Check if already in the datascience environment
    if [[ -n "${VIRTUAL_ENV:-}" ]] && [[ "$VIRTUAL_ENV" == *"datascience"* ]]; then
        ok "Already using datascience environment: $VIRTUAL_ENV"
        export USING_DATASCIENCE_ENV=1
        return
    fi

    # Auto-detect if datascience environment exists
    if [[ -f "$custom_env_activate_script" ]]; then
        step "Found datascience environment at $custom_env_activate_script"
        read -r -t 30 -p "Use optimized datascience environment? This is recommended. [Y/n]: " confirm_custom_env || confirm_custom_env="y"

        if [[ ! "$confirm_custom_env" =~ ^[Nn]([Oo])?$ ]]; then
            step "Activating datascience environment"
            # shellcheck source=/dev/null
            if source "$custom_env_activate_script"; then
                ok "Successfully activated datascience environment"
                export USING_DATASCIENCE_ENV=1

                # Show environment info
                if command -v python &>/dev/null; then
                    local py_version=$(python --version 2>&1 | cut -d' ' -f2)
                    log "Using Python $py_version from datascience environment"
                fi
            else
                warn "Failed to activate datascience environment, continuing with system Python"
            fi
        else
            log "User declined to use datascience environment"
        fi
    else
        log "No datascience environment found at $custom_env_activate_script"
    fi
}
