/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

class VideoStream {
    constructor(videoCanvas) {
        this.frameResolved = null;
        this.metadata = {codec: '', ts: 0, type: ''};
        this.reconnectTimer = null;
        this.ws = null;

        this.videoContext = videoCanvas.getContext("2d");        

        this.decoder = this.createDecoder();
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
        this.clearFrame();
        return   this.decoder = new VideoDecoder({
                output: (frame) => this.displayFrame(frame),
                error: (e) => console.log(e.message),
        });
    }

    async decodeFrame(data) {
        if (this.decoder.state === "configured") {
            const chunk = new EncodedVideoChunk({
                timestamp: this.metadata.ts,
                type: "key",
                data,
            });
            this.decoder.decode(chunk);
            return new Promise(r => this.frameResolved = r);
        } else {
            return Promise.reject(`${this.metadata.codec} decoder not configured`);
        }
    }

    async onH26xFrame(data) {
        if (this.decoder.state !== "configured" && this.metadata.type === "keyframe" ) {
            const codec = this.metadata.codec;
            const config = { codec };
            const support = await VideoDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`decoder supported with codec ${codec}`);
                this.decoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        return this.decodeFrame(data);
    }

    async onJPEGFrame(data) {
        let binaryStr = "";
        for (let i = 0; i < data.length; i++) {
          binaryStr += String.fromCharCode(data[i]);
        }
        const img = new Image();
        img.src = "data:image/jpeg;base64," + btoa(binaryStr);
        await new Promise(r => img.onload=r);
        return new VideoFrame(img, {timestamp: this.metadata.ts});
    }

    onFrame(data) {
        if (this.metadata.codec.startsWith('avc1') || this.metadata.codec.startsWith('hev1') ) {
            return this.onH26xFrame(data);
        } else if (this.metadata.codec === 'JPEG') {
            return this.onJPEGFrame(data);
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
            } catch (e) {
                console.warn(e);
            }
        } else if (typeof data === 'string') {
            this.metadata = JSON.parse(data);
        }
    }

    connect(stream) {
        let wsurl = new URL(stream, location.href);
        wsurl.protocol = wsurl.protocol.replace("http", "ws");
        this.close();
        console.log(`Connecting WebSocket to ${wsurl}`);
        this.ws = new WebSocket(wsurl.href);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onmessage = (message) => this.onMessage(message);
        this.ws.onclose = () => this.reconnectTimer = setTimeout(() => this.connect(stream), 1000);
    }

    close() {
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
    }
}
