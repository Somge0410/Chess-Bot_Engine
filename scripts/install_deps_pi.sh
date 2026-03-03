#!/usr/bin/env bash
set -euo pipefail

echo "=== Installing dependencies for Chess-Bot_Engine (Raspberry Pi) ==="
sudo apt update
sudo apt install -y git build-essential cmake ninja-build

echo "=== Done. All dependencies installed. ==="