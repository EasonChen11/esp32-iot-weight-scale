FROM nodered/node-red:latest

# Install dashboard plugin into Node-RED's own module directory.
# This avoids conflicts with any node_modules that may exist in the
# mounted /data volume.
USER root
RUN npm install --prefix /usr/src/node-red \
        --no-fund --no-update-notifier \
        node-red-dashboard

# Bake the pre-configured flows into the image.
# On first start, Node-RED copies /usr/src/node-red/flows.json to /data/flows.json
# if /data/flows.json does not yet exist.
COPY flows.json /usr/src/node-red/flows.json

USER node-red
