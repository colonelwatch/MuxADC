#ifndef MUXADC_H
#define MUXADC_H

#include <ADC.h>
#include <Audio.h>

namespace MuxADC{

bool allocateChannels(int n_chann);
void route(int i_chann, int8_t pin, AudioPlayQueue* queue);
bool begin();
void refreshChannels(); // call this often in loop() or else the queues will stall

// gives access the number of samples lost due to overflow risk (see MuxADC.cpp for explanation)
int _getLostSamples();
void _resetLostSamples();

} // namespace MuxADC

#endif // MUXADC_H
