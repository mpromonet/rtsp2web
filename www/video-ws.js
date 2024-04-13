/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { MediaStream } from './mediastream.js';

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
                  .loading {
                    position: fixed;
                    z-index: 999;
                    height: 2em;
                    width: 2em;
                    overflow: visible;
                    margin: auto;
                    top: 0;
                    left: 0;
                    bottom: 0;
                    right: 0;
                  }
                  
                  .loading:before {
                    content: '';
                    display: block;
                    position: fixed;
                    background-color: rgba(0,0,0,0.3);
                  }
                  
                  .loading:not(:required) {
                    font: 0/0 a;
                    color: transparent;
                    text-shadow: none;
                    background-color: transparent;
                    border: 0;
                  }
                  
                  .loading:not(:required):after {
                    content: '';
                    display: block;
                    font-size: 10px;
                    width: 1em;
                    height: 1em;
                    margin-top: -0.5em;
                    animation: spinner 1500ms infinite linear;
                    border-radius: 0.5em;
                    box-shadow: rgba(0, 0, 0, 0.75) 1.5em 0 0 0, rgba(0, 0, 0, 0.75) 1.1em 1.1em 0 0, rgba(0, 0, 0, 0.75) 0 1.5em 0 0, rgba(0, 0, 0, 0.75) -1.1em 1.1em 0 0, rgba(0, 0, 0, 0.75) -1.5em 0 0 0, rgba(0, 0, 0, 0.75) -1.1em -1.1em 0 0, rgba(0, 0, 0, 0.75) 0 -1.5em 0 0, rgba(0, 0, 0, 0.75) 1.1em -1.1em 0 0;
                  }
                  
                  @keyframes spinner {
                    0% {
                      transform: rotate(0deg);
                    }
                    100% {
                      transform: rotate(360deg);
                    }
                  }
                  
                  </style>
                  <div id="videowrapper">
                      <video id="video" muted playsinline controls preload="none"></video>
                  </div>`;      
                  

        this.audioContext = new AudioContext();
        const canvas = document.createElement("canvas");
        this.mediaStream = new MediaStream(canvas, this.audioContext, () => this.shadowDOM.getElementById("videowrapper").classList.remove("loading"));

        this.stream = canvas.captureStream();
        this.stream.addTrack(this.audioContext.createMediaStreamDestination().stream.getAudioTracks()[0]);          
    }
  
    connectedCallback() {
        console.log("connectedCallback");

        const video = this.shadowDOM.getElementById("video");
        video.srcObject = this.stream;
        video.addEventListener('play', () => this.audioContext.resume());
        video.addEventListener('pause', () => this.audioContext.suspend());
        video.addEventListener('volumechange', () => this.mediaStream.setVolume(video.muted ? 0 : video.volume));
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
    }
  
    disconnectedCallback() {
        console.log("disconnectedCallback");
        this.mediaStream.close();
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        console.log(`Attribute ${name} has changed.`);
        if (name === "url" && oldValue !== newValue) {
            this.shadowDOM.getElementById("videowrapper").classList.add("loading");
            this.mediaStream.connect(newValue);
        }
    }
  }
  
  customElements.define("video-ws", VideoWsElement);