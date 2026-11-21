FROM qmkfm/base_container:latest

ARG VIAL_REVISION=dd43959ae5c08d8a28d38a1acf7b04e86b14a344

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc-arm-none-eabi libnewlib-arm-none-eabi python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN git init /opt/vial-qmk \
    && git -C /opt/vial-qmk fetch --depth 1 https://github.com/vial-kb/vial-qmk.git "$VIAL_REVISION" \
    && git -C /opt/vial-qmk checkout --detach FETCH_HEAD \
    && git -C /opt/vial-qmk remote add origin https://github.com/vial-kb/vial-qmk.git \
    && git -C /opt/vial-qmk submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/pico-sdk lib/printf lib/lufa \
    && python3 -m venv /opt/qmk \
    && /opt/qmk/bin/pip install --no-cache-dir qmk -r /opt/vial-qmk/requirements.txt \
    && chmod 1777 /opt/vial-qmk \
    && git config --system --add safe.directory /opt/vial-qmk

ENV PATH="/opt/qmk/bin:${PATH}"
ENV QMK_HOME=/opt/vial-qmk
WORKDIR /opt/vial-qmk
