#!/bin/bash
# ------------------------------------------------
# SENTINEL TEST SCRIPT: Markov Text Generator
# ------------------------------------------------
# Tests the functionality of the Markov text generator
# components and integration with SENTINEL.
#
# Usage: ./test_markov_generator.sh
# ------------------------------------------------

# Exit on error
set -e

# Include test utilities if available
if [[ -f "$(dirname "$0")/test_utils.sh" ]]; then
    source "$(dirname "$0")/test_utils.sh"
fi

# Set up colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test directories
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="${ROOT_DIR}/tests/markov_test"
SCRIPT_PATH="${ROOT_DIR}/markov_generator.py"
MODULE_PATH="${ROOT_DIR}/bash_modules.d/sentinel_markov.module"

# Create test directory if it doesn't exist
mkdir -p "${TEST_DIR}"

# Log function
log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Test function
test_case() {
    local name="$1"
    local cmd="$2"
    
    log "${BLUE}Running test: ${name}${NC}"
    echo "Command: $cmd"
    
    if eval "$cmd"; then
        log "${GREEN}✓ Test passed: ${name}${NC}"
        return 0
    else
        log "${RED}✗ Test failed: ${name}${NC}"
        return 1
    fi
}

# Create test input file
create_test_input() {
    cat > "${TEST_DIR}/input.txt" << EOF
SENTINEL is a comprehensive, modular, security-focused bash environment enhancement system for cybersecurity professionals.
It provides AI-powered conversational assistant, modular autocomplete, and command prediction.
The framework supports secure snippet management and context-aware suggestions.
All modules are designed for robust error handling, security, and privacy.
SENTINEL focuses on terminal-based workflows and Linux-first compatibility.
The modular architecture includes components for logging, security, and machine learning.
Users can enable or disable features based on their specific needs and performance requirements.
SENTINEL implements performance optimizations like configuration caching and dependency-based module loading.
The installation process is designed to be secure and customizable for different environments.
Advanced features include distributed compilation and secure file operations for power users.
EOF

    log "${GREEN}Created test input file${NC}"
}

# Check if Python and required packages are available
check_python_deps() {
    if ! command -v python3 &>/dev/null; then
        log "${RED}Python 3 not found. Skipping tests.${NC}"
        exit 1
    fi
    
    if ! python3 -c "import markovify, numpy" &>/dev/null; then
        log "${YELLOW}Required Python packages not found. Some tests may fail.${NC}"
        log "Run: pip install markovify numpy tqdm unidecode"
    else
        log "${GREEN}Python dependencies found${NC}"
    fi
}

# Main test runner
run_tests() {
    local passed=0
    local failed=0
    local total=0
    
    # Check script exists
    if [[ ! -f "${SCRIPT_PATH}" ]]; then
        log "${RED}Error: Markov generator script not found at ${SCRIPT_PATH}${NC}"
        exit 1
    fi
    
    # Make script executable
    chmod +x "${SCRIPT_PATH}"
    
    # Test 1: Basic text generation
    if test_case "Basic text generation" "python3 '${SCRIPT_PATH}' --input '${TEST_DIR}/input.txt' --output '${TEST_DIR}/output1.txt' --count 3"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 2: Check if output file was created
    if test_case "Output file verification" "test -f '${TEST_DIR}/output1.txt' && test -s '${TEST_DIR}/output1.txt'"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 3: Pipe input through stdin
    if test_case "Stdin input" "cat '${TEST_DIR}/input.txt' | python3 '${SCRIPT_PATH}' --stdin --output '${TEST_DIR}/output2.txt'"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 4: Different state sizes
    if test_case "Larger state size" "python3 '${SCRIPT_PATH}' --input '${TEST_DIR}/input.txt' --output '${TEST_DIR}/output3.txt' --state-size 3"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 5: Direct text input
    if test_case "Direct text input" "python3 '${SCRIPT_PATH}' --text 'This is a test sentence. Another test sentence.' --output '${TEST_DIR}/output4.txt'"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 6: Create corpus directory
    mkdir -p "${TEST_DIR}/corpus"
    cp "${TEST_DIR}/input.txt" "${TEST_DIR}/corpus/input1.txt"
    echo "Additional test data for corpus testing." > "${TEST_DIR}/corpus/input2.txt"
    
    if test_case "Corpus directory" "python3 '${SCRIPT_PATH}' --corpus-dir '${TEST_DIR}/corpus' --output '${TEST_DIR}/output5.txt'"; then
        ((++passed))
    else
        ((++failed))
    fi
    ((++total))
    
    # Test 7: Check module file exists
    if [[ -f "${MODULE_PATH}" ]]; then
        log "${GREEN}Module file exists${NC}"
        ((++passed))
    else
        log "${RED}Module file not found at ${MODULE_PATH}${NC}"
        ((++failed))
    fi
    ((++total))
    
    # Test results
    echo ""
    log "${BLUE}Test Results:${NC}"
    log "${GREEN}Tests passed: ${passed}/${total}${NC}"
    
    if ((failed > 0)); then
        log "${RED}Tests failed: ${failed}/${total}${NC}"
        exit 1
    else
        log "${GREEN}All tests passed!${NC}"
    fi
    
    # Cleanup
    if [[ "$1" == "--clean" ]]; then
        log "${YELLOW}Cleaning up test files...${NC}"
        rm -rf "${TEST_DIR}"
    fi
}

# Main execution
log "${BLUE}Starting Markov Text Generator tests${NC}"
check_python_deps
create_test_input
run_tests "$@" 