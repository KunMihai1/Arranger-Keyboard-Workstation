#pragma once

#include <juce_core/juce_core.h>

/*  The last email used to log in, kept in the settings file (key LastEmail, spec part 1 §10: "like a browser").
    Never the password. A null settings pointer means nothing is remembered. */
namespace RememberedEmail
{
    juce::String load (const juce::PropertySet* settings);
    void save (juce::PropertySet* settings, const juce::String& email);   ///< an empty email forgets
    void forget (juce::PropertySet* settings);   ///< "Not you?": this computer only, no account is touched

    /** "Remembered from last time · Not you?" shows in Log in while the field still holds the remembered email. */
    bool showRow (const juce::String& remembered, const juce::String& typed, bool signingUp);
}
