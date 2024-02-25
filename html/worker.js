/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

importScripts('videostream.js');

self.onmessage = (event) => {
    const offscreenCanvas = event.data.canvas;
    console.log('worker received canvas', offscreenCanvas);

    const videoStream = new VideoStream(offscreenCanvas);
    videoStream.connect(event.data.url);
};