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

    async onH264Frame(data) {
        if (!this.decoder) {
            this.decoder = new VideoDecoder({
                output: (frame) => this.frameResolved(frame),
                error: (e) => console.log(e.message),
            });
        }

        const naluType = data[4] & 0x1F;
        if (this.decoder.state !== "configured" && naluType === 7) {
            let codec = 'avc1.';
            for (let i = 0; i < 3; i++) {
                codec += ('00' + data[5 + i].toString(16)).slice(-2);
            }

            const config = { codec };
            const support = await VideoDecoder.isConfigSupported(config);
            if (support.supported) {
                this.decoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        if (this.decoder.state === "configured") {
            const chunk = new EncodedVideoChunk({
                timestamp: performance.now(),
                type: (naluType === 7) || (naluType === 5) ? "key" : "delta",
                data,
            });
            this.decoder.decode(chunk);
            return new Promise(r => this.frameResolved = r);
        } else {
            return Promise.reject(`H264 decoder not configured`);
        }
    }

    onFrame(data) {
        if ((data.length > 3) && (data[0] === 0) && (data[1] === 0) && (data[2] === 0) && (data[3] === 1)) {
            return this.onH264Frame(data);
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
                this.videoElement.title = e;
            }
        }
    }

    connectWebSocket(wsurl) {
        console.log(`Connecting WebSocket to ${wsurl}`);
        let ws = new WebSocket(wsurl);
        ws.binaryType = 'arraybuffer';
        ws.onmessage = (message) => this.onMessage(message);
        ws.onclose = () => setTimeout(() => ws.close(), this.connectWebSocket(wsurl), 1000);
    }
}
