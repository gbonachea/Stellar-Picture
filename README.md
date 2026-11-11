# Stellar Picture
Visor básico de imágenes para Linux, escrito en C++ y Qt6 Widgets.

## Dependencias (Ubuntu / Linux Mint / Debian)
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-image-formats-plugins
```

## Compilar
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/stellar-picture
```

## Abrir un archivo
```bash
./build/stellar-picture /ruta/a/imagen.png
```

## Instalar en el sistema
```bash
sudo cmake --install build
```

## Atajos
- Ctrl+O: abrir
- Flechas izquierda/derecha: imagen anterior/siguiente
- Rueda o +/-: zoom
- F: ajustar
- 1: tamaño real
- [ y ]: rotar
- F11: pantalla completa; Escape: salir

## Notas
- GIF animados se muestran como imagen fija.
- La rotación es visual y no sobrescribe archivos.
- Los formatos disponibles dependen de los plugins Qt instalados.
