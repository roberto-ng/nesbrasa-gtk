import createNesbrasaModule from './nesbrasa.js?v=4';

const WIDTH = 256;
const HEIGHT = 240;
const BUTTONS = { z: 0, x: 1, Backspace: 2, Enter: 3, ArrowUp: 4, ArrowDown: 5, ArrowLeft: 6, ArrowRight: 7 };
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', { alpha: false });
const image = context.createImageData(WIDTH, HEIGHT);
const status = document.querySelector('#status');
const pause = document.querySelector('#pause');
let emulator;
let running = true;

function draw() {
  if (!emulator?.programa_carregado()) return;
  emulator.avancar_quadro();
  const pixels = new Uint32Array(module.HEAPU32.buffer, emulator.framebuffer_ptr(), WIDTH * HEIGHT);
  for (let i = 0; i < pixels.length; i += 1) {
    const color = pixels[i];
    image.data[i * 4] = (color >> 16) & 0xff;
    image.data[i * 4 + 1] = (color >> 8) & 0xff;
    image.data[i * 4 + 2] = color & 0xff;
    image.data[i * 4 + 3] = 255;
  }
  context.putImageData(image, 0, 0);
  if (running) requestAnimationFrame(draw);
}

const module = await createNesbrasaModule();
emulator = new module.WebNes();

document.querySelector('#rom').addEventListener('change', async (event) => {
  const file = event.target.files[0];
  if (!file) return;
  try {
    emulator.carregar_rom(new Uint8Array(await file.arrayBuffer()));
    status.value = file.name;
    pause.disabled = false;
    running = true;
    requestAnimationFrame(draw);
  } catch (error) {
    status.value = error.message || 'Unable to load ROM';
  }
});

pause.addEventListener('click', () => {
  running = !running;
  pause.textContent = running ? 'Pause' : 'Resume';
  if (running) requestAnimationFrame(draw);
});

window.addEventListener('keydown', (event) => {
  const button = BUTTONS[event.key];
  if (button === undefined || event.repeat) return;
  event.preventDefault();
  emulator.set_botao(button, true);
});

window.addEventListener('keyup', (event) => {
  const button = BUTTONS[event.key];
  if (button === undefined) return;
  event.preventDefault();
  emulator.set_botao(button, false);
});
