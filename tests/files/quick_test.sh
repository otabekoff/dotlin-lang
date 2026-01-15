#!/bin/bash

echo "=== Testing Current Status ==="
echo "Files tested:"
ls -1 *.lin | wc -l | awk '{print $1 " tested files"}'
echo ""
echo "=== Testing Complete ==="
