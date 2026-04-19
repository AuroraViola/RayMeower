# RayMeower

Another ray tracing renderer

## Screenshots

<img width="1286" height="1044" alt="Image" src="https://github.com/user-attachments/assets/7d74a164-ed97-4c0b-aec0-1f5cfd49230e" />

## Building

### Linux

#### Install dependencies:
Arch
```
$ sudo pacman -S git gcc cmake sdl3 sdl3_image
```
Fedora
```
$ sudo dnf install git gcc cmake SDL3-devel SDL3_image-devel
```
#### Cloning and building

Clone the repo:
```
$ git clone https://github.com/AuroraViola/RayMeower
$ cd RayMeower
```

Build:
```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

Execute Raymeower:
```
$ ./RayMeower
```

## License

This software is licensed under the [GNU General Public License v3.0](https://github.com/AuroraViola/ChillyGB/blob/main/LICENSE.md)