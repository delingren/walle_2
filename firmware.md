# Firmware

There are two independent parts. One for the main toy and one for the remote control. I'm using Arduino framework for both.

## Main

The firmware is relatively straight-forward. One thing worth mentioning is animation. When we recieve a command, such as turning the head, we want the operation to be rather smooth. So, instead of setting the position immediately, we animate the transition to the target position. Animation is also needed to create breathing patterns in the eyes. This also applies to the speed of the motors so that they accelerate and decelarate smoothly. So I am defining an `Animatable` class and keep a list of ongoing animations at any moment.

Then I have these kinds of animations.
1. Linear transition to a target value over a specified duration.
1. Linear transition to a target value at at specified speed.
1. Offset by a specified increment over a specified duration.
1. Transition to a target value asap.
1. Maintain a constant value for a duration.

We can of course define other types of animations. But this list should be enough for this simple application. Every time we execute `loop()`, i.e. in each frame, we update all the animation status.

1. For an ongoing animation, we calculate its transient value by interpolating using the start time, duration, initial value, and target value.
1. If an animation has reached its target value, we remove it from the list.
1. We also read input from all the sources (remote control and pushbutton) and queue animations as needed.

### Libraries

Instead of relying on Arduino IDE's library manager, I enlisted DYPlayer and IRRemote libraries as submodules and froze them at the current version. So that it's more self contained and resistant to future breaking changes to these libraries. The Servo library, however, is part of RP2040 board SDK which uses PIO instead of PWM. There is no need to enlist it as a submodule, unless I also enlist the whole SDK, which is unnecessarily complicated.

After enlisting these two libraries, I linked all files in their `src` directories under my sketch's `src` directory. Arduino Framework link all source files under `src` directory. I don't know if there is a way to add a local library. But creating symbolic links is pretty straight-forward with the following bash commands.

```
for i in ../../third_party/Arduino-IRremote/src/* ; do; ln -s $i ; done
for i in ../../third_party/dyplayer/src/* ; do; ln -s $i ; done
```

## Remote Control