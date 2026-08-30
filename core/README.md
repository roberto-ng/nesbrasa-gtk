# Nesbrasa core

Biblioteca independente de sistema operacional para emulação de NES e
Famicom. Ela não depende de GTK, Cairo, APIs de navegador ou de um loop de
eventos. Os frontends carregam ROMs, agendam quadros, traduzem entrada e
apresentam o framebuffer.

### Dependências:
* Compilador para C++20
* CMake >= 3.28
* Ninja

### Instruções para compilação:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Run these commands from the repository root. The root CMake project builds
the core library and its tests alongside the GTK frontend.

