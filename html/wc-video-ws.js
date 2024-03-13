/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { VideoStream } from './videostream.js';

class VideoWsElement extends HTMLElement {
    static observedAttributes = ["url"];
  
    constructor() {
      super();
      this.shadowDOM = this.attachShadow({mode: 'open'});
      this.shadowDOM.innerHTML = `
                  <style>
                  video {
                    width: auto;
                    height: 100%;
                    display: block;
                    margin: 0 auto;
                  }
                  </style>
                  <video id="video" muted playsinline controls preload="none"></video>`;      
    }
  
    connectedCallback() {
        console.log("connectedCallback");
        const audioContext = new AudioContext();
        const canvas = document.createElement("canvas");
        this.videoStream = new VideoStream(canvas, audioContext);

        const stream = canvas.captureStream();
        const audioTrack = audioContext.createMediaStreamDestination().stream.getAudioTracks()[0];
        stream.addTrack(audioTrack);

        const video = this.shadowDOM.getElementById("video");
        video.srcObject = stream;
        video.addEventListener('play', () => audioContext.resume());
        video.addEventListener('pause', () => audioContext.suspend());
        video.addEventListener('volumechange', () => this.videoStream.setVolume(video.muted ? 0 : video.volume));
        video.play();
    }
  
    disconnectedCallback() {
        console.log("disconnectedCallback");
        this.videoStream.close();
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        console.log(`Attribute ${name} has changed.`);
        if (name === "url") {
            this.videoStream.connect(newValue);
        }
    }
  }
  
  customElements.define("wc-video-ws", VideoWsElement);