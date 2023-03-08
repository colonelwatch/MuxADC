#include "MuxADC.h"

static const int MAX_N_BLOCKS = 8;
static const int CONTIG_SIZE = 1024;
static const int BUFFER_SIZE = AUDIO_BLOCK_SAMPLES+CONTIG_SIZE;

static ADC *adc;
static IntervalTimer *sampling_timer;
static int n_channels;
static int8_t *pins;
static AudioPlayQueue **queues;
static int16_t **samples;
static int state = 0; // \in {-1} \union {0, 1, ..., n_states} decides which channels to sample or to stop sampling
static int write_head = 0; // shared to ensure that all channels are refreshed at the same time
static int read_head = 0;  //   ^
static int lost_samples = 0;    // use of IntervalTimer causes the Audio library to lag behind some samples, this records 
                                //  the number of samples lost (totalled over all channels)

bool MuxADC::allocateChannels(int n_chann){
    n_channels = n_chann;

    pins = new int8_t[n_channels];
    queues = new AudioPlayQueue*[n_channels];
    samples = new int16_t*[n_channels];
    for(int i = 0; i < n_channels; i++) samples[i] = new int16_t[BUFFER_SIZE];

    bool alloc_success = (pins && queues && samples);
    for(int i = 0; i < n_channels; i++) alloc_success &= (samples[i] != NULL);
    return alloc_success;
}

void MuxADC::route(int i_chann, int8_t pin, AudioPlayQueue* queue){
    pins[i_chann] = pin;
    queues[i_chann] = queue;

    queues[i_chann]->setMaxBuffers(MAX_N_BLOCKS);
}

static void fsm_isr(){ // sampling interrupt service routine
    int next_state;

    if(state != -1){ // hadn't encountered overflow risk previously
        ADC::Sync_result res = adc->readSynchronizedSingle();
        samples[2*state][write_head] = res.result_adc0;
        samples[2*state+1][write_head] = res.result_adc1;

        if(state != n_channels/2-1){ // haven't read all channels <-> not about to advance write_head
            next_state = state+1;
            adc->startSynchronizedSingleRead(pins[2*next_state], pins[2*next_state+1]);
        }
        else{ // have read all channels <-> about to advance write_head
            write_head = (write_head+1)%(BUFFER_SIZE);
            if(write_head != read_head){ // no overflow risk
                next_state = 0;
                adc->startSynchronizedSingleRead(pins[2*next_state], pins[2*next_state+1]);
            }
            else{ // overflow risk
                next_state = -1;
            }
        }
    }
    else{ // had encountered overflow risk previously
        lost_samples += 2;
        if(write_head != read_head){ // if we've moved on from the overflow risk
            next_state = 0;
            adc->startSynchronizedSingleRead(pins[2*next_state], pins[2*next_state+1]);
        }
        else next_state = -1;
    }

    state = next_state;
}

bool MuxADC::begin(){
    if(n_channels%2 != 0) return false; // we need an even number of channels

    for(int i = 0; i < n_channels; i++) pinMode(pins[i], INPUT_DISABLE); // INPUT_DISABLE is the right mode for analog!

    // configure joint ADCs
    adc = new ADC();
    adc->adc0->setResolution(12);
    adc->adc0->setAveraging(1);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    adc->adc1->setResolution(12);
    adc->adc1->setAveraging(1);
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED);
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);

    // starting sampling interrupt
    bool init_success = true;
    init_success &= adc->startSynchronizedSingleRead(pins[0], pins[1]); // bootstrap first conversion
    sampling_timer = new IntervalTimer();
    // sampling_timer->priority(216);   // 208 is the priorty of the Audio library, so choosing a lower priority like 216
                                        //  causes the sampling interrupt to lag behind the Audio library instead!
    init_success &= sampling_timer->begin(fsm_isr, 1000000./AUDIO_SAMPLE_RATE_EXACT/(n_channels/2));

    return init_success;
}

void MuxADC::refreshChannels(){
    // if there is enough data to call playBuffer...
    int buffer_size = (write_head > read_head)? write_head-read_head-1 : write_head+(BUFFER_SIZE)-read_head-1;
    if(buffer_size >= AUDIO_BLOCK_SAMPLES){
        // acquire pointers to a buffer in each queue
        int16_t **out_buffers = new int16_t*[n_channels];
        for(int i = 0; i < n_channels; i++) out_buffers[i] = queues[i]->getBuffer();
        
        // copy from samples to the buffers...
        for(int j = 0; j < AUDIO_BLOCK_SAMPLES; j++){
            for(int i = 0; i < n_channels; i++){
                out_buffers[i][j] = 16*((int16_t)samples[i][read_head]-2048);
            }
            read_head = (read_head+1)%(BUFFER_SIZE); // ... in time-major order (lets us advance read_head as we go)
        }
        
        for(int i = 0; i < n_channels; i++) queues[i]->playBuffer();
        delete[] out_buffers;
    }
}

int MuxADC::_getLostSamples(){
    return lost_samples;
}

void MuxADC::_resetLostSamples(){
    lost_samples = 0;
}