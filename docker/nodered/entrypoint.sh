#!/bin/sh
# Seed /data/flows.json with the baked-in defaults on first run.
# If the file already exists (user edits, container restart) it is kept as-is.
if [ ! -f /data/flows.json ]; then
    echo "[init] No flows.json found — installing default weight-scale flows."
    cp /usr/src/node-red/default_flows.json /data/flows.json
fi
exec node-red --userDir /data "$@"
