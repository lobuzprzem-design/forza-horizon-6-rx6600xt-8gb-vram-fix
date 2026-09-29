# Credits

This project is a small RX 6600 XT focused variant of an existing AMD Universal Fix.

## Original Work

- Joao Lucas / Megadroidgames: original proxy DLL work and FH201 / FH205 compatibility research.
- JuniorD-Isael: AMD Universal Fix fork, D3D12 / DXGI proxy maintenance, and VRAM compatibility work.

## This Variant

This variant was prepared and tested for an MSI AMD Radeon RX 6600 XT 8 GB system where Forza Horizon 6 reported only about 6 GB of available VRAM.

The goal is narrow:

- keep the original proxy structure
- report 8 GB for RX 6600 XT instead of 4 GB
- package the result in a way regular Vortex / Xbox app users can install and roll back

## License Preservation

The original project is MIT licensed. The MIT license is included unchanged in this repository and in release packages.

