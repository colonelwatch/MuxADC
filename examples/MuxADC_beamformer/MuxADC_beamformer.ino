// Assuming microphones aligned on the same plane/line, a beamformer built 
//  using a 6-channel config of MuxADC can is pointed in the direction(s) 
//  perpendicular to that plane/line.
// Outputs to a PT8211 DAC, so change that if needed.

// <Audio System Design Tool>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=69,35
AudioPlayQueue           queue2;         //xy=100,70
AudioPlayQueue           queue3;         //xy=133,106
AudioPlayQueue           queue4;         //xy=164,141
AudioPlayQueue           queue5;         //xy=194,176
AudioPlayQueue           queue6;         //xy=224,211
AudioFilterFIR           fir1;           //xy=242,35
AudioFilterFIR           fir2;           //xy=270,70
AudioFilterFIR           fir3;           //xy=299,106
AudioFilterFIR           fir4;           //xy=327,141
AudioFilterFIR           fir5;           //xy=355,176
AudioFilterFIR           fir6;           //xy=383,211
AudioMixer4              mixer1;         //xy=508,83
AudioMixer4              mixer2;         //xy=701,118
AudioFilterBiquad        biquad1;        //xy=848,118
AudioOutputPT8211        pt8211_1;       //xy=1014,117
AudioConnection          patchCord1(queue1, fir1);
AudioConnection          patchCord2(queue2, fir2);
AudioConnection          patchCord3(queue3, fir3);
AudioConnection          patchCord4(queue4, fir4);
AudioConnection          patchCord5(queue5, fir5);
AudioConnection          patchCord6(queue6, fir6);
AudioConnection          patchCord7(fir1, 0, mixer1, 0);
AudioConnection          patchCord8(fir2, 0, mixer1, 1);
AudioConnection          patchCord9(fir3, 0, mixer1, 2);
AudioConnection          patchCord10(fir4, 0, mixer1, 3);
AudioConnection          patchCord11(fir5, 0, mixer2, 1);
AudioConnection          patchCord12(fir6, 0, mixer2, 2);
AudioConnection          patchCord13(mixer1, 0, mixer2, 0);
AudioConnection          patchCord14(mixer2, biquad1);
AudioConnection          patchCord15(biquad1, 0, pt8211_1, 0);
AudioConnection          patchCord16(biquad1, 0, pt8211_1, 1);
// GUItool: end automatically generated code

// </Audio System Design Tool>

#include <MuxADC.h>

unsigned long last_millis;

const int filter_length = 32;
const float shifts[6] = {16, 16, 15.666, 15.666, 15.333, 15.333};
int16_t filters[6][filter_length];

float norm_sinc(float x){
    if(x == 0)
        return 1;
    else
        return sin(M_PI*x)/(M_PI*x);
}

void setup(){
    Serial.begin(9600);
    AudioMemory(100);

    Serial.println("Starting MuxADC...");

    // corrective delay to line up the channels
    for(int i = 0; i < 6; i++)
        for(int j = 0; j < filter_length; j++)
            filters[i][filter_length-j-1] = 32767*norm_sinc(j-shifts[i]);
    fir1.begin(filters[0], filter_length);
    fir2.begin(filters[1], filter_length);
    fir3.begin(filters[2], filter_length);
    fir4.begin(filters[3], filter_length);
    fir5.begin(filters[4], filter_length);
    fir6.begin(filters[5], filter_length);
    
    // averaging all channels
    mixer1.gain(0, 1/6.);
    mixer1.gain(1, 1/6.);
    mixer1.gain(2, 1/6.);
    mixer1.gain(3, 1/6.);
    mixer2.gain(0, 1);
    mixer2.gain(1, 1/6.);
    mixer2.gain(2, 1/6.);

    // filter to knock out spatial aliasing (assuming a minimum gap of 5cm)
    biquad1.setLowpass(0, 3430, 0.707);

    if(!MuxADC::allocateChannels(6))
        Serial.println("Failed to allocate channels");
    MuxADC::route(0, A0, &queue1);
    MuxADC::route(1, A1, &queue2);
    MuxADC::route(2, A2, &queue3);
    MuxADC::route(3, A3, &queue4);
    MuxADC::route(4, A4, &queue5);
    MuxADC::route(5, A5, &queue6);
    if(!MuxADC::begin())
        Serial.println("Failed to begin MuxADC");

    last_millis = millis();
}

void loop(){
    MuxADC::refreshChannels();

    if(millis() - last_millis > 1000){
        int lost_samples = MuxADC::_getLostSamples();
        MuxADC::_resetLostSamples();
        
        Serial.println();
        Serial.print("Lost samples (totalled over all channels): ");
        Serial.println(lost_samples);
        Serial.print("AudioMemoryUsageMax: ");
        Serial.println(AudioMemoryUsageMax());

        last_millis = millis();
    }
}
