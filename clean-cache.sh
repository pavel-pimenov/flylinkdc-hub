#!/bin/bash
# Clean caches that grow large and waste disk space
# Auto-clean ipch when free disk space < 1 GB
set -euo pipefail

FREE_KB=$(df / --output=avail | tail -1 | tr -d ' ')
FREE_GB=$((FREE_KB / 1048576))

echo "=== Disk: ${FREE_GB} GB free ==="

IPCH_DIR="$HOME/.cache/vscode-cpptools/ipch"

if [ "$FREE_KB" -lt 1048576 ]; then
    echo "WARNING: less than 1 GB free on disk!"
    if [ -d "$IPCH_DIR" ]; then
        SIZE=$(du -sh "$IPCH_DIR" 2>/dev/null | cut -f1)
        echo "  Cleaning ipch cache ($SIZE)..."
        rm -rf "$IPCH_DIR"
        echo "  Deleted"
        FREE_KB=$(df / --output=avail | tail -1 | tr -d ' ')
        echo "  Now: $((FREE_KB / 1048576)) GB free"
    fi
    # Also prune Docker build cache if still low
    if [ "$FREE_KB" -lt 1048576 ]; then
        echo "  Still low, pruning Docker build cache..."
        docker builder prune -af 2>/dev/null && echo "  Docker cache pruned" || true
        FREE_KB=$(df / --output=avail | tail -1 | tr -d ' ')
        echo "  Now: $((FREE_KB / 1048576)) GB free"
    fi
else
    echo "Disk space OK (${FREE_GB} GB free)"
fi

df -h / | tail -1
