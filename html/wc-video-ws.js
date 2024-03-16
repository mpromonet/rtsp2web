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

        this.audioContext = new AudioContext();
        const canvas = document.createElement("canvas");
        this.videoStream = new VideoStream(canvas, this.audioContext);

        this.stream = canvas.captureStream();
        this.stream.addTrack(this.audioContext.createMediaStreamDestination().stream.getAudioTracks()[0]);          
    }
  
    connectedCallback() {
        console.log("connectedCallback");

        const video = this.shadowDOM.getElementById("video");
        video.srcObject = this.stream;
        video.addEventListener('play', () => this.audioContext.resume());
        video.addEventListener('pause', () => this.audioContext.suspend());
        video.addEventListener('volumechange', () => this.videoStream.setVolume(video.muted ? 0 : video.volume));
        this.audioContext.onstatechange = () => {
            switch(this.audioContext.state) {
                case 'suspended':
                    video.muted = true;
                    break;
                case 'running':
                    video.muted = false;
                    break;
            }
        };        
        video.play();

        if (this.hasAttribute("url")) {
            this.videoStream.connect(this.getAttribute("url"));
        }
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