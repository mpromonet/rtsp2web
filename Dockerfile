ARG IMAGE=ubuntu:24.04
FROM $IMAGE as builder
WORKDIR /rtsp2ws	
COPY . .

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates g++ cmake make pkg-config git libssl-dev \
    && cmake . && make install && apt-get clean && rm -rf /var/lib/apt/lists/

FROM $IMAGE
LABEL maintainer michel.promonet@free.fr
COPY --from=builder /usr/local/bin/rtsp2ws /usr/local/bin/
COPY --from=builder /usr/local/share/rtsp2ws/ /usr/local/share/rtsp2ws/

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates libssl-dev && rm -rf /var/lib/apt/lists/

ENTRYPOINT [ "/usr/local/bin/rtsp2ws", "-p", "/usr/local/share/rtsp2ws/www", "-C", "/usr/local/share/rtsp2ws/config.json"]
CMD []
