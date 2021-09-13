//to upload print in the console "make program-dfu"
//or if Autohotkey is enabled  "uu" and space

#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
DaisyPetal petal;

bool compressor_ON = false;
bool flanger_ON    = false;
bool tone_ON       = false;
bool osc_ON        = false;


#define WAVE_LENGTH 4
float presets[WAVE_LENGTH];
/** The DSY_QSPI_BSS attribute places your array in QSPI memory */
float DSY_QSPI_BSS qspi_buffer[WAVE_LENGTH];

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    petal.ProcessAllControls();
    compressor_ON ^= petal.switches[0].RisingEdge();
    flanger_ON ^= petal.switches[1].RisingEdge();
    tone_ON ^= petal.switches[2].RisingEdge();
    osc_ON ^= petal.switches[3].RisingEdge();

    for(size_t i = 0; i < size; i++)
    {
        out[i] = in[i];
    }
}


int main(void)
{
    hw.Configure();
    hw.Init();
    petal.Init();

    size_t size_preset = sizeof(presets[0]) * WAVE_LENGTH;
    /** Grab physical address from pointer */
    size_t address = (size_t)qspi_buffer;
    /** Erase qspi and then write that wave */
    hw.qspi.Erase(address, address + size_preset);
    hw.qspi.Write(address, size_preset, (uint8_t *)presets);


    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1)
    {
        petal.DelayMs(6);
        petal.ClearLeds();

        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_1, compressor_ON);
        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_2, flanger_ON);
        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_3, tone_ON);
        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_3, osc_ON);

        petal.UpdateLeds();
    }
}