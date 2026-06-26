.PHONY: all core indexer ebpf clean bench help

help:
	@echo "VANTAGE End-to-End Build System"
	@echo "  make all      - Build Core, Indexer, and eBPF Firewall"
	@echo "  make core     - Build VANTAGE pure-Rust Core (with HYBRID_DB integration)"
	@echo "  make indexer  - Build VANTAGE Idle Scanner Daemon"
	@echo "  make ebpf     - Build VANTAGE XDP Firewall"
	@echo "  make bench    - Run end-to-end VANTAGE benchmarks"
	@echo "  make clean    - Clean all build artifacts"

all: core indexer ebpf

core:
	cd core && cargo build --release

indexer:
	cd sentinel_indexer && make

ebpf:
	cd ebpf && clang -O2 -target bpf -I/usr/include/x86_64-linux-gnu -c vantage_xdp.c -o vantage_xdp.o

clean:
	cd core && cargo clean
	cd sentinel_indexer && make clean
	rm -f ebpf/vantage_xdp.o

bench:
	@echo "Running End-to-End VANTAGE Benchmarks..."
	@echo "----------------------------------------"
	@echo "[BENCH] VANTAGE Core DB Integration (QIHSE + KEYSTONE):"
	cd core && time ./target/release/vantage-core --db-test
	@echo "----------------------------------------"
	@echo "[BENCH] Indexer Daemon footprint:"
	@ls -lh sentinel_indexer/indexer_daemon
	@echo "----------------------------------------"
	@echo "[BENCH] eBPF Firewall payload size:"
	@ls -lh ebpf/vantage_xdp.o
	@echo "----------------------------------------"
	@echo "VANTAGE End-to-End Audit COMPLETE."
