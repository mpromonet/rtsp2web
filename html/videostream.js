/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

class VideoStream {
    constructor(videoElement) {
        this.videoElement = videoElement;
        const videoCanvas = document.createElement("canvas");
        this.videoContext = videoCanvas.getContext("2d");
        this.decoder = null;
        this.frameResolved = null;
        this.metadata = {codec: '', ts: 0}
        
        videoElement.srcObject = videoCanvas.captureStream();
        videoElement.play();
        this.videoContext.clearRect(0, 0, videoCanvas.width, videoCanvas.height);
    }

    displayFrame(frame) {
        this.videoContext.canvas.width = frame.displayWidth;
        this.videoContext.canvas.height = frame.displayHeight;
        this.videoContext.drawImage(frame, 0, 0);
        frame.close();
    }

    createDecoder() {
        if (!this.decoder) {
            this.decoder = new VideoDecoder({
                output: (frame) => this.displayFrame(frame),
                error: (e) => console.log(e.message),
            });
        }
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
        this.createDecoder();
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
        this.createDecoder();
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

    connectWebSocket(wsurl) {
        console.log(`Connecting WebSocket to ${wsurl}`);
        this.ws = new WebSocket(wsurl);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onmessage = (message) => this.onMessage(message);
        this.ws.onclose = () => setTimeout(() => this.ws.close(), this.connectWebSocket(wsurl), 1000);
    }
}