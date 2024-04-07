/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** -------------------------------------------------------------------------*/

export class WebGPURenderer {
  canvas = null;
  ctx = null;

  started = null;

  device = null;
  pipeline = null;
  sampler = null;

  static shaderSource = `
    struct VertexOutput {
      @builtin(position) Position: vec4<f32>,
      @location(0) uv: vec2<f32>,
    }

    @vertex
    fn vert_main(@builtin(vertex_index) VertexIndex: u32) -> VertexOutput {
      var pos = array<vec2<f32>, 6>(
        vec2<f32>( 1.0,  1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0,  1.0),
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(-1.0,  1.0)
      );

      var uv = array<vec2<f32>, 6>(
        vec2<f32>(1.0, 0.0),
        vec2<f32>(1.0, 1.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(1.0, 0.0),
        vec2<f32>(0.0, 1.0),
        vec2<f32>(0.0, 0.0)
      );

      var output : VertexOutput;
      output.Position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
      output.uv = uv[VertexIndex];
      return output;
    }

    @group(0) @binding(1) var mySampler: sampler;
    @group(0) @binding(2) var myTexture: texture_external;
    
    @fragment
    fn frag_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
      return textureSampleBaseClampToEdge(myTexture, mySampler, uv);
    }
  `;

  constructor(canvas) {
    this.canvas = canvas;
    if (!navigator.gpu) {
      throw new Error("WebGPU is not supported in this browser.");
    }
    this.started = this._start();
  }

  async _start() {
    const adapter = await navigator.gpu.requestAdapter();
    this.device = await adapter.requestDevice();
    const format = navigator.gpu.getPreferredCanvasFormat();

    this.ctx = this.canvas.getContext("webgpu");
    this.ctx.configure({
      device: this.device,
      format,
      alphaMode: "opaque",
    });

    const module =  this.device.createShaderModule({code: WebGPURenderer.shaderSource});
    this.pipeline = this.device.createRenderPipeline({
      layout: "auto",
      vertex: {
        module,
        entryPoint: "vert_main"
      },
      fragment: {
        module,
        entryPoint: "frag_main",
        targets: [{format}]
      },
      primitive: {
        topology: "triangle-list"
      }
    });
  
    this.sampler = this.device.createSampler({});
  }

  _createBindGroup(frame) {
    if (frame) {
      this.canvas.width = frame.displayWidth;
      this.canvas.height = frame.displayHeight;

      return this.device.createBindGroup({
        layout: this.pipeline.getBindGroupLayout(0),
        entries: [
          {binding: 1, resource: this.sampler},
          {binding: 2, resource: this.device.importExternalTexture({source: frame})}
        ],
      });
    } else{
      return null;
    }
  }

  _createCommandEncoder(frame) {
    const uniformBindGroup = this._createBindGroup(frame);
    const commandEncoder = this.device.createCommandEncoder();
    const view = this.ctx.getCurrentTexture().createView();
    const renderPassDescriptor = {
      colorAttachments: [
        {
          view,
          clearValue: [1.0, 1.0, 1.0, 1.0],
          loadOp: "clear",
          storeOp: "store",
        },
      ],
    };
    const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);    
    if (uniformBindGroup) {
      passEncoder.setPipeline(this.pipeline);
      passEncoder.setBindGroup(0, uniformBindGroup);
      passEncoder.draw(6);
    }
    passEncoder.end();

    return commandEncoder;
  }

  async draw(frame) {
    await this.started;

    const commandEncoder = this._createCommandEncoder(frame);
    this.device.queue.submit([commandEncoder.finish()]);

    frame?.close();
  }

  _createTextCanvas(text) {
    const canvas = new OffscreenCanvas(this.ctx.canvas.width, this.ctx.canvas.height);
    const context = canvas.getContext('2d');
    context.fillStyle = 'white';
    context.fillRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = 'black';    
    context.textAlign = 'center';
    context.textBaseline = 'middle';
    context.font = "16px Arial";
    context.fillText(text, canvas.width/2, canvas.height/2);
    return canvas;
  }

  async drawText(text) {
    const canvas = this._createTextCanvas(text);
    const bitmap = await createImageBitmap(canvas);
    const frame = new VideoFrame(bitmap, { timestamp: performance.now() });
    return this.draw(frame);
  }

  async clear() {
    return this.draw();
  }
};
