/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { VideoProcessor } from './videoprocessor.js';
import { AudioProcessor } from './audioprocessor.js';

export class MediaStream {
    reconnectTimer = null;
    ws = null;
    metadata = {media:'', codec: '', freq: 0, channels: 0, ts: 0, type: ''};

    constructor(videoCanvas, audioContext, onloadedcallback) {
        this.videoProcessor = new VideoProcessor(videoCanvas, onloadedcallback);
        if (audioContext) {
            this.audioProcessor = new AudioProcessor(audioContext);
        }
    }

    async onMessage(message) {
        const { data } = message;
        try {
            if (data instanceof ArrayBuffer) {
                const bytes = new Uint8Array(data);
                if (this.metadata.media === 'video') {
                    await this.videoProcessor.onVideoFrame(this.metadata, bytes);
                } else if (this.metadata.media === 'audio') {
                    await this.audioProcessor?.onAudioFrame(this.metadata, bytes);
                }
            } else if (typeof data === 'string') {
                this.metadata = JSON.parse(data);
            }
        } catch (e) {
            console.warn(e);
        }
    }

    setVolume(volume) {
        this.audioProcessor?.setVolume(volume);
    }
    
    connect(stream) {
        let wsurl = new URL(stream, location.href);
        wsurl.protocol = wsurl.protocol.replace("http", "ws");
        this.close();
        console.log(`Connecting WebSocket to ${wsurl}`);
        this.ws = new WebSocket(wsurl.href);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onopen = () => clearTimeout(this.reconnectTimer);
        this.ws.onerror = () => console.log(`WebSocket error`);
        this.ws.onmessage = (message) => this.onMessage(message);
        this.ws.onclose = () => this.reconnectTimer = setTimeout(() => this.connect(stream), 1000);
    }

    close() {
        if (this.reconnectTimer) {
            clearTimeout(this.reconnectTimer);
            this.reconnectTimer = null;
        }
        if (this.ws) {
            this.ws.onclose = () => {};
            this.ws.onerror = () => {};
            this.ws.close();
        }
        this.ws = null;
        this.videoProcessor.close();
        this.audioProcessor?.close();
    }
}
