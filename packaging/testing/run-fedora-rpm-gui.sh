#!/bin/bash
# =============================================================================
#  Slurm JobMaker — Fedora Container RPM Test
#  ------------------------------------------
#  Purpose:
#    Install and run the SlurmJobMaker RPM package inside a Fedora
#    container to validate cross-distribution packaging and GUI execution
#    from a Debian host.
#
#  Requirements:
#    - Podman 5.4.2 or later
#    - Wayland with XWayland support, or a running X11 display server
#    - RPM file present in this directory as SlurmJobMaker.rpm
#
#  Usage:
#    cd packaging/testing
#    ./run-fedora-rpm-gui.sh
#
#  Notes:
#    - The RPM package and its dependencies are installed inside the container.
#    - The container is automatically removed after the session ends.
#    - Intended for development and testing only.
#
#  Author: Vincenzo Brachetta
#  Date:   2026-04-11
#  Developed and tested on Debian GNU/Linux 13.4 (Trixie)
# =============================================================================
set -e

RPM_FILE="slurm-jobmaker-1.0.0-1.x86_64.rpm"

if [ ! -f "$RPM_FILE" ]; then
    echo "ERROR: RPM file not found: $RPM_FILE"
    exit 1
fi

xhost +local:

trap 'xhost -local:' EXIT

podman run -it --rm --rmi \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "$PWD":/app \
  -w /app \
  fedora:43 \
  bash -c "
    dnf install -y /app/$RPM_FILE && \
    SlurmJobMaker
  "
