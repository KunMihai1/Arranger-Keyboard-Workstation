/*
  ==============================================================================

    SupabaseClient.cpp
    Created: 28 Feb 2026 3:30:00am
    Author:  Kisuke

  ==============================================================================
*/

#include "SupabaseClient.h"

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
    const juce::String connectionFailedBody = "{\"error\":\"connection failed\"}";
    constexpr int      connectionTimeoutMs = 5000;

    // Request bodies are written by hand; each piece below is the JSON text between two values.
    namespace JsonText
    {
        const juce::String emailStart        = "{\"email\":\"";
        const juce::String passwordField     = "\",\"password\":\"";
        const juce::String stringObjectEnd   = "\"}";
        const juce::String signupNameField   = "\",\"data\":{\"name\":\"";
        const juce::String signupEnd         = "\"}}";
        const juce::String userIdStart       = "{\"user_id\":\"";
        const juce::String pidField          = "\",\"pid\":\"";
        const juce::String vidField          = "\",\"vid\":\"";
        const juce::String secondsField      = "\",\"seconds\":";
        const juce::String vendorIdField     = "\",\"vendor_id\":\"";
        const juce::String productIdField    = "\",\"product_id\":\"";
        const juce::String deviceNameField   = "\",\"device_name\":\"";
        const juce::String keyCountField     = "\",\"nr_keys\":";
        const juce::String objectEnd         = "}";
    }
}

SupabaseClient::SupabaseClient()
{

}

HttpResult SupabaseClient::login(const juce::String& email,
    const juce::String& password)
{
    HttpResult result;

    juce::String body = JsonText::emailStart + email +
        JsonText::passwordField + password + JsonText::stringObjectEnd;
    
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
        result.body = connectionFailedBody;
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

    juce::String body = JsonText::emailStart + email +
        JsonText::passwordField + password +
        JsonText::signupNameField + username + JsonText::signupEnd;
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
        result.body = connectionFailedBody;
        return result;
    };
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

    juce::String body = JsonText::userIdStart + id + JsonText::pidField + PID
        + JsonText::vidField + VID
        + JsonText::secondsField + juce::String(seconds) + JsonText::objectEnd;

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
        result.body = connectionFailedBody;
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

    juce::String body = JsonText::userIdStart + id +
        JsonText::vendorIdField + VID +
        JsonText::productIdField + PID +
        JsonText::deviceNameField + deviceName +
        JsonText::keyCountField + juce::String(nrKeys) + JsonText::objectEnd;

    
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
        result.body = connectionFailedBody;
        return result;
    };

    
    result.body = stream->readEntireStreamAsString();

    return result;
}