<template>
    <v-app>
      <v-main id="content">
        <h1>{{ message }}</h1>
        <video v-show="!message" id="video" muted class="h-100" preload="none"></video>
      </v-main>
      <v-divider></v-divider>
      <v-footer>
        <v-container>
          <v-row align="center" justify="center">
                <v-icon>mdi-github</v-icon><a href="https://github.com/mpromonet/rtsp2ws">rtsp2ws</a><p>{{ version }}</p>
          </v-row>
       </v-container>
      </v-footer>
    </v-app>
</template>

<script>
import axios from "axios";
axios.defaults.baseURL = import.meta.env.VITE_APP_BASE_URL

export default {
  name: 'App',
  components: {},
  mounted() {
    const videoCanvas = document.createElement("canvas");
    this.videoCanvas = videoCanvas.getContext("2d");
    const video = document.getElementById("video");
    video.srcObject = videoCanvas.captureStream();
    video.play();
    this.videoCanvas.clearRect(0,0,videoCanvas.width,videoCanvas.height);
    axios.get("/api/version").then(
      (response) => this.version = response.data); 
  },
  created() {
    let wsurl = new URL("./ws", import.meta.env.VITE_APP_BASE_URL || location.href);
    wsurl.protocol = wsurl.protocol.replace("http","ws");
    this.connectWebSocket(wsurl.href);
  },
  data() {
    return {
      message: "...",
      version: "",
      videoCanvas: null,
    };
  },
  methods: {
    connectWebSocket(wsurl) {
        console.log(`Connecting WebSocket to ${wsurl}`);
        this.ws = new WebSocket(wsurl);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onmessage = this.onMessage;
        this.ws.onclose = () => setTimeout(() => this.connectWebSocket(wsurl), 1000);
    },
    async onMessage(message) {
      const { data } = message;
      if (typeof data === 'string') {
        console.log(data);

      } else if (data instanceof ArrayBuffer) {
        const bytes = new Uint8Array(data);
        try {
          const frame = await this.onFrame(bytes);
          this.displayFrame(frame);
          this.message = null;
        } catch (e) {
          this.message = e;
        }
      }
    },
    displayFrame(frame) {
        this.videoCanvas.canvas.width = frame.displayWidth;
        this.videoCanvas.canvas.height = frame.displayHeight;
        this.videoCanvas.drawImage(frame, 0, 0);
        frame.close();
    },
    async onH264Frame(bytes) {
      if (!this.ws.decoder) {
        this.ws.decoder = new VideoDecoder({
          output: (frame) => this.frameResolved(frame),
          error: (e) => console.log(e.message),
        });
      }

      const naluType = bytes[4] & 0x1F;
      if (this.ws.decoder.state !== "configured" && naluType === 7) {
          let codec = 'avc1.';
          for (let i = 0; i < 3; i++) {
              codec += ('00' + bytes[5+i].toString(16)).slice(-2);
          }
          console.log(codec);
          const config = {codec};
          const support = await VideoDecoder.isConfigSupported(config);
          if (support.supported) {
            this.ws.decoder.configure(config);
          } else {
            return Promise.reject(`${codec} is not supported`);
          }
      }
      if (this.ws.decoder.state === "configured") {
          const chunk = new EncodedVideoChunk({
              timestamp: performance.now(),
              type: (naluType === 7) || (naluType === 5) ? "key" : "delta",
              data: bytes,
          });
          this.ws.decoder.decode(chunk);
          return new Promise(r => this.frameResolved = r);
      } else {
        return Promise.reject(`H264 decoder not configured`);
      }
    },
    onFrame(bytes) {
      if ( (bytes.length > 3) && (bytes[0] === 0) && (bytes[1] === 0) && (bytes[2] === 0) && (bytes[3] === 1)) {
        return this.onH264Frame(bytes);

      } else {
        return Promise.reject(`Unknown format`);
      }
    },
  }
}
</script>

<style>
html {
  overflow-y: hidden;
}
#content {
  height: calc(100vh - 100px);
  overflow-y: auto;
  text-align: center;
}
h1 {
  text-align: center;
}
h3 {
  text-align: center;
}
p {
  margin-left: 0.5em;
}
</style>
