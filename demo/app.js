/* VocaMax Space 2 - interactive web preview
   By Mark33s Beats Lab
   Mirrors the VST3 control mapping so the knobs can be heard in a browser. */

(() => {
  'use strict';

  const clamp = (v, min, max) => Math.min(max, Math.max(min, v));

  const DEFAULTS = { power: 100, space: 65, low: 45, high: 55, focus: 1 };

  const PRESETS = [
    { name: 'Default', power: 100, space: 65, low: 45, high: 55, focus: 1 },
    { name: 'Natural Vocals', power: 85, space: 48, low: 32, high: 50, focus: 0 },
    { name: 'Clear Vocals', power: 100, space: 60, low: 60, high: 70, focus: 1 },
    { name: 'Wide Space', power: 100, space: 88, low: 40, high: 60, focus: 2 },
    { name: 'Deep Pocket', power: 100, space: 35, low: 55, high: 38, focus: 3 },
    { name: 'Tight & Bright', power: 100, space: 22, low: 72, high: 82, focus: 1 },
    { name: 'Max Space', power: 100, space: 100, low: 52, high: 62, focus: 2 },
    { name: 'Radio Ready', power: 100, space: 45, low: 62, high: 60, focus: 1 }
  ];

  const FOCUS = [
    { width: 0.95, presence: 0.0, air: 0.0, lowShelf: 0.0, highPass: 1.00, mid: 0.00 },
    { width: 0.98, presence: 2.5, air: 1.0, lowShelf: -0.5, highPass: 1.15, mid: 0.02 },
    { width: 1.35, presence: 1.0, air: 0.5, lowShelf: 0.0, highPass: 1.00, mid: -0.02 },
    { width: 1.12, presence: -1.0, air: -2.5, lowShelf: 3.0, highPass: 0.80, mid: 0.04 }
  ];

  const state = { ...DEFAULTS };
  const knobCanvases = {};
  const readouts = {};
  let presetIndex = 0;

  /* ------------------------------------------------------------------ */
  /*  Knob drawing                                                       */
  /* ------------------------------------------------------------------ */

  const TAU = Math.PI * 2;
  const START_ANGLE = Math.PI * 1.2;
  const END_ANGLE = Math.PI * 2.8;

  function roundRect(ctx, x, y, w, h, r) {
    const radius = Math.min(r, w / 2, h / 2);
    ctx.beginPath();
    if (typeof ctx.roundRect === 'function') {
      ctx.roundRect(x, y, w, h, radius);
      return;
    }
    ctx.moveTo(x + radius, y);
    ctx.arcTo(x + w, y, x + w, y + h, radius);
    ctx.arcTo(x + w, y + h, x, y + h, radius);
    ctx.arcTo(x, y + h, x, y, radius);
    ctx.arcTo(x, y, x + w, y, radius);
    ctx.closePath();
  }

  function drawKnob(canvas, value) {
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    if (rect.width < 40) return;

    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const size = Math.round(rect.width);
    const px = Math.round(size * dpr);

    if (canvas.width !== px || canvas.height !== px) {
      canvas.width = px;
      canvas.height = px;
    }

    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, size, size);

    const cx = size / 2;
    const cy = size / 2;
    const radius = size * 0.5 - 6;
    const angle = START_ANGLE + (clamp(value, 0, 100) / 100) * (END_ANGLE - START_ANGLE);
    const ring = Math.max(3, radius * 0.105);

    // outer air glow
    const glow = ctx.createRadialGradient(cx, cy, radius * 0.66, cx, cy, radius * 1.18);
    glow.addColorStop(0, 'rgba(56,200,255,0.20)');
    glow.addColorStop(0.55, 'rgba(30,130,255,0.10)');
    glow.addColorStop(1, 'rgba(30,130,255,0)');
    ctx.fillStyle = glow;
    ctx.beginPath();
    ctx.arc(cx, cy, radius * 1.18, 0, TAU);
    ctx.fill();

    // tick ring
    for (let i = 0; i <= 60; i++) {
      const t = i / 60;
      const a = START_ANGLE - 0.12 + t * (END_ANGLE - START_ANGLE + 0.24);
      const major = i % 5 === 0;
      const r1 = radius * (major ? 0.900 : 0.930);
      const r2 = radius * (major ? 0.982 : 0.958);
      const ca = a - Math.PI / 2;
      const passed = t <= clamp(value, 0, 100) / 100;

      ctx.strokeStyle = passed
        ? `rgba(120,225,255,${major ? 0.62 : 0.28})`
        : `rgba(255,255,255,${major ? 0.32 : 0.13})`;
      ctx.lineWidth = major ? 1.3 : 0.8;
      ctx.beginPath();
      ctx.moveTo(cx + Math.cos(ca) * r1, cy + Math.sin(ca) * r1);
      ctx.lineTo(cx + Math.cos(ca) * r2, cy + Math.sin(ca) * r2);
      ctx.stroke();
    }

    // metallic bezel
    const bezel = ctx.createLinearGradient(cx - radius * 0.6, cy - radius * 0.8, cx + radius * 0.5, cy + radius * 0.7);
    bezel.addColorStop(0, '#e6f4ff');
    bezel.addColorStop(0.28, '#8fb6d8');
    bezel.addColorStop(0.55, '#23324a');
    bezel.addColorStop(0.8, '#0a1220');
    bezel.addColorStop(1, '#1c3450');

    ctx.strokeStyle = bezel;
    ctx.lineWidth = ring;
    ctx.beginPath();
    ctx.arc(cx, cy, radius - ring * 0.5, 0, TAU);
    ctx.stroke();

    // glass body
    const body = radius - ring * 1.85;
    const bodyFill = ctx.createRadialGradient(cx - body * 0.35, cy - body * 0.45, body * 0.05, cx, cy, body * 1.05);
    bodyFill.addColorStop(0, '#33496a');
    bodyFill.addColorStop(0.42, '#16233a');
    bodyFill.addColorStop(1, '#02050b');

    ctx.fillStyle = bodyFill;
    ctx.beginPath();
    ctx.arc(cx, cy, body, 0, TAU);
    ctx.fill();

    // glass sheen
    const sheen = ctx.createRadialGradient(cx - body * 0.30, cy - body * 0.42, body * 0.05,
                                           cx - body * 0.05, cy - body * 0.10, body * 0.95);
    sheen.addColorStop(0, 'rgba(255,255,255,0.20)');
    sheen.addColorStop(0.45, 'rgba(255,255,255,0.05)');
    sheen.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = sheen;
    ctx.beginPath();
    ctx.arc(cx, cy, body - 1, 0, TAU);
    ctx.fill();

    ctx.strokeStyle = 'rgba(255,255,255,0.16)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(cx, cy, body, 0, TAU);
    ctx.stroke();

    // value arc with neon bloom
    if (Math.abs(angle - START_ANGLE) > 0.002) {
      const arcRadius = radius * 0.855;
      ctx.save();
      ctx.shadowColor = 'rgba(56,200,255,0.9)';
      ctx.shadowBlur = 16;
      ctx.strokeStyle = 'rgba(56,200,255,0.85)';
      ctx.lineWidth = Math.max(2, ring * 0.72);
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.arc(cx, cy, arcRadius, START_ANGLE - Math.PI / 2, angle - Math.PI / 2);
      ctx.stroke();
      ctx.restore();

      ctx.save();
      ctx.shadowColor = 'rgba(159,232,255,0.9)';
      ctx.shadowBlur = 6;
      ctx.strokeStyle = 'rgba(226,250,255,0.95)';
      ctx.lineWidth = Math.max(1.2, ring * 0.26);
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.arc(cx, cy, arcRadius, START_ANGLE - Math.PI / 2, angle - Math.PI / 2);
      ctx.stroke();
      ctx.restore();
    }

    // pointer
    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(angle);
    const pointerWidth = Math.max(2.4, radius * 0.055);
    const pointerLength = radius * 0.54;
    ctx.shadowColor = 'rgba(56,200,255,0.8)';
    ctx.shadowBlur = 10;
    ctx.fillStyle = '#f4fcff';
    roundRect(ctx, -pointerWidth / 2, -radius * 0.66, pointerWidth, pointerLength, pointerWidth / 2);
    ctx.fill();
    ctx.restore();

    // hub
    const hub = radius * 0.155;
    const hubFill = ctx.createRadialGradient(cx - hub * 0.4, cy - hub * 0.4, hub * 0.1, cx, cy, hub * 1.1);
    hubFill.addColorStop(0, '#41618a');
    hubFill.addColorStop(0.6, '#0c1524');
    hubFill.addColorStop(1, '#02050b');
    ctx.fillStyle = hubFill;
    ctx.beginPath();
    ctx.arc(cx, cy, hub, 0, TAU);
    ctx.fill();

    ctx.strokeStyle = 'rgba(56,200,255,0.85)';
    ctx.lineWidth = 1.2;
    ctx.beginPath();
    ctx.arc(cx, cy, hub * 0.78, 0, TAU);
    ctx.stroke();

    ctx.fillStyle = 'rgba(120,225,255,0.55)';
    ctx.beginPath();
    ctx.arc(cx, cy, hub * 0.26, 0, TAU);
    ctx.fill();
  }

  function renderAllKnobs() {
    for (const key of Object.keys(knobCanvases)) {
      drawKnob(knobCanvases[key], state[key]);
    }
  }

  function updateReadout(key) {
    const el = readouts[key];
    if (el) el.textContent = `${Math.round(state[key])}%`;

    const canvas = knobCanvases[key];
    if (canvas) {
      canvas.setAttribute('aria-valuenow', String(Math.round(state[key])));
      canvas.setAttribute('aria-valuetext', `${Math.round(state[key])} percent`);
    }
  }

  /* ------------------------------------------------------------------ */
  /*  Audio engine                                                       */
  /* ------------------------------------------------------------------ */

  class VocaMaxEngine {
    constructor() {
      this.ctx = null;
      this.nodes = null;
      this.source = null;
      this.sourceKind = null;
      this.micStream = null;
      this.demoBuffer = null;
      this.inputAnalyser = null;
      this.outputAnalyser = null;
      this.timeData = new Float32Array(2048);
    }

    async ensure() {
      if (!this.ctx) {
        const Ctx = window.AudioContext || window.webkitAudioContext;
        if (!Ctx) throw new Error('This browser does not support the Web Audio API.');
        this.ctx = new Ctx();
        this.buildGraph();
      }
      if (this.ctx.state === 'suspended') await this.ctx.resume();
      return this.ctx;
    }

    buildGraph() {
      const ctx = this.ctx;

      const input = ctx.createGain();
      input.channelCount = 2;
      input.channelCountMode = 'explicit';
      input.channelInterpretation = 'speakers';

      const lowShelf = ctx.createBiquadFilter();
      lowShelf.type = 'lowshelf';
      lowShelf.frequency.value = 130;

      const highPass = ctx.createBiquadFilter();
      highPass.type = 'highpass';
      highPass.frequency.value = 20;
      highPass.Q.value = 0.707;

      const presence = ctx.createBiquadFilter();
      presence.type = 'peaking';
      presence.frequency.value = 3200;
      presence.Q.value = 0.9;

      const air = ctx.createBiquadFilter();
      air.type = 'highshelf';
      air.frequency.value = 9000;

      input.connect(lowShelf);
      lowShelf.connect(highPass);
      highPass.connect(presence);
      presence.connect(air);

      // Mid / side matrix
      const split = ctx.createChannelSplitter(2);
      air.connect(split);

      const mid = ctx.createGain();
      const side = ctx.createGain();

      const lMid = ctx.createGain(); lMid.gain.value = 0.5;
      const rMid = ctx.createGain(); rMid.gain.value = 0.5;
      const lSide = ctx.createGain(); lSide.gain.value = 0.5;
      const rSide = ctx.createGain(); rSide.gain.value = -0.5;

      split.connect(lMid, 0);
      split.connect(rMid, 1);
      split.connect(lSide, 0);
      split.connect(rSide, 1);

      lMid.connect(mid);
      rMid.connect(mid);
      lSide.connect(side);
      rSide.connect(side);

      const sideHighPass = ctx.createBiquadFilter();
      sideHighPass.type = 'highpass';
      sideHighPass.frequency.value = 90;
      sideHighPass.Q.value = 0.707;
      side.connect(sideHighPass);

      const sideWidth = ctx.createGain();
      const sideWidthInverted = ctx.createGain();
      sideHighPass.connect(sideWidth);
      sideHighPass.connect(sideWidthInverted);

      const midFocus = ctx.createGain();
      mid.connect(midFocus);

      const merger = ctx.createChannelMerger(2);
      midFocus.connect(merger, 0, 0);
      sideWidth.connect(merger, 0, 0);
      midFocus.connect(merger, 0, 1);
      sideWidthInverted.connect(merger, 0, 1);

      // Power is a bypass depth, exactly like the plugin (no comb filtering).
      const wet = ctx.createGain();
      const dry = ctx.createGain();
      merger.connect(wet);
      input.connect(dry);

      const master = ctx.createGain();
      master.gain.value = 0.9;
      wet.connect(master);
      dry.connect(master);

      const inputAnalyser = ctx.createAnalyser();
      inputAnalyser.fftSize = 2048;
      input.connect(inputAnalyser);

      const outputAnalyser = ctx.createAnalyser();
      outputAnalyser.fftSize = 2048;
      master.connect(outputAnalyser);
      outputAnalyser.connect(ctx.destination);

      this.nodes = {
        input, lowShelf, highPass, presence, air,
        sideHighPass, sideWidth, sideWidthInverted, midFocus,
        wet, dry, master
      };
      this.inputAnalyser = inputAnalyser;
      this.outputAnalyser = outputAnalyser;
    }

    setParams(params) {
      if (!this.ctx || !this.nodes) return;

      const now = this.ctx.currentTime;
      const time = 0.025;
      const n = this.nodes;

      const power = params.power / 100;
      const space = params.space / 100;
      const lowAmt = (params.low / 100) * power;
      const highKnob = params.high / 100;
      const focus = FOCUS[params.focus] || FOCUS[1];

      const spaceWidth = space <= 0.5
        ? 0.15 + 0.85 * (space / 0.5)
        : 1 + (space - 0.5) / 0.5;

      const width = Math.min(2.2, 1 + (spaceWidth * focus.width - 1) * power);
      const midGain = 1 + ((focus.mid + (1 - space) * 0.10) * power);
      const highPassFreq = Math.min(20 * Math.pow(12, lowAmt) * focus.highPass, this.ctx.sampleRate * 0.45);
      const lowShelfDb = -3.5 * lowAmt + focus.lowShelf * power;
      const presenceDb = focus.presence * power;
      const airDb = ((highKnob - 0.5) * 16 + focus.air) * power;
      const sideCutoff = 60 + 160 * lowAmt;
      const bypass = Math.min(1, power * 20);

      const set = (param, value) => {
        if (typeof param.setTargetAtTime === 'function') param.setTargetAtTime(value, now, time);
        else param.value = value;
      };

      set(n.lowShelf.gain, lowShelfDb);
      set(n.highPass.frequency, highPassFreq);
      set(n.presence.gain, presenceDb);
      set(n.air.gain, airDb);
      set(n.sideHighPass.frequency, sideCutoff);
      set(n.sideWidth.gain, width);
      set(n.sideWidthInverted.gain, -width);
      set(n.midFocus.gain, midGain);
      set(n.wet.gain, bypass);
      set(n.dry.gain, 1 - bypass);
    }

    readLevel(which) {
      const analyser = which === 'input' ? this.inputAnalyser : this.outputAnalyser;
      if (!analyser) return 0;

      analyser.getFloatTimeDomainData(this.timeData);
      let sum = 0;
      for (let i = 0; i < this.timeData.length; i++) {
        const v = this.timeData[i];
        sum += v * v;
      }
      const rms = Math.sqrt(sum / this.timeData.length);
      if (rms <= 1e-7) return 0;
      const db = 20 * Math.log10(rms);
      return clamp((db + 60) / 60, 0, 1);
    }

    stopSources() {
      if (this.source) {
        try { this.source.stop(); } catch (err) { /* already stopped */ }
        try { this.source.disconnect(); } catch (err) { /* already disconnected */ }
        this.source = null;
      }
      if (this.micStream) {
        this.micStream.getTracks().forEach((track) => track.stop());
        this.micStream = null;
      }
      this.sourceKind = null;
    }

    playBuffer(buffer, kind) {
      this.stopSources();
      const source = this.ctx.createBufferSource();
      source.buffer = buffer;
      source.loop = true;
      source.connect(this.nodes.input);
      source.start();
      this.source = source;
      this.sourceKind = kind;
    }

    async playDemo() {
      await this.ensure();
      if (!this.demoBuffer) this.demoBuffer = createDemoBuffer(this.ctx);
      this.playBuffer(this.demoBuffer, 'demo');
      this.setParams(state);
      return 'Demo loop playing - move SPACE to hear the vocal sit differently.';
    }

    async startMic() {
      await this.ensure();

      if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
        throw new Error('Microphone capture is not available in this browser.');
      }

      this.stopSources();

      const stream = await navigator.mediaDevices.getUserMedia({
        audio: {
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false
        },
        video: false
      });

      const source = this.ctx.createMediaStreamSource(stream);
      source.connect(this.nodes.input);

      this.source = source;
      this.sourceKind = 'mic';
      this.micStream = stream;
      this.setParams(state);
      return 'Microphone live - use headphones to avoid feedback.';
    }

    async playFile(file) {
      await this.ensure();

      const data = await file.arrayBuffer();
      const buffer = await new Promise((resolve, reject) => {
        const result = this.ctx.decodeAudioData(data, resolve, reject);
        if (result && typeof result.then === 'function') result.then(resolve, reject);
      });

      this.playBuffer(buffer, 'file');
      this.setParams(state);
      return `Playing "${file.name}" - drag the knobs to hear the change.`;
    }
  }

  /* ------------------------------------------------------------------ */
  /*  Demo loop (procedural stereo audio, seamless 8 s loop)             */
  /* ------------------------------------------------------------------ */

  function createDemoBuffer(ctx) {
    const sr = ctx.sampleRate;
    const duration = 8;
    const length = Math.floor(sr * duration);
    const buffer = ctx.createBuffer(2, length, sr);
    const left = buffer.getChannelData(0);
    const right = buffer.getChannelData(1);

    const snap = (frequency) => Math.round(frequency * duration) / duration;

    const pad = [
      { f: snap(110), pan: 0.00, amp: 0.075, phase: 0.0 },
      { f: snap(130.8125), pan: -0.45, amp: 0.055, phase: 1.1 },
      { f: snap(164.8125), pan: 0.45, amp: 0.050, phase: 2.3 },
      { f: snap(220), pan: -0.20, amp: 0.040, phase: 0.7 },
      { f: snap(329.625), pan: 0.30, amp: 0.026, phase: 3.1 }
    ];

    const melody = [
      { start: 0.00, dur: 1.05, note: 220.0 },
      { start: 1.25, dur: 1.00, note: 261.625 },
      { start: 2.50, dur: 1.35, note: 329.625 },
      { start: 4.10, dur: 0.90, note: 293.625 },
      { start: 5.30, dur: 1.60, note: 220.0 },
      { start: 7.10, dur: 0.70, note: 164.8125 }
    ];

    const kicks = [0.0, 2.0, 4.0, 6.0];

    const formantGain = (f) => {
      const g1 = Math.exp(-Math.pow((f - 700) / 260, 2)) * 1.10;
      const g2 = Math.exp(-Math.pow((f - 1250) / 420, 2)) * 0.85;
      const g3 = Math.exp(-Math.pow((f - 2700) / 700, 2)) * 0.35;
      return 0.35 + g1 + g2 + g3;
    };

    // Precompute the vocal note harmonics.
    const melodies = melody.map((note) => {
      const harmonics = [];
      let energy = 0;
      for (let h = 1; h <= 12; h++) {
        const freq = note.note * h;
        if (freq > sr * 0.45) break;
        const amp = formantGain(freq) / h;
        energy += amp;
        harmonics.push({ n: h, amp, phase: (h * 0.73) % 1 * TAU });
      }
      const scale = energy > 0 ? 0.085 / energy : 0;
      return { ...note, harmonics: harmonics.map((h) => ({ ...h, amp: h.amp * scale })) };
    });

    for (let i = 0; i < length; i++) {
      const t = i / sr;

      let l = 0;
      let r = 0;

      // Warm pad, stereo spread, slow breathing.
      for (const partial of pad) {
        const sway = 0.72 + 0.28 * Math.sin(TAU * t / 8 + partial.phase);
        const sample = Math.sin(TAU * partial.f * t + partial.phase) * partial.amp * sway;
        const panAngle = (partial.pan + 1) * Math.PI * 0.25;
        l += sample * Math.cos(panAngle);
        r += sample * Math.sin(panAngle);
      }

      // Vocal-ish centre melody with vibrato and formants.
      for (const note of melodies) {
        if (t < note.start || t > note.start + note.dur) continue;

        const local = t - note.start;
        const attack = Math.min(1, local / 0.08);
        const release = Math.min(1, (note.dur - local) / 0.22);
        const envelope = Math.max(0, Math.min(attack, release));
        if (envelope <= 0) continue;

        const vibrato = 1 + 0.0045 * Math.sin(TAU * 5.2 * local);
        let sample = 0;

        for (const harmonic of note.harmonics) {
          sample += harmonic.amp *
            Math.sin(TAU * note.note * harmonic.n * local * vibrato + harmonic.phase);
        }

        sample *= envelope;
        l += sample;
        r += sample;
      }

      // Soft heartbeat kick.
      for (const beat of kicks) {
        let local = t - beat;
        if (local < 0) local += duration;
        if (local > 0.55) continue;
        const envelope = Math.exp(-local * 9.5);
        const sample = Math.sin(TAU * 52 * local * (1 - local * 0.4)) * 0.075 * envelope;
        l += sample;
        r += sample;
      }

      // Airy noise bed (zero at the loop edges).
      const noiseEnvelope = Math.sin(Math.PI * t / duration);
      const noiseL = (Math.sin(i * 12.9898) * 43758.5453) % 1;
      const noiseR = (Math.sin(i * 78.233) * 12345.6789) % 1;
      l += noiseL * 0.012 * noiseEnvelope;
      r += noiseR * 0.012 * noiseEnvelope;

      // Gentle soft clip to keep the loop clean.
      left[i] = Math.tanh(l * 1.1) * 0.95;
      right[i] = Math.tanh(r * 1.1) * 0.95;
    }

    return buffer;
  }

  /* ------------------------------------------------------------------ */
  /*  UI wiring                                                          */
  /* ------------------------------------------------------------------ */

  const engine = new VocaMaxEngine();
  const statusEl = document.getElementById('audioStatus');
  const statusDot = document.getElementById('statusDot');
  const demoBtn = document.getElementById('demoBtn');
  const micBtn = document.getElementById('micBtn');
  const stopBtn = document.getElementById('stopBtn');
  const fileInput = document.getElementById('fileInput');
  const presetSelect = document.getElementById('presetSelect');
  const focusButtons = Array.from(document.querySelectorAll('.focus-btn'));
  const modal = document.getElementById('modal');
  const modalTitle = document.getElementById('modalTitle');
  const modalBody = document.getElementById('modalBody');
  const modalClose = document.getElementById('modalClose');

  function setStatus(message, live) {
    if (statusEl) statusEl.textContent = message;
    if (statusDot) statusDot.classList.toggle('live', Boolean(live));
  }

  function setActiveButton(active) {
    if (!demoBtn || !micBtn) return;
    demoBtn.classList.toggle('active', active === 'demo');
    micBtn.classList.toggle('active', active === 'mic');
    demoBtn.textContent = active === 'demo' ? 'Demo playing' : 'Play demo loop';
    micBtn.textContent = active === 'mic' ? 'Mic live' : 'Microphone';
  }

  function updateFocusUI() {
    focusButtons.forEach((button) => {
      const active = Number(button.dataset.focus) === state.focus;
      button.classList.toggle('active', active);
      button.setAttribute('aria-pressed', active ? 'true' : 'false');
    });
  }

  function commit() {
    engine.setParams(state);
  }

  function setKnob(key, value) {
    state[key] = Math.round(clamp(value, 0, 100) * 10) / 10;
    updateReadout(key);
    drawKnob(knobCanvases[key], state[key]);
    commit();
  }

  function applyPreset(index) {
    const preset = PRESETS[clamp(index, 0, PRESETS.length - 1)];
    presetIndex = clamp(index, 0, PRESETS.length - 1);

    state.power = preset.power;
    state.space = preset.space;
    state.low = preset.low;
    state.high = preset.high;
    state.focus = preset.focus;

    if (presetSelect) presetSelect.selectedIndex = presetIndex;

    Object.keys(knobCanvases).forEach((key) => {
      updateReadout(key);
      drawKnob(knobCanvases[key], state[key]);
    });

    updateFocusUI();
    commit();
  }

  function bindKnob(canvas) {
    const key = canvas.dataset.knob;
    let drag = null;

    canvas.addEventListener('pointerdown', (event) => {
      event.preventDefault();
      canvas.setPointerCapture(event.pointerId);
      drag = { y: event.clientY, value: state[key] };
      canvas.focus({ preventScroll: true });
    });

    canvas.addEventListener('pointermove', (event) => {
      if (!drag) return;
      const sensitivity = event.shiftKey ? 0.11 : 0.42;
      const delta = (drag.y - event.clientY) * sensitivity;
      setKnob(key, drag.value + delta);
    });

    const endDrag = (event) => {
      if (!drag) return;
      drag = null;
      if (canvas.hasPointerCapture && canvas.hasPointerCapture(event.pointerId)) {
        canvas.releasePointerCapture(event.pointerId);
      }
    };

    canvas.addEventListener('pointerup', endDrag);
    canvas.addEventListener('pointercancel', endDrag);

    canvas.addEventListener('wheel', (event) => {
      event.preventDefault();
      const step = event.shiftKey ? 0.5 : 2;
      setKnob(key, state[key] + (event.deltaY < 0 ? step : -step));
    }, { passive: false });

    canvas.addEventListener('dblclick', () => setKnob(key, DEFAULTS[key]));

    canvas.addEventListener('keydown', (event) => {
      const step = event.shiftKey ? 5 : 1;
      let handled = true;

      switch (event.key) {
        case 'ArrowUp':
        case 'ArrowRight': setKnob(key, state[key] + step); break;
        case 'ArrowDown':
        case 'ArrowLeft': setKnob(key, state[key] - step); break;
        case 'PageUp': setKnob(key, state[key] + 10); break;
        case 'PageDown': setKnob(key, state[key] - 10); break;
        case 'Home': setKnob(key, 0); break;
        case 'End': setKnob(key, 100); break;
        default: handled = false;
      }

      if (handled) event.preventDefault();
    });
  }

  /* ---------------- meters ---------------- */

  const meterState = {
    input: { value: 0, peak: 0 },
    output: { value: 0, peak: 0 }
  };

  const inputMeterCanvas = document.getElementById('inputMeter');
  const outputMeterCanvas = document.getElementById('outputMeter');

  function drawMeter(canvas, level, hold) {
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    if (rect.width < 3) return;

    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const w = Math.max(1, Math.round(rect.width));
    const h = Math.max(1, Math.round(rect.height));
    const px = Math.round(w * dpr);
    const py = Math.round(h * dpr);

    if (canvas.width !== px || canvas.height !== py) {
      canvas.width = px;
      canvas.height = py;
    }

    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);

    roundRect(ctx, 0.5, 0.5, w - 1, h - 1, 4);
    ctx.fillStyle = 'rgba(5,17,36,0.95)';
    ctx.fill();
    ctx.strokeStyle = 'rgba(255,255,255,0.10)';
    ctx.lineWidth = 1;
    ctx.stroke();

    const ix = 2;
    const iy = 2;
    const iw = w - 4;
    const ih = h - 4;

    if (level > 0.002) {
      const gradient = ctx.createLinearGradient(ix, 0, ix + iw, 0);
      gradient.addColorStop(0, '#1657b0');
      gradient.addColorStop(0.5, '#2a9dff');
      gradient.addColorStop(0.85, '#8fe3ff');
      gradient.addColorStop(1, '#ffffff');

      ctx.save();
      roundRect(ctx, ix, iy, Math.max(2, iw * level), ih, 2.5);
      ctx.clip();
      ctx.fillStyle = gradient;
      ctx.fillRect(ix, iy, iw, ih);
      ctx.restore();
    }

    ctx.fillStyle = 'rgba(2,7,16,0.55)';
    const step = Math.max(5, iw / 42);
    for (let x = ix + step; x < ix + iw; x += step) {
      ctx.fillRect(x, iy, 1, ih);
    }

    if (hold > 0.002) {
      ctx.fillStyle = 'rgba(255,255,255,0.85)';
      ctx.fillRect(ix + iw * hold - 1, iy, 2, ih);
    }
  }

  function updateMeters() {
    const input = engine.readLevel('input');
    const output = engine.readLevel('output');

    for (const [key, level] of [['input', input], ['output', output]]) {
      const meter = meterState[key];
      meter.value = Math.max(level, meter.value * 0.88);
      meter.peak = Math.max(level, meter.peak - 0.004);
      if (meter.peak < 0.002) meter.peak = 0;
    }

    drawMeter(inputMeterCanvas, meterState.input.value, meterState.input.peak);
    drawMeter(outputMeterCanvas, meterState.output.value, meterState.output.peak);
  }

  function animate() {
    updateMeters();
    requestAnimationFrame(animate);
  }

  /* ---------------- modal ---------------- */

  let lastFocused = null;

  function openModal(title, html) {
    lastFocused = document.activeElement;
    modalTitle.textContent = title;
    modalBody.innerHTML = html;
    modal.hidden = false;
    modalClose.focus();
  }

  function closeModal() {
    modal.hidden = true;
    if (lastFocused && typeof lastFocused.focus === 'function') lastFocused.focus();
  }

  const HELP_HTML = `
    <h3>Power</h3>
    <p><strong>Centre knob, overall control.</strong> Scales the whole chain from 0% (true bypass) to 100% (full processing).</p>
    <h3>Space</h3>
    <p>How much room sits between the vocal and everything else. <strong>Less space</strong> is tight and forward. <strong>More space</strong> is wide and open. The low end stays centred.</p>
    <h3>Low Frequency</h3>
    <p>Left knob: tighten or remove rumble and mud.</p>
    <h3>High Frequency</h3>
    <p>Right knob: brighten or remove top end.</p>
    <h3>Vocal Focus Mode</h3>
    <p>Natural, Clear, Wide and Deep add a fixed character on top of the four knobs.</p>
    <h3>Shortcuts</h3>
    <p>Drag a knob vertically. Hold <strong>Shift</strong> for fine control. Double-click a knob to reload the current preset. Arrow keys work when a knob is focused.</p>
    <p>In the VST3 every control is automatable and saved with your project.</p>`;

  function settingsHtml() {
    const rate = engine.ctx ? `${Math.round(engine.ctx.sampleRate)} Hz` : 'starts with audio';
    const channels = engine.ctx ? `${engine.ctx.destination.channelCount}` : '2';
    return `
      <h3>Engine</h3>
      <p>Web preview of the VocaMax Space 2 engine. The VST3 runs the same control mapping in your DAW.</p>
      <p>Sample rate: <strong>${rate}</strong><br>Output channels: <strong>${channels}</strong><br>Added latency: <strong>0 samples</strong><br>Processing: <strong>32-bit float</strong></p>
      <h3>Version</h3>
      <p>VocaMax Space 2 &middot; v2.0.0<br>By Mark33s Beats Lab</p>
      <h3>Compatibility</h3>
      <p>VST3 for Windows 10 and 11 (64-bit). Works with FL Studio, Ableton Live, Cubase, Studio One, REAPER, Pro Tools, Bitwig and any other VST3 host.</p>`;
  }

  /* ---------------- save ---------------- */

  function savePreset() {
    const preset = PRESETS[presetIndex] || PRESETS[0];
    const payload = {
      plugin: 'VocaMax Space 2',
      vendor: 'Mark33s Beats Lab',
      version: '2.0.0',
      format: 'VocaMax Space 2 settings',
      preset: preset.name,
      parameters: {
        power: state.power,
        space: state.space,
        low: state.low,
        high: state.high,
        focus: ['Natural', 'Clear', 'Wide', 'Deep'][state.focus]
      },
      saved: new Date().toISOString()
    };

    const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `VocaMax Space 2 - ${preset.name}.vmx2.json`;
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);

    setStatus(`Saved "${preset.name}" settings to your downloads.`, Boolean(engine.sourceKind));
  }

  /* ---------------- init ---------------- */

  function init() {
    document.querySelectorAll('.knob-canvas').forEach((canvas) => {
      const key = canvas.dataset.knob;
      knobCanvases[key] = canvas;
      readouts[key] = document.querySelector(`[data-readout="${key}"]`);
      bindKnob(canvas);
    });

    renderAllKnobs();
    Object.keys(readouts).forEach(updateReadout);
    updateFocusUI();
    engine.setParams(state);
    setStatus('Preview ready - start the demo loop, load a track or use your microphone.', false);

    focusButtons.forEach((button) => {
      button.addEventListener('click', () => {
        state.focus = Number(button.dataset.focus) || 0;
        updateFocusUI();
        commit();
      });
    });

    if (presetSelect) {
      presetSelect.addEventListener('change', () => applyPreset(presetSelect.selectedIndex));
    }

    const prev = document.getElementById('presetPrev');
    const next = document.getElementById('presetNext');
    if (prev) prev.addEventListener('click', () => applyPreset((presetIndex + PRESETS.length - 1) % PRESETS.length));
    if (next) next.addEventListener('click', () => applyPreset((presetIndex + 1) % PRESETS.length));

    const saveBtn = document.getElementById('savePreset');
    if (saveBtn) saveBtn.addEventListener('click', savePreset);

    const settingsBtn = document.getElementById('settingsBtn');
    if (settingsBtn) settingsBtn.addEventListener('click', () => openModal('Settings', settingsHtml()));

    const helpBtn = document.getElementById('helpBtn');
    if (helpBtn) helpBtn.addEventListener('click', () => openModal('How to use VocaMax Space 2', HELP_HTML));

    if (modalClose) modalClose.addEventListener('click', closeModal);
    if (modal) {
      modal.addEventListener('click', (event) => {
        if (event.target === modal) closeModal();
      });
    }

    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && modal && !modal.hidden) closeModal();
    });

    if (demoBtn) {
      demoBtn.addEventListener('click', async () => {
        try {
          demoBtn.disabled = true;
          setStatus('Building the demo loop...', false);
          const message = await engine.playDemo();
          setActiveButton('demo');
          setStatus(message, true);
        } catch (error) {
          setStatus(`Could not start audio: ${error.message}`, false);
        } finally {
          demoBtn.disabled = false;
        }
      });
    }

    if (micBtn) {
      micBtn.addEventListener('click', async () => {
        try {
          setStatus('Requesting microphone access...', false);
          const message = await engine.startMic();
          setActiveButton('mic');
          setStatus(message, true);
        } catch (error) {
          const denied = error && (error.name === 'NotAllowedError' || error.name === 'SecurityError');
          setStatus(denied
            ? 'Microphone permission was blocked - allow it in your browser or load an audio file instead.'
            : `Microphone error: ${error.message}`, false);
        }
      });
    }

    if (fileInput) {
      fileInput.addEventListener('change', async () => {
        const file = fileInput.files && fileInput.files[0];
        if (!file) return;

        try {
          setStatus(`Loading "${file.name}"...`, false);
          const message = await engine.playFile(file);
          setActiveButton(null);
          setStatus(message, true);
        } catch (error) {
          setStatus(`Could not decode that file: ${error.message}`, false);
        } finally {
          fileInput.value = '';
        }
      });
    }

    if (stopBtn) {
      stopBtn.addEventListener('click', () => {
        engine.stopSources();
        setActiveButton(null);
        setStatus('Audio stopped.', false);
      });
    }

    const redraw = () => {
      renderAllKnobs();
      drawMeter(inputMeterCanvas, meterState.input.value, meterState.input.peak);
      drawMeter(outputMeterCanvas, meterState.output.value, meterState.output.peak);
    };

    window.addEventListener('resize', redraw);

    if (typeof ResizeObserver === 'function') {
      const observer = new ResizeObserver(redraw);
      const knobs = document.querySelector('.knobs');
      if (knobs) observer.observe(knobs);
    }

    document.addEventListener('visibilitychange', () => {
      if (!engine.ctx) return;
      if (document.hidden) {
        engine.ctx.suspend();
      } else if (engine.sourceKind) {
        engine.ctx.resume();
      }
    });

    requestAnimationFrame(animate);
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
