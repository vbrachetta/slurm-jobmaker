#!/bin/bash
# =============================================================================
#  Slurm JobMaker — Fedora Container Binary Test
#  ---------------------------------------------
#  Purpose:
#    Run the SlurmJobMaker binary inside a Fedora container to verify
#    cross-distribution compatibility from a Debian host.
#
#  Requirements:
#    - Podman 5.4.2 or later
#    - Wayland with XWayland support, or a running X11 display server
#    - Compiled SlurmJobMaker binary present in this directory
#
#  Usage:
#    cd packaging/testing
#    ./run-fedora-gui.sh
#
#  Notes:
#    - Qt6 runtime dependencies are installed inside the container.
#    - The container is automatically removed after the session ends.
#    - Intended for development and testing only.
#
#  Author: Vincenzo Brachetta
#  Date:   2026-04-11
#  Developed and tested on Debian GNU/Linux 13.4 (Trixie)
# =============================================================================


set -e

# Temporarily allow local GUI clients (e.g. container) to access the X display
xhost +local:

# Revoke relaxed X11 access control when the script terminates (normal or error)
trap 'xhost -local:' EXIT

podman run -it --rm --rmi \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "$PWD":/app \
  -w /app \
  fedora:43 \
  bash -c "dnf install -y --setopt=fastestmirror=True qt6-qtbase qt6-qtbase-gui &&./SlurmJobMaker"
