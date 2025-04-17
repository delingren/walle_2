# Wall-E 2.0

I bought this Wall-E toy made by Thinkway on eBay. Actually I bought a few, all partially  broken. The toy is pretty primitive. It can make some basic moves. But the treads cannot be independently controlled. The arms can go up and down a little too, but not independently. The head can turn. The eyes can tilt a little bit. It only has one motor and it's smartly connected to all those parts with a gearbox to make them move in some ways. But in general, there isn't a high degree of maneuverability. It also has a speaker to make some sounds. When new, it came with an IR remote control. None of the ones I bought still had it.

![wall-e](./media/IMG_1013.jpeg)

My plan is to control it with a microcontroller. I already made a v1 but didn't document the project in details. I am also not happy with a few things. So, here's my attempt at improving and simplifying the design. The general idea is to remove the internals and replace them with my own. The objectives are as follows.

1. Treads can be controlled independently so that they can make all possible movements including spinning in place.
1. The arms can lift and lower independently. They only have one degree of freedom. It'd be too complicated to make them do more than that.
1. The head can turn left and right.
1. The eyes can light up. They each have a blue LED inside.
1. The eyesbrows can raise and lower, which is done by tilting the eyes around the center.
1. It can play pre-recorded sounds.
1. It can be controlled with a remote control.

The general approach and necessary materials and tools:
1. All the mechanical parts will be 3d printed. I use Fusion 360 for 3d modeling and FlashForge Adventure 3 Lite for printing.
1. The central control unit will be a microcontroller.
1. I will be using Arduino framework to code the mcu.
1. Misc electronic components that will be described in details in this document. All can be found on Amazon or AliExpress.
1. Misc small screws, also found on Amazon or AliExpress.
1. Misc tools such as a soldering station and precision screw drivers and drills.

The rest of the documentation is organized as follows.

* [Disassembly](disassembly.md)
* Prototyping
  - [Microcontroller](mcu.md)
  - [Controlling arms](arm.md)
  - [Rotating the head](head_rotation.md)
  - [Driving motors with H-bridges](hbridge.md)
  - [Driving one-direction motors](motor.md)
  - [Audio](audio.md)
  - [IR remote receiver](remote_receiver.md)

* [Integration](integration.md)

* [Custom Remote Control](remote_transmitter.md)

* [Firmware](firmware.md)

## Schematic

![schematic](schematic.svg)