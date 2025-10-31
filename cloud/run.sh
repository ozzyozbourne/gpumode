#!/usr/bin/env bash
set -e

LOGFILE="/var/log/setup.log"
exec > >(tee -a "$LOGFILE") 2>&1

echo "🔄 Updating system packages..."
sudo dnf update -y --allowerasing || echo "⚠️ Some packages skipped during update"

echo "🧱 Installing base developer tools..."
sudo dnf groupinstall -y "Development Tools" --skip-broken || true
sudo dnf install -y --allowerasing --skip-broken \
  gcc-c++ cmake git curl wget unzip python3 python3-pip kernel-tools || true

# kernel headers often fail — retry with protection
sudo dnf install -y --allowerasing --skip-broken \
  kernel-devel-$(uname -r) kernel-headers-$(uname -r) || echo "⚠️ Kernel headers skipped"

echo "⚙️ Installing performance analysis tools..."
sudo dnf install -y --allowerasing --skip-broken perf || true

echo "📊 Installing additional monitoring and profiling tools..."
sudo dnf install -y --allowerasing --skip-broken \
  sysstat htop iotop iftop iperf3 strace ltrace gdb valgrind numactl hwloc || true

echo "🧠 Optional: eBPF and tracing tools..."
sudo dnf install -y --allowerasing --skip-broken bcc-tools bpftrace || true

echo "🧩 Verifying installations..."
if command -v perf >/dev/null 2>&1; then
    perf --version
else
    echo "❌ perf not found — try manually installing: sudo dnf install -y kernel-tools"
fi

gcc --version || true
g++ --version || true

echo "✅ EC2 setup completed successfully at $(date)" | sudo tee /var/log/setup-success.log
