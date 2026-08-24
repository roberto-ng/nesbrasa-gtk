# Nesbrasa
## Emulador de NES e Famicom

Nesbrasa é organizado como um monorepo: `core` contém a biblioteca de
emulação e `frontends` contém as aplicações que apresentam e controlam o
emulador.

### Dependências:
* Compilador para C++17
* Meson Build System
* Ninja
* Gtkmm >= 3.24.1

### Instruções para compilação:

```
meson _build
cd _build
ninja
```

O executável GTK será criado em `_build/frontends/gtk/nesbrasa-gtk`.

### Web

A versão web reutiliza o mesmo núcleo C++ compilado para WebAssembly. É
necessário ter o Emscripten instalado e disponível no `PATH`:

```
cd frontends/web
npm run build:wasm
npm run serve
```

Depois, abra <http://localhost:8080>.

## Screenshots

### nestest.nes
![alt text1][nestest]

### Star Gate
![alt text1][star]

### Ice Climber
![alt text1][ice]

[nestest]: screenshots/nestest.png "nestest.nes"
[star]: screenshots/star_gate.png "Star Gate"
[ice]: screenshots/ice_climber.png "Ice Climber"
