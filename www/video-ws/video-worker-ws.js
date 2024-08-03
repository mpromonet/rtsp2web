/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

class VideoWorkerWsElement extends HTMLElement {
    static observedAttributes = ["url"];
  
    constructor() {
        super();
        this.shadowDOM = this.attachShadow({mode: 'open'});
        this.shadowDOM.innerHTML = `
                  <link rel="stylesheet" href="video-ws/video-ws.css">
                  <div class="videoContent">
                    <video id="video" muted playsinline controls preload="none"></video>
                    <div id="spinner" class="loading"></div>
                  </div>`;      
                  
        const canvas = document.createElement("canvas");
        const offscreenCanvas = canvas.transferControlToOffscreen();
        this.worker = new Worker('video-ws/worker.js',  { type: "module" });
        this.worker.postMessage({ canvas: offscreenCanvas }, [offscreenCanvas]); 
        this.worker.onmessage = (e) => {
            if (e.data.type === "loaded") {
                if (e.data.loaded) {
                    this.shadowDOM.getElementById("spinner").classList.remove("loading");
                } else {
                    this.shadowDOM.getElementById("spinner").classList.add("loading");
                }
            }
        }
        this.stream = canvas.captureStream();
    }
  
    connectedCallback() {
        console.log("connectedCallback");


        const video = this.shadowDOM.getElementById("video");
        video.srcObject = this.stream;
        video.play();
    }
  
    disconnectedCallback() {
        console.log("disconnectedCallback");
        if (this.worker) {
            this.worker.terminate();
        }
    }
    
    attributeChangedCallback(name, oldValue, newValue) {
        console.log(`Attribute ${name} has changed.`);
        if (name === "url" && oldValue !== newValue) {
            this.shadowDOM.getElementById("spinner").classList.add("loading");
            this.worker.postMessage({ url: newValue });
        }
    }
  }
  
  customElements.define("video-worker-ws", VideoWorkerWsElement);