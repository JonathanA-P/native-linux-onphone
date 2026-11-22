#!/usr/bin/env bash
# scripts/verify_checksums.sh — Validate SHA256 checksums of all deliverables
set -e
ARTIFACTS_DIR="/home/jonathan/Documents/Projects/Apps/a70/native-linux/artifacts"
cd "$ARTIFACTS_DIR"
sha256sum -c SHA256SUMS
