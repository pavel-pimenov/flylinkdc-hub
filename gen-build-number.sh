#!/bin/bash
# Generate build_number.txt from git before Docker build
BUILD_NUM=$(git rev-list --count HEAD 2>/dev/null || echo "0")
echo "$BUILD_NUM" > build_number.txt
echo "Build number: $BUILD_NUM"
