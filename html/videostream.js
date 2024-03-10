/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { VideoProcessor } from './videoprocessor.js';
import { AudioProcessor } from './audioprocessor.js';

export class VideoStream {
    constructor(videoCanvas, audioContext) {
        this.metadata = {media:'', codec: '', ts: 0, type: ''};
        this.reconnectTimer = null;
        this.ws = null;

        this.videoProcessor = new VideoProcessor(videoCanvas);

        this.audioProcessor = new AudioProcessor(audioContext);
    }

    async onMessage(message) {
        const { data } = message;
        try {
            if (data instanceof ArrayBuffer) {
                const bytes = new Uint8Array(data);
                if (this.metadata.media === 'video') {
                    await this.videoProcessor.onVideoFrame(this.metadata, bytes);
                } else if (this.metadata.media === 'audio') {
                    await this.audioProcessor.onAudioFrame(this.metadata, bytes);
                }
            } else if (typeof data === 'string') {
                this.metadata = JSON.parse(data);
            }
        } catch (e) {
            console.warn(e);
        }
    }

    setVolume(volume) {
        this.audioProcessor.setVolume(volume);
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
        this.videoProcessor.reset();
    }
}
