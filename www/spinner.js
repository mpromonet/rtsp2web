/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

export class Spinner {
    spinnerAngle = undefined;
    requestAnimationFrameId = null;

    constructor(canvas) {
        this.ctx = canvas.getContext("2d");
    }

    _showSpinner() {
        if (this.spinnerAngle !== undefined) {
          this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height);
          this.ctx.save();
          this.ctx.translate(this.ctx.canvas.width / 2, this.ctx.canvas.height / 2);
          this.ctx.rotate(this.spinnerAngle);
          this.ctx.beginPath();
          this.ctx.arc(0, 0, this.ctx.canvas.width/10, 0, Math.PI * 1.5);
          this.ctx.strokeStyle = 'black';
          this.ctx.lineWidth = this.ctx.canvas.width/50 | 1;
          this.ctx.stroke();
          this.ctx.restore();
          this.spinnerAngle += 0.05;
          if (this.spinnerAngle > Math.PI * 2) {
              this.spinnerAngle -= Math.PI * 2;
          }
          this.requestAnimationFrameId = requestAnimationFrame(() => this._showSpinner());
        }
    }

    start() {
        this.spinnerAngle = 0;
        this._showSpinner();
    }

    stop() {
        this.spinnerAngle = undefined;
        cancelAnimationFrame(this.requestAnimationFrameId);
        this.requestAnimationFrameId = undefined;
    }
}