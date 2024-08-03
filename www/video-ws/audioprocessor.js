/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/


export class AudioProcessor {
    decoder = null;
    audioBufferQueue = { bufferQueue: new Set(), nextBufferTime: 0 };

    constructor(audioContext) {
        this.audioContext = audioContext;
        this.gain = this.audioContext.createGain();
        this.gain.connect(this.audioContext.destination);
    }

    setVolume(volume) {
        this.gain.gain.value = volume;
    }

    async onAudioFrame(metadata, data) {
        if (!this.decoder || this.decoder.state === "closed") {
            this.decoder = this._createAudioDecoder();
        }
        if (this.decoder.state !== "configured") {
            const codec = metadata.codec;
            const sampleRate = metadata.freq;
            const numberOfChannels = metadata.channels;
            const config = { codec, sampleRate, numberOfChannels };
            const support = await AudioDecoder.isConfigSupported(config);
            if (support.supported) {
                console.log(`decoder supported with codec ${codec}`);
                await this.decoder.configure(config);
            } else {
                return Promise.reject(`${codec} is not supported`);
            }
        }
        if (this.decoder.state === "configured") {
            const chunk = new EncodedAudioChunk({
                timestamp: metadata.ts,
                type: "key",
                data,
            });
            await this.decoder.decode(chunk);
            return Promise.resolve();
        } else {
            return Promise.reject(`${metadata.codec} decoder not configured`);
        }
    }

    close() {
        if (this.decoder && this.decoder.state !== "closed") {
            this.decoder.close();
        }
        this.audioBufferQueue.bufferQueue.forEach(source => source.stop());
        this.audioBufferQueue.bufferQueue.clear();
    }    

    async _processAudioFrame(frame) {
        const { numberOfChannels, numberOfFrames, sampleRate, format } = frame;

        const audioBuffer = this.audioContext.createBuffer(numberOfChannels, numberOfFrames, sampleRate);
        if (format.endsWith('-planar')) {
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

        this._queueAudioBuffer(audioBuffer);

        frame.close();
    }

    _queueAudioBuffer(audioBuffer) {
        const source = this.audioContext.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(this.gain);
        this.audioBufferQueue.bufferQueue.add(source);

        if (this.audioContext.currentTime > this.audioBufferQueue.nextBufferTime) {
            source.start();
            this.audioBufferQueue.nextBufferTime = this.audioContext.currentTime + audioBuffer.duration;
        } else {
            source.start(this.audioBufferQueue.nextBufferTime);
            this.audioBufferQueue.nextBufferTime += audioBuffer.duration;
        }

        source.onended = () => this.audioBufferQueue.bufferQueue.delete(source);
    }

    _createAudioDecoder() {
        return new AudioDecoder({
                output: (frame) => this._processAudioFrame(frame),
                error: (e) => console.log(e.message),
        });
    }
}
