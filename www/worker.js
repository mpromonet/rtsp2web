/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { MediaStream } from './mediastream.js';

self.onmessage = (event) => {
    const offscreenCanvas = event.data.canvas;
    console.log('worker received canvas', offscreenCanvas);

    const mediaStream = new MediaStream(offscreenCanvas, null, () => postMessage({type: 'loaded'}));
    mediaStream.connect(event.data.url);
};