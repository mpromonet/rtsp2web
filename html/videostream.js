/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

class VideoStream {
    constructor(videoCanvas, audioTrack) {
        this.metadata = {media:'', codec: '', ts: 0, type: ''};
        this.reconnectTimer = null;
        this.ws = null;

        this.videoContext = videoCanvas.getContext("2d");
        this.videoDecoder = this.createVideoDecoder();

        this.audioTrack = audioTrack;
        this.audioDecoder = this.createAudioDecoder();        
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

    createVideoDecoder() {
        this.clearFrame();
        return new VideoDecoder({
                output: (frame) => this.displayFrame(frame),
                error: (e) => console.log(e.message),
        });
    }

    async playAudioFrame(frame) {
        const { numberOfChannels, numberOfFrames, sampleRate, format } = frame;

        const audioBuffer = this.audioTrack.context.createBuffer(numberOfChannels, numberOfFrames, sampleRate);
        if (format.startsWith('planar')) {
            for (let channel = 0; channel < numberOfChannels; channel++) {
                const channelData = new Float32Array(numberOfFrames);
                frame.copyTo(channelData, { planeIndex: channel });
                audioBuffer.copyToChannel(channelData, channel);
            }
        } else {
            const interleavingBuffer = new Float32Array(numberOfFrames*numberOfChannels);
            frame.copyTo(interleavingBuffer, { planeIndex: 0 });
            for (let channel = 0; channel < numberOfChannels; channel++) {
                const channelData = new Float32Array(numberOfFrames);
                for (let i = 0; i < numberOfFrames; i++) {
                    channelData[i] = interleavingBuffer[i * numberOfChannels + channel];
                }
                audioBuffer.copyToChannel(channelData, channel);
            }            
        }

        const source = this.audioTrack.context.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(this.audioTrack.context.destination);
        source.start();

        frame.close();
    }

    createAudioDecoder() {
        return new AudioDecoder({
                output: (frame) => this.playAudioFrame(frame),
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
            return Promise.resolve();
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

    onVideoFrame(data) {
        if (this.metadata.codec.startsWith('avc1') || this.metadata.codec.startsWith('hev1') ) {
            return this.onH26xFrame(data);
        } else if (this.metadata.codec === 'jpeg') {
            return this.onJPEGFrame(data);
        } else {
            return Promise.reject(`Unknown format`);
        }
    }

    async onAudioFrame(data) {
        if (this.audioDecoder.state !== "configured") {
            const codec = this.metadata.codec;
            const sampleRate = 48000;
            const numberOfChannels = 2;
            const config = { codec, sampleRate, numberOfChannels };
            const support = await AudioDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`decoder supported with codec ${codec}`);
                await this.audioDecoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        return this.decodeAudioFrame(data);
    }

    async decodeAudioFrame(data) {
        if (this.audioDecoder.state === "configured") {
            const chunk = new EncodedAudioChunk({
                timestamp: this.metadata.ts,
                type: "key",
                data,
            });
            await this.audioDecoder.decode(chunk);
            return Promise.resolve();
        } else {
            return Promise.reject(`${this.metadata.codec} decoder not configured`);
        }
    }

    async onMessage(message) {
        const { data } = message;
        try {
            if (data instanceof ArrayBuffer) {
                const bytes = new Uint8Array(data);
                if (this.metadata.media === 'video') {
                    await this.onVideoFrame(bytes);
                } else if (this.metadata.media === 'audio') {
                    await this.onAudioFrame(bytes);
                }
            } else if (typeof data === 'string') {
                this.metadata = JSON.parse(data);
            }
        } catch (e) {
            console.warn(e);
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
        this.decoder = this.createVideoDecoder();
    }
}
