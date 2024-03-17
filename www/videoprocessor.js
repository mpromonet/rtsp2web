/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

export class VideoProcessor {
    constructor(videoCanvas) {
        this.videoContext = videoCanvas.getContext("2d");
        this.decoder = null;
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
        if (!this.decoder || this.decoder.state === "closed") {
            this.decoder = this.createVideoDecoder();
        }
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
        const decoder = new ImageDecoder({data, type: 'image/jpeg'});
        const image = await decoder.decode();
        const frame = new VideoFrame(image.image, {timestamp: metadata.ts});
        this.displayFrame(frame);
        return Promise.resolve();
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
        return new VideoDecoder({
                output: (frame) => this.displayFrame(frame),
                error: (e) => console.log(e.message),
        });
    }    

    close() {
        this.clearFrame();
        if (this.decoder && this.decoder.state !== "closed") {
            this.decoder.close();
        }
    }
}
