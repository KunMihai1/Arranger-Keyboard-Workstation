/*
  ==============================================================================

    SupabaseClient.cpp
    Created: 28 Feb 2026 3:30:00am
    Author:  Kisuke

  ==============================================================================
*/

#include "SupabaseClient.h"
#include "BackendJson.h"

namespace
{
    // Supabase edge functions.
    const juce::String loginUrl        = "https://ecmlftmkoqszdwjugqtn.supabase.co/functions/v1/auth-proxy?grant_type=password";
    const juce::String signupUrl       = "https://ecmlftmkoqszdwjugqtn.supabase.co/functions/v1/auth-proxy?action=signup";
    const juce::String playtimeUrl     = "https://ecmlftmkoqszdwjugqtn.supabase.co/functions/v1/increment-playtime-api";
    const juce::String deviceUrl       = "https://ecmlftmkoqszdwjugqtn.supabase.co/functions/v1/insert-or-update-device-api";

    const juce::String jsonContentHeader   = "Content-Type: application/json\r\n";
    const juce::String bearerHeaderStart   = "Authorization: Bearer ";
    const juce::String headerLineEnd       = "\r\n";
    constexpr int      connectionTimeoutMs = 5000;
}

SupabaseClient::SupabaseClient()
{

}

HttpResult SupabaseClient::login(const juce::String& email,
    const juce::String& password)
{
    HttpResult result;

    const auto body = BackendJson::loginBody (email, password);   // escaped: a quote in a password stays text
    
    auto postUrl = juce::URL(loginUrl)
        .withPOSTData(body);
    juce::String extraHeaders;
    extraHeaders << jsonContentHeader;
    auto stream = postUrl.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders(extraHeaders)
        .withConnectionTimeoutMs(connectionTimeoutMs)
        .withStatusCode(&result.statusCode)
    );
    if (!stream)
    {
        result.body = BackendJson::connectionFailedBody();
        return result;
    };
    result.body= stream->readEntireStreamAsString();
    return result;
}



HttpResult SupabaseClient::signup(const juce::String& email,
    const juce::String& password,
    const juce::String& username)
{
    HttpResult result;

    const auto body = BackendJson::signupBody (email, password, username);
    auto postUrl = juce::URL(signupUrl)
        .withPOSTData(body);
    juce::String extraHeaders;
    int statusCode {};
    extraHeaders << jsonContentHeader;
    auto stream = postUrl.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(connectionTimeoutMs)
        .withExtraHeaders(extraHeaders)
        .withStatusCode(&result.statusCode)
    );
    if (!stream)
    {
        result.body = BackendJson::connectionFailedBody();
        return result;
    };
    result.body = stream->readEntireStreamAsString();   // the reply says whether the account was made
    return result;
}

HttpResult SupabaseClient::incrementPlaytime(int seconds, const juce::String& VID, const juce::String& PID)
{
    juce::String id, token;
    {
        std::lock_guard<std::mutex> lock(mutex); //not needed, now for future refresh token/ re-login/update values while timer is running
        id = userId;
        token = accessToken;
    }

    HttpResult result;

    const auto body = BackendJson::playtimeBody (id, PID, VID, seconds);

    auto postUrl = juce::URL(playtimeUrl)
        .withPOSTData(body);
    juce::String extraHeaders;
    extraHeaders << jsonContentHeader;
    extraHeaders << bearerHeaderStart + token + headerLineEnd;  
    int statusCode {};
    auto stream = postUrl.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders(extraHeaders)
        .withConnectionTimeoutMs(connectionTimeoutMs)
        .withStatusCode(&result.statusCode)
    );
    if (!stream)
    {
        result.body = BackendJson::connectionFailedBody();
        return result;
    };
    result.body = stream->readEntireStreamAsString();
    
    return result;
}

HttpResult SupabaseClient::addCurrency(int amount)
{
    HttpResult result;
    return result;
}

void SupabaseClient::setUserId(const juce::String& newId)
{
    std::lock_guard<std::mutex> lock(mutex);
    this->userId = newId;
}

void SupabaseClient::setAccessToken(const juce::String& newAccessToken)
{
    std::lock_guard<std::mutex> lock(mutex);
    this->accessToken = newAccessToken;
}

HttpResult SupabaseClient::addOrUpdateDevice(const juce::String& VID, const juce::String& PID, const juce::String& deviceName, int nrKeys)
{
    juce::String id, token;
    {
        std::lock_guard<std::mutex> lock(mutex); // protects userId/accessToken
        id = userId;
        token = accessToken;
    }

    HttpResult result;

    const auto body = BackendJson::deviceBody (id, VID, PID, deviceName, nrKeys);   // a device name may hold quotes

    
    auto postUrl = juce::URL(deviceUrl)
        .withPOSTData(body);

    
    juce::String extraHeaders;
    extraHeaders << jsonContentHeader;
    extraHeaders << bearerHeaderStart + token + headerLineEnd;

    int statusCode {};

    
    auto stream = postUrl.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders(extraHeaders)
        .withConnectionTimeoutMs(connectionTimeoutMs)
        .withStatusCode(&result.statusCode)
    );

    if (!stream)
    {
        result.body = BackendJson::connectionFailedBody();
        return result;
    };

    
    result.body = stream->readEntireStreamAsString();

    return result;
}