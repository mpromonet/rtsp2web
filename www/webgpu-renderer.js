export class WebGPURenderer {
  #canvas = null;
  #ctx = null;

  // Promise for `#start()`, WebGPU setup is asynchronous.
  #started = null;

  // WebGPU state shared between setup and drawing.
  #format = null;
  #device = null;
  #pipeline = null;
  #sampler = null;

  // Generates two triangles covering the whole canvas.
  static vertexShaderSource = `
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
  `;

  // Samples the external texture using generated UVs.
  static fragmentShaderSource = `
    @group(0) @binding(1) var mySampler: sampler;
    @group(0) @binding(2) var myTexture: texture_external;
    
    @fragment
    fn frag_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
      return textureSampleBaseClampToEdge(myTexture, mySampler, uv);
    }
  `;

  constructor(canvas) {
    this.#canvas = canvas;
    if (!navigator.gpu) {
      throw new Error("WebGPU is not supported in this browser.");
    }
    this.#started = this.#start();
  }

  async #start() {
    const adapter = await navigator.gpu.requestAdapter();
    this.#device = await adapter.requestDevice();
    this.#format = navigator.gpu.getPreferredCanvasFormat();

    this.#ctx = this.#canvas.getContext("webgpu");
    this.#ctx.configure({
      device: this.#device,
      format: this.#format,
      alphaMode: "opaque",
    });

    this.#pipeline = this.#device.createRenderPipeline({
      layout: "auto",
      vertex: {
        module: this.#device.createShaderModule({code: WebGPURenderer.vertexShaderSource}),
        entryPoint: "vert_main"
      },
      fragment: {
        module: this.#device.createShaderModule({code: WebGPURenderer.fragmentShaderSource}),
        entryPoint: "frag_main",
        targets: [{format: this.#format}]
      },
      primitive: {
        topology: "triangle-list"
      }
    });
  
    // Default sampler configuration is nearset + clamp.
    this.#sampler = this.#device.createSampler({});
  }

  _createBindGroup(frame) {
    if (frame) {
      this.#canvas.width = frame.displayWidth;
      this.#canvas.height = frame.displayHeight;

      return this.#device.createBindGroup({
        layout: this.#pipeline.getBindGroupLayout(0),
        entries: [
          {binding: 1, resource: this.#sampler},
          {binding: 2, resource: this.#device.importExternalTexture({source: frame})}
        ],
      });
    } else{
      return null;
    }
  }

  _createCommandEncoder() {
    const commandEncoder = this.#device.createCommandEncoder();
    const view = this.#ctx.getCurrentTexture().createView();
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

    return {commandEncoder, passEncoder};
  }

  async draw(frame) {
    // Don't try to draw any frames until the context is configured.
    await this.#started;

    const uniformBindGroup = this._createBindGroup(frame);
    const {commandEncoder, passEncoder} = this._createCommandEncoder();
    if (uniformBindGroup) {
      passEncoder.setPipeline(this.#pipeline);
      passEncoder.setBindGroup(0, uniformBindGroup);
      passEncoder.draw(6);
    }
    passEncoder.end();
    this.#device.queue.submit([commandEncoder.finish()]);

    frame?.close();
  }

  async drawText(text) {
    const canvas = new OffscreenCanvas(this.#ctx.canvas.width, this.#ctx.canvas.height);
    const context = canvas.getContext('2d');
    context.fillStyle = 'white';
    context.fillRect(0, 0, this.#canvas.width, this.#canvas.height);
    context.fillStyle = 'black';    
    context.textAlign = 'center';
    context.textBaseline = 'middle';
    context.font = "16px Arial";
    const centerX = this.#ctx.canvas.width / 2;
    const centerY = this.#ctx.canvas.height / 2;    
    context.fillText(text, centerX, centerY);    
    const bitmap = await createImageBitmap(canvas);
    const frame = new VideoFrame(bitmap, { timestamp: performance.now() });

    await this.draw(frame);
  }

  async clear() {
    this.draw();
  }
};
