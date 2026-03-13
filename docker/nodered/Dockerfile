FROM nodered/node-red:latest

USER root

# Install node-red-dashboard v2.x (v3+ has an incompatible theme format)
# Install into Node-RED's own module directory so it is always available
# regardless of the /data volume contents.
RUN npm install --prefix /usr/src/node-red \
        --no-fund --no-update-notifier \
        "node-red-dashboard@2"

USER node-red
