# Nesbrasa
## Emulador de NES e Famicom

Nesbrasa é organizado como um monorepo: `core` contém a biblioteca de
emulação e `frontends` contém as aplicações que apresentam e controlam o
emulador.

### Dependências:
* Compilador para C++17
* CMake >= 3.28
* Ninja
* Gtkmm >= 3.24.1

### Instruções para compilação:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

O executável GTK será criado em `build/frontends/gtk/nesbrasa-gtk`.

Para uma compilação otimizada de release do frontend GTK, use o preset:

```
cmake --preset gtk-release
cmake --build --preset gtk-release
```

O executável será criado em `build/gtk-release/frontends/gtk/nesbrasa-gtk`.

Para executar os testes:

```
ctest --test-dir build --output-on-failure
```

### Web

A versão web reutiliza o mesmo núcleo C++ compilado para WebAssembly. É
necessário ter o Emscripten instalado e disponível no `PATH`:

```
cd frontends/web
npm install
npm run build:wasm
npm run build
npm run serve
```

Depois, abra <http://localhost:4173>.

Para desenvolvimento com Vite, use `npm run dev` e abra
<http://localhost:5173>.

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
