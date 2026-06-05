#!/usr/bin/env bats

@test "Installer runs without fatal errors" {
    set -x
    run bash installer/install.sh --non-interactive --dry-run
  [ "$status" -eq 0 ] || (echo "Installer failed with status $status" && echo "Output: $output" && false)
}
