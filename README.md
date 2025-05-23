# Wall-E 2.0

I bought this Wall-E toy made by Thinkway on eBay. Actually I bought a few, all partially  broken. The toy is pretty primitive. It can make some basic moves. But the treads cannot be independently controlled. The arms can go up and down a little too, but not independently. The head can turn. The eyes can tilt a little bit. It only has one motor and it's smartly connected to all those parts with a gearbox to make them move in some ways. But in general, there isn't a high degree of maneuverability. It also has a speaker to make some sounds. When new, it came with an IR remote control. None of the ones I bought still had it.

![wall-e](./media/IMG_1013.jpeg)

My plan is to control it with a microcontroller. I already made a v1 but didn't document the project in details. I am also not happy with a few things. So, here's my attempt at improving and simplifying the design. The general idea is to remove the internals and replace them with my own. The objectives are as follows.

1. Treads can be controlled independently so that they can make all possible movements including spinning in place.
1. The arms can lift and lower independently. They only have one degree of freedom. It'd be too complicated to make them do more than that.
1. The head can turn left and right.
1. The eyes can light up. They each have a blue LED inside.
1. The eyebrows can raise and lower, which is done by tilting the eyes around the center.
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

* Schematics
  - [Main unit](./schematic-main.svg)
  - [Remote control](./schematic-remote.svg)

* 3D models
  - [Fusion 360 file](./wall-e.f3d)
  - [STL files](./stl/)
## Demo

Here is the demo video of the final product, controlled by a Roku remote control and my custom remote control. It opens on youtube.

[![demo](http://img.youtube.com/vi/PiLhb5Huz5E/0.jpg)](http://www.youtube.com/watch?v=PiLhb5Huz5E)

## Future plans

In the next version, I am planning to make these improvements.

* More powerful H-bridge. In this version, I am using two 6V/75RPM N30 motors. They are a little slow. But faster motors don't seem to have enough torque. I suspect the problem is the H-bridge IC not providing enough current/voltage. In the next version, I'll try to root cause the issue and possibly replace the H-bridge with an L298N breakout board.
* Improve remote control range. The range of my custom remote control is rather poor, running on 3V. There are a few possible solutions I can think of, in order of increasing complexity.
  - Use an IR LED with less forward voltage drop.
  - Use 433MHz RF transmitter and receiver instead of IR. I think the IR protocols are [OOF](https://en.wikipedia.org/wiki/On–off_keying) modulated. So I should be able to connect the RF receiver to the same pin as the IR receiver and use the same protocol. The library doesn't care about the transmission medium, after all.
  - Use 3.7V lithium batteries, with a charging circuit, on the transmitter. There is enough room in the gamepad to put in a charging circuit. The wiring and assembly needs a little work.
* Improve remote control protocol. One problem I found with my current design is. If a joystick has changed position but the transmission is lost, the receiver is trapped in the last state. This could result in the motors running indefinitely. There are a couple of improvements that can be done here.
  - On the transmitter, send repeats. We don't need to send them in quick succession if the position has not changed. But at least we should send at least a few repeats per second.
  - On the receiver, restore to home position after having not received anything for a duration.
* Utilize play mode. The three-way power switch has an "on" position and a "play" position. The play position connects two wires in addition to the power. We can connect one of the wires to a pin and the other to `GND` or `Vcc` and read the wire upon booting. If we're in the play position, we would ignore the remote control signal. Instead, we can have a few preprogrammed sequences that can be played by pressing the button on Wall-E's collar bone.
