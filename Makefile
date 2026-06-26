.PHONY: all core indexer ebpf clean bench help submodule qihse qihse-clean

help:
	@echo "VANTAGE End-to-End Build System"
	@echo "  make all         - Build Core, Indexer, and eBPF Firewall"
	@echo "  make core        - Build QIHSE (silicon-optimized) + VANTAGE Core"
	@echo "  make qihse       - Build QIHSE libqihse.so for current silicon"
	@echo "  make indexer     - Build VANTAGE Idle Scanner Daemon"
	@echo "  make ebpf        - Build VANTAGE XDP Firewall"
	@echo "  make bench       - Run end-to-end VANTAGE benchmarks"
	@echo "  make clean       - Clean all build artifacts"
	@echo "  make qihse-clean - Clean QIHSE build artifacts only"
	@echo "  make submodule   - Initialize and update git submodules (QIHSE)"

all: core indexer ebpf

submodule:
	git submodule update --init --recursive

qihse:
	cd vendor/QIHSE && ./scripts/build-native.sh --auto --verbose

qihse-clean:
	cd vendor/QIHSE && make clean

core: qihse
	cd core && cargo build --release

indexer:
	cd vantage_indexer && make

ebpf:
	cd ebpf && clang -O2 -target bpf -I/usr/include/x86_64-linux-gnu -c vantage_xdp.c -o vantage_xdp.o

clean:
	cd core && cargo clean
	cd vantage_indexer && make clean
	rm -f ebpf/vantage_xdp.o
	cd vendor/QIHSE && make clean

bench:
	@echo "Running End-to-End VANTAGE Benchmarks..."
	@echo "----------------------------------------"
	@echo "[BENCH] VANTAGE Core DB Integration (QIHSE + KEYSTONE):"
	cd core && time ./target/release/vantage-core --db-test
	@echo "----------------------------------------"
	@echo "[BENCH] QIHSE Silicon-Optimized Search:"
	cd core && time ./target/release/vantage-core --qihse-test
	@echo "----------------------------------------"
	@echo "[BENCH] Indexer Daemon footprint:"
	@ls -lh vantage_indexer/indexer_daemon
	@echo "----------------------------------------"
	@echo "[BENCH] eBPF Firewall payload size:"
	@ls -lh ebpf/vantage_xdp.o
	@echo "----------------------------------------"
	@echo "VANTAGE End-to-End Audit COMPLETE."
