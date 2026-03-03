FROM nodered/node-red:latest

USER root

# Install node-red-dashboard into Node-RED's own module directory,
# not /data, so it is always available regardless of volume contents.
RUN npm install --prefix /usr/src/node-red \
        --no-fund --no-update-notifier \
        node-red-dashboard

# Store default flows at a stable non-/data path inside the image.
COPY flows.json /usr/src/node-red/default_flows.json

# Entrypoint seeds /data/flows.json on first run, then starts Node-RED.
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

USER node-red
ENTRYPOINT ["/entrypoint.sh"]
