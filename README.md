# MuxADC

A library built for the [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html) using [ADC](https://github.com/pedvide/ADC) and [IntervalTimer](https://www.pjrc.com/teensy/td_timing_IntervalTimer.html) that concurrently feeds many `AudioPlayQueue` objects with the samples taken from a user-defined set of corresponding pins.

```c++
if(!MuxADC::allocateChannels(4)) // must be an even number of channels!
    Serial.println("Failed to allocate channels");
MuxADC::route(0, A0, &queue1);
MuxADC::route(1, A1, &queue2);
MuxADC::route(2, A2, &queue3);
MuxADC::route(3, A3, &queue4);
if(!MuxADC::begin())
    Serial.println("Failed to begin MuxADC");
```