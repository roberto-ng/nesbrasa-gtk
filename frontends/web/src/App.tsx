import { For, Show, createSignal, onCleanup, onMount } from 'solid-js';

type ControlId = 'a' | 'b' | 'select' | 'start' | 'up' | 'down' | 'left' | 'right';
type View = 'play' | 'settings';

interface WasmModule {
  HEAPU32: Uint32Array;
  WebNes: new () => WasmNes;
}

interface WasmNes {
  carregar_rom(data: Uint8Array): void;
  avancar_quadro(): number;
  framebuffer_ptr(): number;
  programa_carregado(): boolean;
  set_botao(button: number, pressed: boolean): void;
}

interface Control {
  id: ControlId;
  label: string;
  button: number;
}

const controls: Control[] = [
  { id: 'a', label: 'A', button: 0 },
  { id: 'b', label: 'B', button: 1 },
  { id: 'select', label: 'Select', button: 2 },
  { id: 'start', label: 'Start', button: 3 },
  { id: 'up', label: 'Up', button: 4 },
  { id: 'down', label: 'Down', button: 5 },
  { id: 'left', label: 'Left', button: 6 },
  { id: 'right', label: 'Right', button: 7 },
];

const defaults: Record<ControlId, string> = {
  a: 'KeyZ', b: 'KeyX', select: 'Backspace', start: 'Enter',
  up: 'ArrowUp', down: 'ArrowDown', left: 'ArrowLeft', right: 'ArrowRight',
};

const STORAGE_KEY = 'nesbrasa.controls.v1';

function loadControls(): Record<ControlId, string> {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) ?? '{}') as Partial<Record<ControlId, string>>;
    return Object.fromEntries(controls.map((control) => [control.id, saved[control.id] ?? defaults[control.id]])) as Record<ControlId, string>;
  } catch {
    return { ...defaults };
  }
}

function keyLabel(code: string): string {
  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  if (code === ' ') return 'Space';
  return code;
}

export default function App() {
  let canvas: HTMLCanvasElement | undefined;
  let screenWrap: HTMLElement | undefined;
  const [view, setView] = createSignal<View>('play');
  const [status, setStatus] = createSignal('No ROM loaded');
  const [paused, setPaused] = createSignal(false);
  const [controlsState, setControlsState] = createSignal(loadControls());
  const [capturing, setCapturing] = createSignal<ControlId>();
  const [emulator, setEmulator] = createSignal<WasmNes>();
  const [wasm, setWasm] = createSignal<WasmModule>();
  const [fullscreen, setFullscreen] = createSignal(false);
  let frameRequest = 0;

  const persistControls = (next: Record<ControlId, string>) => {
    setControlsState(next);
    localStorage.setItem(STORAGE_KEY, JSON.stringify(next));
  };

  const assignKey = (id: ControlId, code: string) => {
    const next = { ...controlsState() };
    for (const control of controls) {
      if (control.id !== id && next[control.id] === code) next[control.id] = '';
    }
    next[id] = code;
    persistControls(next);
    setCapturing();
  };

  const handleKey = (event: KeyboardEvent, pressed: boolean) => {
    const selected = capturing();
    if (selected) {
      event.preventDefault();
      if (pressed && !event.repeat) {
        if (event.key === 'Escape') setCapturing();
        else assignKey(selected, event.code);
      }
      return;
    }
    if (view() !== 'play' || event.repeat) return;
    const control = controls.find((item) => controlsState()[item.id] === event.code);
    if (!control || !emulator()) return;
    event.preventDefault();
    emulator()!.set_botao(control.button, pressed);
  };

  const draw = () => {
    const nes = emulator();
    const module = wasm();
    if (!nes || !module || !canvas || paused() || !nes.programa_carregado()) return;
    nes.avancar_quadro();
    const pixels = new Uint32Array(module.HEAPU32.buffer, nes.framebuffer_ptr(), 256 * 240);
    const context = canvas.getContext('2d', { alpha: false })!;
    const image = context.createImageData(256, 240);
    for (let i = 0; i < pixels.length; i += 1) {
      image.data[i * 4] = (pixels[i] >> 16) & 0xff;
      image.data[i * 4 + 1] = (pixels[i] >> 8) & 0xff;
      image.data[i * 4 + 2] = pixels[i] & 0xff;
      image.data[i * 4 + 3] = 255;
    }
    context.putImageData(image, 0, 0);
    frameRequest = requestAnimationFrame(draw);
  };

  const loadRom = async (event: Event) => {
    const file = (event.currentTarget as HTMLInputElement).files?.[0];
    if (!file || !emulator()) return;
    try {
      emulator()!.carregar_rom(new Uint8Array(await file.arrayBuffer()));
      setStatus(`ROM: ${file.name}`);
      setPaused(false);
      cancelAnimationFrame(frameRequest);
      frameRequest = requestAnimationFrame(draw);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : 'Unable to load ROM');
    }
  };

  const toggleFullscreen = async () => {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await screenWrap?.requestFullscreen();
  };

  onMount(async () => {
    const wasmUrl = new URL('/nesbrasa.js', window.location.origin).href;
    const module = (await import(/* @vite-ignore */ wasmUrl)) as unknown as { default: () => Promise<WasmModule> };
    const loaded = await module.default();
    setWasm(loaded);
    setEmulator(new loaded.WebNes());
    const keyDown = (event: KeyboardEvent) => handleKey(event, true);
    const keyUp = (event: KeyboardEvent) => handleKey(event, false);
    window.addEventListener('keydown', keyDown);
    window.addEventListener('keyup', keyUp);
    const fullscreenChanged = () => setFullscreen(document.fullscreenElement === screenWrap);
    document.addEventListener('fullscreenchange', fullscreenChanged);
    onCleanup(() => {
      window.removeEventListener('keydown', keyDown);
      window.removeEventListener('keyup', keyUp);
      document.removeEventListener('fullscreenchange', fullscreenChanged);
      cancelAnimationFrame(frameRequest);
    });
  });

  return (
    <main class="app">
      <header class="toolbar">
        <h1>Nesbrasa</h1>
        <Show when={view() === 'play'}>
          <label class="file-button">Open ROM<input type="file" accept=".nes" onChange={loadRom} /></label>
          <button type="button" disabled={!emulator()?.programa_carregado()} onClick={() => { setPaused(!paused()); if (paused()) frameRequest = requestAnimationFrame(draw); }}>
            {paused() ? 'Resume' : 'Pause'}
          </button>
          <output>{status()}</output>
          <button type="button" onClick={toggleFullscreen}>{fullscreen() ? 'Exit full screen' : 'Full screen'}</button>
        </Show>
        <button type="button" onClick={() => setView(view() === 'play' ? 'settings' : 'play')}>
          {view() === 'play' ? 'Settings' : 'Back to emulator'}
        </button>
      </header>

      <Show when={view() === 'play'} fallback={
        <section class="settings" aria-labelledby="settings-title">
          <h2 id="settings-title">Controls</h2>
          <p>Choose a row, then press the key you want to use.</p>
          <div class="control-list">
            <For each={controls}>{(control) =>
              <div class="control-row">
                <span>{control.label}</span>
                <button type="button" class={capturing() === control.id ? 'capturing' : ''} onClick={() => setCapturing(control.id)}>
                  {capturing() === control.id ? 'Press a key…' : (keyLabel(controlsState()[control.id]) || 'Unassigned')}
                </button>
              </div>
            }</For>
          </div>
          <button type="button" onClick={() => persistControls({ ...defaults })}>Restore defaults</button>
        </section>
      }>
        <section ref={screenWrap} class="screen-wrap" aria-label="NES screen">
          <canvas ref={canvas} width="256" height="240" />
        </section>
        <p class="help">Keyboard controls can be changed in Settings.</p>
      </Show>
    </main>
  );
}
