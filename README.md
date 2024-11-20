# wfb_bind (the wfb-ng bind button)

## Overview

This project is intended to ease the wfb-ng key exchange for novice users. It is designed to follow the KISS (Keep It Simple, Stupid) principle, offering the least amount of configuration possible. Just run the program, and you'll be bound!


## Key Exchange Process

`wfb-bind` uses the existing wfb-ng software to send the `drone.key` to the drone. No additional software is needed. It achieves this by starting its own instances of `wfb_tx` and `wfb_rx` with a hardcoded keypair and sending the key via UDP. The bind process will timeout after 30 seconds.

## Usage

### Ground Station Mode:
Run the following command to start on the Ground Station:
```bash
./wfb_bind gs
```

### Drone Mode:
Run the following command to start on the Drone:
```bash
./wfb_bind drone
```

### Key Generation
If `/etc/gs.key` is missing, the program invokes `wfg_keygen` to create the necessary keys before starting communication.