/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

import { Spinner } from './spinner.js';

export class Canvas2DRenderer {
  ctx = null;
  spinner = null;

  constructor(canvas) {
    this.ctx = canvas.getContext("2d");
    this.spinner = new Spinner(canvas);
  }

  draw(frame) {
    this.spinner.stop();
    this.ctx.canvas.width = frame.displayWidth;
    this.ctx.canvas.height = frame.displayHeight;
    this.ctx.drawImage(frame, 0, 0, frame.displayWidth, frame.displayHeight);
    frame.close();
  }



  drawText(text) {
    this.spinner.stop();
    this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    this.ctx.font = "16px Arial";
    this.ctx.textAlign = 'center'; 
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText(text, this.ctx.canvas.width/2, this.ctx.canvas.height/2);    
  }

  clear() {
    this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
    this.spinner.start();
  }
};
