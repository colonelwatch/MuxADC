// Cycles through queues of fed using a 4-channel config of MuxADC using a mixer 
//  and reports which channel is being output in Serial, along with reporting 
//  the number of samples lost and the max memory usage.
// Outputs to a PT8211 DAC, so change that if needed.

// <Audio System Design Tool>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=117,39
AudioPlayQueue           queue2;         //xy=145,73
AudioPlayQueue           queue3;         //xy=173,107
AudioPlayQueue           queue4;         //xy=201,141
AudioMixer4              mixer1;         //xy=366,89
AudioOutputPT8211        pt8211_1;       //xy=537,87
AudioConnection          patchCord1(queue1, 0, mixer1, 0);
AudioConnection          patchCord2(queue2, 0, mixer1, 1);
AudioConnection          patchCord3(queue3, 0, mixer1, 2);
AudioConnection          patchCord4(queue4, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, 0, pt8211_1, 0);
AudioConnection          patchCord6(mixer1, 0, pt8211_1, 1);
// GUItool: end automatically generated code

// </Audio System Design Tool>

#include <MuxADC.h>

unsigned long last_millis;
int channel = 0;

void select_channel(int channel){
    mixer1.gain(0, channel == 0);
    mixer1.gain(1, channel == 1);
    mixer1.gain(2, channel == 2);
    mixer1.gain(3, channel == 3);
}

void setup(){
    Serial.begin(9600);
    AudioMemory(16);

    select_channel(channel);

    if(!MuxADC::allocateChannels(4)) // must be an even number of channels!
        Serial.println("Failed to allocate channels");
    MuxADC::route(0, A0, &queue1);
    MuxADC::route(1, A1, &queue2);
    MuxADC::route(2, A2, &queue3);
    MuxADC::route(3, A3, &queue4);
    if(!MuxADC::begin())
        Serial.println("Failed to begin MuxADC");

    last_millis = millis();
}

void loop(){
    MuxADC::refreshChannels();

    if(millis() - last_millis > 1000){
        int lost_samples = MuxADC::_getLostSamples();
        MuxADC::_resetLostSamples();
        channel = (channel+1)%4;
        select_channel(channel);

        Serial.println();
        Serial.print("Channel: ");
        Serial.println(channel);
        Serial.print("Lost samples (totalled over all channels): ");
        Serial.println(lost_samples);
        Serial.print("AudioMemoryUsageMax: ");
        Serial.println(AudioMemoryUsageMax());

        last_millis = millis();
    }
}