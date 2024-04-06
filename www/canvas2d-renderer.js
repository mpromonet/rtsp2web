export class Canvas2DRenderer {
  #canvas = null;
  #ctx = null;

  constructor(canvas) {
    this.#canvas = canvas;
    this.#ctx = canvas.getContext("2d");
  }

  draw(frame) {
    this.#canvas.width = frame.displayWidth;
    this.#canvas.height = frame.displayHeight;
    this.#ctx.drawImage(frame, 0, 0, frame.displayWidth, frame.displayHeight);
    frame.close();
  }

  drawText(text) {
    this.#ctx.font = "16px Arial";
    this.#ctx.textAlign = 'center'; 
    this.#ctx.textBaseline = 'middle';
    const centerX = this.#ctx.canvas.width / 2;
    const centerY = this.#ctx.canvas.height / 2;
    this.#ctx.fillText(text, centerX, centerY);    
  }

  clear() {
    this.#ctx.clearRect(0, 0, this.#canvas.width, this.#canvas.height);
  }
};
