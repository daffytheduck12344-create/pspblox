# PSPBlox

PSPBlox is an original, offline block-building homebrew prototype for the **PSP-1000**, including the PSP-1001. It is not a Roblox client or a copy of Roblox assets/services.

## What’s in this first playable build

- A small 16×16 block world rendered with the PSP GU.
- Third-person movement and camera turning.
- Place and remove blocks in front of the player.
- Save/load the world on the Memory Stick.
- PSP homebrew output (`EBOOT.PBP`) for a PSP running compatible custom firmware.

This is a compact starting point rather than a finished commercial-scale game. It deliberately does **not** pad the project to 1 GB: a huge asset payload would waste Memory Stick space, increase loading time, and exceed what this small homebrew needs. The PSP-1000 has 32 MB of main RAM, so worlds and assets need to be designed for that limit. Homebrew is normally installed as `ms0:/PSP/GAME/PSPBlox/EBOOT.PBP`; an ISO is not needed for this format.

## Build

Install a PSPDEV toolchain with PSPSDK, then from the repository root run:

```sh
make
```

The build should produce `EBOOT.PBP`. Copy it to:

```text
ms0:/PSP/GAME/PSPBlox/EBOOT.PBP
```

Run it from the PSP's Game menu. Use a PSP-1001 with compatible homebrew support; this project does not modify firmware or provide custom firmware installation instructions.

## Controls

- Analog stick: move
- L / R: turn the camera/player
- Cross: place a block in front
- Square: remove a block in front
- Select: save
- Start: exit

Save data is written to `ms0:/PSP/SAVEDATA/PSPBLOX/world.dat`.

## Limitations / next steps

This initial prototype uses a flat block world and simple third-person camera. Terrain generation, richer UI, sound, multiplayer, and polished art are not implemented yet. Contributions can extend the game while keeping its content original and PSP-friendly.
