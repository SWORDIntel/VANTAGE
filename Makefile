.PHONY: help test test-fast test-optional

help:
	@echo "SENTINEL local test targets"
	@echo "  make test-fast              Run the fast local regression suite"
	@echo "  make test                   Run the broad local regression suite"
	@echo "  make test RUN_OPTIONAL=1    Run the broad suite plus optional dependency-gated tests"
	@echo "  make test-optional          Run only optional dependency-gated tests"

test-fast:
	@./scripts/test-all.sh fast

test:
	@./scripts/test-all.sh broad $(if $(RUN_OPTIONAL),--include-optional)

test-optional:
	@./scripts/test-all.sh optional
