#!/usr/bin/env bash
set -e

LOGFILE="/var/log/zig-install.log"
exec > >(tee -a "$LOGFILE") 2>&1

echo "⚡ Installing Zig 0.15.2 for Linux (x86_64)..."

ZIG_VERSION="0.15.2"
ZIG_URL="https://ziglang.org/download/${ZIG_VERSION}/zig-x86_64-linux-${ZIG_VERSION}.tar.xz"
ZIG_DIR="/usr/local/zig"
ZIG_BIN="/usr/local/bin/zig"

echo "⬇️ Downloading Zig from $ZIG_URL ..."
cd /tmp
curl -L "$ZIG_URL" -o zig.tar.xz

echo "📦 Extracting Zig..."
sudo mkdir -p "$ZIG_DIR"
sudo tar -xJf zig.tar.xz -C "$ZIG_DIR" --strip-components=1

echo "🔗 Creating symlink..."
sudo ln -sf "$ZIG_DIR/zig" "$ZIG_BIN"

echo "🧹 Cleaning up..."
rm -f zig.tar.xz

echo "🧩 Verifying installation..."
zig version || { echo "❌ Zig binary not found!"; exit 1; }

echo "✅ Zig ${ZIG_VERSION} installed successfully at $(date)" | sudo tee /var/log/zig-install-success.log
