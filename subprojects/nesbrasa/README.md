# Nesbrasa 
## Emulador de NES e Famicom

Este repositório é o repositório para a biblioteca núcleo do emulador Nesbrasa, que contém apenas as partes do código que são idependentes de sistema operacional.

Repisitório do front-end Gtk para Windows e GNU/Linux: https://gitlab.com/robertonazareth/nesbrasa-gtk

### Dependências:
* Compilador para C++17
* Meson Build System
* Ninja

### Instruções para compilação:

```
meson _build
cd _build
ninja
```