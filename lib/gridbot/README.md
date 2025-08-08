# Grid Trading Bot PoC (C++)
This repository contains a proof-of-concept grid trading bot in C++ with:
- Mock exchange that simulates price moves, partial fills, and slippage
- Grid strategy that reads parameters from `config.json`
- Build scripts (CMake)
- Run helper script `run.sh`

## Build & Run
```bash
mkdir build && cd build
cmake ..
make
./grid_bot ../config.json
```
or run `./run.sh` from repo root.

## Notes
- This is a PoC/simulator only. **Do NOT** use with real funds.
- Config is a JSON file (`config.json`) — change grid parameters without recompiling.
