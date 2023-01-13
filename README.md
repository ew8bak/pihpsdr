# pihpsdr

Fork of John Melton G0ORX's pihpsdr.

# piHPSDR Windows Support
  
### Development environment

  Development and testing has been run on Windows 10 and Windows 11.
  Tools needed for assembly:</br>
    [MSYS2](https://www.msys2.org/)</br>
    [GTK+ for Windows Runtime Environment](https://github.com/tschoonj/GTK-for-Windows-Runtime-Environment-Installer)

### Prerequisites for building (MSYS2 MINGW64)

```
  pacman -S mingw-w64-x86_64-gtk3
  pacman -S mingw-w64-x86_64-toolchain
  pacman -S make
  pacman -S git
  pacman -S mingw-w64-x86_64-pulseaudio
  pacman -S mingw-w64-x86_64-portaudio
```

### piHPSDR requires WDSP to be built and installed (MSYS2 MINGW64 console)

```
  git clone https://github.com/ew8bak/wdsp
  cd wdsp
  make
  make install
```

### To download and compile piHPSDR from https://github.com/ew8bak/pihpsdr (MSYS2 MINGW64 console)

```
  git clone https://github.com/ew8bak/pihpsdr
  cd pihpsdr
  make -f Makefile.win
```
  After successful compilation, the pihpsdr.exe file will be located in the "bin" directory

### Troubleshooting

  - Library not found message when starting program:<br>
      copy to "bin" directory wdsp.dll, libportaudio.dll and libfftw3-3.dll
  - The program starts, but there is no panadapter and waterfall:<br>
      add "pihpsdr.exe" to Windows Firewall exceptions. Allow incoming and outgoing UDP/TCP packets in Firewall

## Original readme
Raspberry Pi 3/4 standalone code for HPSDR

Supports both the old and new ethernet protocols.

See the Wiki (https://github.com/g0orx/pihpsdr/wiki) for more information about building and running piHPSDR.

Note: The latest source now code has the gpiod branch merged in and also reuqires the latest version of wdsp.

