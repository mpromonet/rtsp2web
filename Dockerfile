ARG IMAGE=debian:trixie
FROM $IMAGE AS builder
LABEL maintainer="michel.promonet@free.fr"
ARG USERNAME=dev
ARG USERID=10000

WORKDIR /rtsp2ws	
COPY . .

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates g++ cmake make pkg-config git libssl-dev sudo \
    && groupadd --gid $USERID $USERNAME && useradd --uid $USERID --gid $USERNAME -m -s /bin/bash $USERNAME \
	&& echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
	&& chmod 0440 /etc/sudoers.d/$USERNAME \
    && cmake . && make install && apt-get clean && rm -rf /var/lib/apt/lists/

USER $USERNAME

FROM $IMAGE
LABEL maintainer="michel.promonet@free.fr"
LABEL org.opencontainers.image.description="rtsp to websocket gateway"

COPY --from=builder /usr/local/bin/rtsp2web /usr/local/bin/
COPY --from=builder /usr/local/share/rtsp2web/ /usr/local/share/rtsp2web/

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates libssl-dev \
    && rm -rf /var/lib/apt/lists/ \
    && useradd -m user

WORKDIR /usr/local/share/rtsp2web
USER user

ENTRYPOINT [ "/usr/local/bin/rtsp2web"]
CMD ["-C", "config.json", "-c", "keycert.pem"]
