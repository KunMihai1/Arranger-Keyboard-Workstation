#include "RememberedEmail.h"
#include "../Common/PersistedKeys.h"

juce::String RememberedEmail::load (const juce::PropertySet* settings)
{
    return settings != nullptr ? settings->getValue (SettingsFileKeys::lastEmail) : juce::String();
}

void RememberedEmail::save (juce::PropertySet* settings, const juce::String& email)
{
    if (settings == nullptr)
        return;
    if (email.isEmpty())
        settings->removeValue (SettingsFileKeys::lastEmail);   // nothing typed: nothing to remember
    else
        settings->setValue (SettingsFileKeys::lastEmail, email);
}

void RememberedEmail::forget (juce::PropertySet* settings)
{
    if (settings != nullptr)
        settings->removeValue (SettingsFileKeys::lastEmail);
}

bool RememberedEmail::showRow (const juce::String& remembered, const juce::String& typed, bool signingUp)
{
    return ! signingUp && remembered.isNotEmpty() && typed.trim() == remembered;
}
