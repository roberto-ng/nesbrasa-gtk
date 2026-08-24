# Nesbrasa core

Biblioteca independente de sistema operacional para emulação de NES e
Famicom. Ela não depende de GTK, Cairo, APIs de navegador ou de um loop de
eventos. Os frontends carregam ROMs, agendam quadros, traduzem entrada e
apresentam o framebuffer.

### Dependências:
* Compilador para C++17
* Meson Build System
* Ninja

### Instruções para compilação:

```
meson setup _build .
cd _build
ninja
```

Run these commands from the repository root. The root Meson project builds
the core library and its tests alongside the GTK frontend.
