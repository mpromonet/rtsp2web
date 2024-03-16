/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/


export class AudioProcessor {
    constructor(audioContext) {
        this.audioContext = audioContext;
        this.audioBufferQueue = { bufferQueue: new Set(), nextBufferTime: 0 };
        this.gain = this.audioContext.createGain();
        this.gain.connect(this.audioContext.destination);
        this.decoder = null;
    }

    async processAudioFrame(frame) {
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

        this.queueAudioBuffer(audioBuffer);

        frame.close();
    }

    queueAudioBuffer(audioBuffer) {
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

    setVolume(volume) {
        this.gain.gain.value = volume;
    }
    
    createAudioDecoder() {
        return new AudioDecoder({
                output: (frame) => this.processAudioFrame(frame),
                error: (e) => console.log(e.message),
        });
    }

    async onAudioFrame(metadata, data) {
        if (!this.decoder || this.decoder.state === "closed") {
            this.decoder = this.createAudioDecoder();
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
        return this.decodeAudioFrame(metadata, data);
    }

    async decodeAudioFrame(metadata, data) {
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
}
