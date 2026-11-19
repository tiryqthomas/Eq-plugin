#pragma once

#include <juce_graphics/juce_graphics.h>

// All colours live here so the look can be swapped in one place.
namespace theme
{
    inline const juce::Colour background   { 0xff0b0b0c };
    inline const juce::Colour panel        { 0xff17171a };
    inline const juce::Colour grid         { 0x18ffffff };
    inline const juce::Colour gridLabel    { 0x66ffffff };
    inline const juce::Colour spectrum     { 0x40ffffff };
    inline const juce::Colour spectrumLine { 0x80ffffff };
    inline const juce::Colour curve        { 0xffffffff };
    inline const juce::Colour text         { 0xffe8e8ea };
    inline const juce::Colour textDim      { 0x80ffffff };

    inline juce::Colour band (int i)
    {
        static const juce::uint32 colours[] = { 0xffff6b6b, 0xffffb347, 0xfff4e04d, 0xff6bd47a, 0xff4dc3ff,
                                                0xff6b7bff, 0xffb36bff, 0xffff6bd1, 0xff7ee8e0, 0xffc7c7c7 };
        return juce::Colour (colours[i % 10]);
    }
}
