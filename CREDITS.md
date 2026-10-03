# Credits

This project grew from an RX 6600 XT focused variant of an existing AMD Universal Fix.

## Original Work

- Joao Lucas / Megadroidgames: original proxy DLL work and FH201 / FH205 compatibility research.
- JuniorD-Isael: AMD Universal Fix fork, D3D12 / DXGI proxy maintenance, and VRAM compatibility work.

## Earlier RX 6600 XT Variant

This variant was prepared and tested for an MSI AMD Radeon RX 6600 XT 8 GB system where Forza Horizon 6 reported only about 6 GB of available VRAM.

The goal is narrow:

- keep the original proxy structure
- report 8 GB for RX 6600 XT instead of 4 GB
- package the result in a way regular Vortex / Xbox app users can install and roll back

## License Preservation

The original project is MIT licensed. The MIT license is included unchanged in this repository and in release packages.

## Experimental AMD Update

The v0.3.1-rc1 candidate replaces the fixed memory values with a correction based on the dedicated capacity of an unambiguously matching AMD adapter. D3D12 and DXGI forward to the genuine Windows libraries. The AGS declarations come from AMD's AGS SDK under its included MIT license.

This candidate is prepared by the project maintainer with Codex assistance. Its local API and installer checks are separate from the pending clean Forza test. Broader AMD compatibility remains experimental.

