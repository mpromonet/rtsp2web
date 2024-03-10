/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

export class VideoProcessor {
    constructor(videoCanvas) {
        this.videoContext = videoCanvas.getContext("2d");
        this.decoder = this.createVideoDecoder();
    }

    async decodeFrame(metadata, data) {
        if (this.decoder.state === "configured") {
            const chunk = new EncodedVideoChunk({
                timestamp: metadata.ts,
                type: "key",
                data,
            });
            this.decoder.decode(chunk);
            return Promise.resolve();
        } else {
            return Promise.reject(`${metadata.codec} decoder not configured`);
        }
    }

    async onH26xFrame(metadata, data) {
        if (this.decoder.state !== "configured" && metadata.type === "keyframe" ) {
            const codec = metadata.codec;
            const config = { codec };
            const support = await VideoDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`decoder supported with codec ${codec}`);
                this.decoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        return this.decodeFrame(metadata, data);
    }

    async onJPEGFrame(metadata, data) {
        let binaryStr = "";
        for (let i = 0; i < data.length; i++) {
          binaryStr += String.fromCharCode(data[i]);
        }
        const img = new Image();
        img.src = "data:image/jpeg;base64," + btoa(binaryStr);
        await new Promise(r => img.onload=r);
        return new VideoFrame(img, {timestamp: metadata.ts});
    }

    onVideoFrame(metadata, data) {
        if (metadata.codec.startsWith('avc1') || metadata.codec.startsWith('hev1') ) {
            return this.onH26xFrame(metadata, data);
        } else if (metadata.codec === 'jpeg') {
            return this.onJPEGFrame(metadata, data);
        } else {
            return Promise.reject(`Unknown format`);
        }
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

    reset() {
        this.decoder = this.createVideoDecoder();
    }
}
