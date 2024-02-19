ARG IMAGE=ubuntu:22.04
FROM $IMAGE as builder
WORKDIR /rtsp2ws	
COPY . .

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates g++ autoconf automake libtool xz-utils cmake make pkg-config git libjpeg-dev libssl-dev \
    && cmake . && make install && apt-get clean && rm -rf /var/lib/apt/lists/

FROM $IMAGE
LABEL maintainer michel.promonet@free.fr
COPY --from=builder /usr/local/bin/rtsp2ws /usr/local/bin/
COPY --from=builder /usr/local/share/rtsp2ws/ /usr/local/share/rtsp2ws/

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends ca-certificates libjpeg-dev libssl-dev libasound2-dev && rm -rf /var/lib/apt/lists/

ENTRYPOINT [ "/usr/local/bin/rtsp2ws" ]
CMD [ "-p", "/usr/local/share/rtsp2ws" ]
