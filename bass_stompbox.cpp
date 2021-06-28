#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger    flanger;
DaisyPetal petal;
Tone       tone;
Compressor comp;

//>>compressor
Parameter threshParam, ratioParam, attackParam, releaseParam;
bool compressor_ON = true;
float comp_sig, comp_out;
//<<compressor

//>>flanger
bool  flanger_ON = true;
float deltarget, del; 
float lfotarget, lfo;
float wet_f;
float flanger_out;
//<<flanger

//>>tone
bool  tone_ON = false;
float tone_freq, tone_out;
//<<tone

void AudioCallback(float *in, float *out, size_t size)
{
    petal.ProcessAllControls();
    float dry_in;

    //>>from compressor
    //comp.SetThreshold(threshParam.Process());
    comp.SetThreshold(-15.f);
    comp.SetRatio(ratioParam.Process());
    //comp.SetRatio(10.f);
    //comp.SetAttack(attackParam.Process());
    comp.SetAttack(0.9f);
    //comp.SetRelease(releaseParam.Process());
    comp.SetRelease(0.9f);
    compressor_ON ^= petal.switches[0].RisingEdge();
    //<<from compressor

    //>>flanger
    deltarget = petal.knob[petal.KNOB_3].Process();
    //flanger.SetFeedback(petal.knob[3].Process()); fixed parameter down below
    flanger.SetFeedback(0.5f);
    float val = petal.knob[petal.KNOB_5].Process();
    flanger.SetLfoFreq(val * val * 10.f);
    lfotarget = petal.knob[petal.KNOB_6].Process();

    flanger_ON ^= petal.switches[1].RisingEdge();

    wet_f += petal.encoder.Increment() * .05f;  //encoder
    wet_f = fclamp(wet_f, 0.f, 1.f);            //encoder

    //>>flanger
  
    //>>tone
    float tone_freq = 200.0f; //200 Hz by ears
    tone.SetFreq(tone_freq);
    float tone_ratio = petal.knob[petal.KNOB_4].Process();
    //tone_ON ^= petal.switches[2].RisingEdge();
    tone_ON = flanger_ON; // tone shaping! 
    //<<tone

    for(size_t i = 0; i < size; i++)
    {
        //>>compressor
        dry_in    = in[i] * 2.0f;
        // Scales input by 2 and then the output by 0.5
        // This is because there are 6dB of headroom on the daisy
        // and currently no way to tell where '0dB' is supposed to be
        comp_sig  = comp.Process(dry_in);
        comp_out = in[i]; // to avoid if else with footswitch

        if(compressor_ON)
        {
            comp_out = comp_sig * 0.25f;//it was too loud
        }
        //<<compressor

        //>>flanger
        fonepole(del, deltarget, .0001f); //smooth at audio rate LOW PASS FILTER
        flanger.SetDelay(del);

        fonepole(lfo, lfotarget, .0001f); //smooth at audio rate
        flanger.SetLfoDepth(lfo);

        flanger_out = comp_out;

        if(flanger_ON)
        {
            flanger_out = 1.5f * (flanger.Process(comp_out) * wet_f + comp_out * (1.f - wet_f)); //1.5f to match the volume
        }
        //<<flanger


        //>>tone
        tone_out = flanger_out; 

        if(tone_ON)
        {
            tone_out = flanger_out + tone.Process(flanger_out) * tone_ratio;
        }       
        //<<tone
        out[i] =  tone_out;
    }
}

int main(void)
{
    petal.Init();
  
    float samplerate = petal.AudioSampleRate();

    //>>compressor
    comp.Init(samplerate);

    //threshParam.Init(petal.knob[petal.KNOB_3], -80.0f, 0.f, Parameter::LINEAR); //fixed, see above
    ratioParam.Init(petal.knob[petal.KNOB_1], 1.2f, 40.f, Parameter::LINEAR);
    //attackParam.Init(petal.knob[petal.KNOB_5], 0.01f, 1.f, Parameter::EXPONENTIAL); //fixed, see above
    //releaseParam.Init(petal.knob[petal.KNOB_6], 0.01f, 1.f, Parameter::EXPONENTIAL); //fixed, see above

    //>>compressor

    //>>flanger
    deltarget = del = 0.f;
    lfotarget = lfo = 0.f;
    flanger.Init(samplerate);
    wet_f = .8f;//default value for flanger after power off/on
    //<<flanger

    tone.Init(samplerate);

    petal.StartAdc();
    petal.StartAudio(AudioCallback);

    while(1)
    {
        petal.DelayMs(6);
        petal.ClearLeds();

        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_1, compressor_ON);
        petal.SetFootswitchLed(petal.FOOTSWITCH_LED_2, flanger_ON);
        //petal.SetFootswitchLed(petal.FOOTSWITCH_LED_3, tone_ON);
       

        int wet_f_int  = (int)(wet_f * 9.f); //default value

        for(int i = 0; i < wet_f_int; i++)
        {
            if (tone_ON) {petal.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);}
            else {petal.SetRingLed((DaisyPetal::RingLed)i, 0.f, 1.f, 0.f);} //make red when tone is ON
        }

        petal.UpdateLeds();
    }
}

