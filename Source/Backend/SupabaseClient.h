/*
  ==============================================================================

    SupabaseClient.h
    Created: 28 Feb 2026 3:30:00am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

struct HttpResult {
    int statusCode {};
    juce::String body;
};

class SupabaseClient
{
public:
    SupabaseClient();

    HttpResult login(const juce::String& email, const juce::String& password);

    HttpResult signup(const juce::String& email, const juce::String& password, const juce::String& username);

    HttpResult incrementPlaytime(int seconds, const juce::String& VID="", const juce::String& PID = "");

    HttpResult addCurrency(int amount);

    void setUserId(const juce::String& newId);

    void setAccessToken(const juce::String& newAccessToken);

    HttpResult addOrUpdateDevice(const juce::String& VID, const juce::String& PID, const juce::String& deviceName, int nrKeys);

private:
    std::mutex mutex;
    juce::String userId;
    juce::String accessToken;
};