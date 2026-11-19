#pragma once

#include <juce_graphics/juce_graphics.h>

// All colours live here so the look can be swapped in one place.
namespace theme
{
    inline const juce::Colour background    { 0xff1a1820 };
    inline const juce::Colour backgroundTop { 0xff24212c };
    inline const juce::Colour header        { 0xff121117 };
    inline const juce::Colour panel         { 0xe62a2732 };
    inline const juce::Colour panelEdge     { 0x30ffffff };
    inline const juce::Colour grid          { 0x12ffffff };
    inline const juce::Colour gridLabel     { 0x70ffffff };
    inline const juce::Colour dbLabel       { 0xffe8b84a };
    inline const juce::Colour spectrumFill  { 0x26ffffff };
    inline const juce::Colour spectrumLine  { 0x66ffffff };
    inline const juce::Colour curve         { 0xfff2c94c };
    inline const juce::Colour text          { 0xffe8e6ee };
    inline const juce::Colour textDim       { 0x8ce8e6ee };
    inline const juce::Colour knobBody      { 0xff1d1b23 };
    inline const juce::Colour knobTrack     { 0x26ffffff };

    inline juce::Colour band (int i)
    {
        static const juce::uint32 colours[] = { 0xffd5f05a, 0xffef5a5a, 0xff5aa8ef, 0xffb05aef, 0xff7a6bff,
                                                0xff5aefa0, 0xffefa05a, 0xffef5ac8, 0xff5ae6ef, 0xffc8c8d0 };
        return juce::Colour (colours[i % 10]);
    }
}
