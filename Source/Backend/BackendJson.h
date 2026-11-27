#pragma once

#include <juce_core/juce_core.h>

/*  The JSON the app sends to and reads from its Supabase functions (spec 2026-11-27 sign-in). Pure: no I/O.
    Bodies are built with juce::JSON, so a quote or backslash in a password, user name or device name stays text
    (they were concatenated by hand before). The readers make the decisions LoginComponent made inline, unchanged. */
namespace BackendJson
{
    juce::String loginBody (const juce::String& email, const juce::String& password);
    juce::String signupBody (const juce::String& email, const juce::String& password, const juce::String& username);
    juce::String playtimeBody (const juce::String& userId, const juce::String& productId, const juce::String& vendorId, int seconds);
    juce::String deviceBody (const juce::String& userId, const juce::String& vendorId, const juce::String& productId,
                             const juce::String& deviceName, int keyCount);

    struct LoginReply
    {
        enum class Outcome
        {
            signedIn,       ///< an access token came back
            rejected,       ///< the server said why (wrong password, no connection, ...): message
            confirmEmail,   ///< the account exists but its email is not confirmed yet
            unreadable,     ///< not a JSON object
            unexpected      ///< an object without a token or an error
        };
        Outcome outcome { Outcome::unreadable };
        juce::String message, userId, accessToken;
    };

    LoginReply readLoginReply (const juce::String& body);

    struct SignupReply
    {
        enum class Outcome
        {
            created,
            createdCheckEmail,   ///< created; a confirmation email was sent
            rejected,            ///< message (may be empty)
            notCreated,          ///< neither a session nor a user came back
            unreadable
        };
        Outcome outcome { Outcome::unreadable };
        juce::String message;
    };

    SignupReply readSignupReply (const juce::String& body);
}
