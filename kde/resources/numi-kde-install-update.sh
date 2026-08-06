#!/bin/bash
# Privileged helper for numi-kde self-update — invoked exclusively via pkexec.
# Usage: numi-kde-install-update <rpm-path>
set -e
exec dnf install -y --nogpgcheck "$1"
