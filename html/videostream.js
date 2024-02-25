/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

class VideoStream {
    constructor(videoElement) {
        this.videoElement = videoElement;
        this.frameResolved = null;
        this.metadata = {codec: '', ts: 0}
        this.reconnectTimer = null;
        this.ws = null;

        const videoCanvas = document.createElement("canvas");
        this.videoContext = videoCanvas.getContext("2d");        
        this.videoElement.srcObject = videoCanvas.captureStream();
        this.videoElement.play();

        this.decoder = this.createDecoder();
        this.clearFrame();
    }

    clearFrame() {
        this.videoContext.clearRect(0, 0, this.videoContext.canvas.width, this.videoContext.canvas.height);
    }

    displayFrame(frame) {
        this.videoContext.canvas.width = frame.displayWidth;
        this.videoContext.canvas.height = frame.displayHeight;
        this.videoContext.drawImage(frame, 0, 0);
        frame.close();
    }

    createDecoder() {
        return   this.decoder = new VideoDecoder({
                output: (frame) => this.displayFrame(frame),
                error: (e) => console.log(e.message),
        });
    }

    async decodeFrame(data, type) {
        if (this.decoder.state === "configured") {
            const chunk = new EncodedVideoChunk({
                timestamp: this.metadata.ts,
                type,
                data,
            });
            this.decoder.decode(chunk);
            return new Promise(r => this.frameResolved = r);
        } else {
            return Promise.reject(`${this.metadata.codec} decoder not configured`);
        }
    }

    async onH264Frame(data) {
        const naluType = data[4] & 0x1F;
        if (this.decoder.state !== "configured" && naluType === 7) {
            let codec = 'avc1.';
            for (let i = 0; i < 3; i++) {
                codec += ('00' + data[5 + i].toString(16)).slice(-2);
            }

            const config = { codec };
            const support = await VideoDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`H264 decoder supported with codec ${codec}`);
                this.decoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        const type = (naluType === 7) || (naluType === 5) ? "key" : "delta";
        return this.decodeFrame(data, type);
    }

    async onH265Frame(data) {
        const naluType = (data[4] & 0x7E) >> 1;
        if (this.decoder.state !== "configured" && naluType === 32) {
            let codec = 'hev1.';
            for (let i = 0; i < 3; i++) {
                codec += ('00' + data[5 + i].toString(16)).slice(-2);
            }

            const config = { codec };
            const support = await VideoDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`H265 decoder supported with codec ${codec}`);
                this.decoder.configure(config);


            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        const type = (naluType === 32) || (naluType === 19) || (naluType === 20) ? "key" : "delta";
        return this.decodeFrame(data, type);
    }

    onFrame(data) {
        if (this.metadata.codec === 'H264') {
            return this.onH264Frame(data);
        } else if (this.metadata.codec === 'H265') {
            return this.onH265Frame(data);
        } else {
            return Promise.reject(`Unknown format`);
        }
    }

    async onMessage(message) {
        const { data } = message;
        if (data instanceof ArrayBuffer) {
            const bytes = new Uint8Array(data);
            try {
                const frame = await this.onFrame(bytes);
                this.displayFrame(frame);
                this.videoElement.title = '';
            } catch (e) {
                console.warn(e);
                this.videoElement.title = e;
            }
        } else if (typeof data === 'string') {
            this.metadata = JSON.parse(data);
        }
    }

    connectWebSocket(stream) {
        let wsurl = new URL(stream, location.href);
        wsurl.protocol = wsurl.protocol.replace("http", "ws");
        this.closeWebSocket();
        console.log(`Connecting WebSocket to ${wsurl}`);
        this.ws = new WebSocket(wsurl.href);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onmessage = (message) => this.onMessage(message);
        this.ws.onclose = () => this.reconnectTimer = setTimeout(() => this.connectWebSocket(wsurl), 1000);
    }

    closeWebSocket() {
        if (this.reconnectTimer) {
            clearTimeout(this.reconnectTimer);
            this.reconnectTimer = null;
        }
        if (this.ws  && this.ws.readyState === WebSocket.OPEN) {
            this.ws.onclose = () => {};
            this.ws.close();
        }
        this.ws = null;
        this.decoder = this.createDecoder();
        this.clearFrame();
    }
}
