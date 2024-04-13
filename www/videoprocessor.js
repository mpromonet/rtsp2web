/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { WebGPURenderer } from './webgpu-renderer.js';
import { Canvas2DRenderer } from './canvas2d-renderer.js';

export class VideoProcessor {
    renderer = null;
    decoder = null;
    onloadedcallback = null;

    constructor(videoCanvas, onloadedcallback) {
        this.onloadedcallback = onloadedcallback;
        try {
            this.renderer = new WebGPURenderer(videoCanvas);
        } catch(e) {
            console.log(`WebGPU not supported: ${e.message} fallback to Canvas2DRenderer`);
            this.renderer = new Canvas2DRenderer(videoCanvas);
        }
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
                this.renderer.drawText(`Codec ${codec} not supported`);
                this?.onloadedcallback(false);
                return Promise.reject(`${codec} is not supported`);
            }
        }
        return this.decodeFrame(metadata, data);
    }

    async onJPEGFrame(metadata, data) {
        const decoder = new ImageDecoder({data, type: 'image/jpeg'});
        const image = await decoder.decode();
        const frame = new VideoFrame(image.image, {timestamp: metadata.ts});
        this.renderer.draw(frame);
        this?.onloadedcallback(true);
        return Promise.resolve();
    }

    onVideoFrame(metadata, data) {
        if (metadata.codec.startsWith('avc1') || metadata.codec.startsWith('hev1') ) {
            return this.onH26xFrame(metadata, data);
        } else if (metadata.codec === 'jpeg') {
            return this.onJPEGFrame(metadata, data);
        } else {
            this.renderer.drawText(`Codec ${metadata.codec} unknown`);
            this?.onloadedcallback(false);
            return Promise.reject(`Unknown format`);
        }
    }

    createVideoDecoder() {
        return new VideoDecoder({
            output: (frame) => {
                this.renderer.draw(frame);
                this?.onloadedcallback(true);
            },
            error: (e) => console.log(e.message),
        });
    }    

    close() {
        this.renderer.clear()
        if (this.decoder && this.decoder.state !== "closed") {
            this.decoder.close();
        }
    }
}
