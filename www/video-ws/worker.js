/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { MediaStream } from './mediastream.js';

self.onmessage = (event) => {
    if (event.data.canvas) {
        const offscreenCanvas = event.data.canvas;
        console.log('worker received canvas', offscreenCanvas);
    
        self.mediaStream = new MediaStream(offscreenCanvas, null, (loaded) => postMessage({type: 'loaded', loaded}));
    
    }
    if (event.data.url && self.mediaStream) {
        self.mediaStream.connect(event.data.url);
    }
};