#include <JuceHeader.h>
#include "Backend/BackendJson.h"
#include "Backend/RememberedEmail.h"

/*  Sign-in (spec 2026-11-27): the bodies sent to the Supabase functions are real JSON whatever the user types, the
    replies are read as LoginComponent always read them, and the last email is remembered on this computer only. */

namespace BackendJsonTestValues
{
    const juce::String testName     = "BackendJson";
    const juce::String testCategory = "Unit";

    const juce::String plainBodies     = "Ordinary values give the same bodies the app sent before";
    const juce::String escapedBodies   = "Quotes, backslashes and new lines stay text and the body stays JSON";
    const juce::String loginReplies    = "Login replies: token, server error, unconfirmed email, no connection, junk";
    const juce::String signupReplies   = "Sign-up replies: created, check email, error, nothing created, no connection, junk";
    const juce::String rememberedEmail = "The last email is saved, read back and forgotten in the settings";
    const juce::String rememberedRow   = "'Remembered from last time' shows only in Log in while the field holds that email";

    const juce::String email    = "player@example.com";
    const juce::String password = "secret";
    const juce::String username = "Player";
    const juce::String userId   = "user-1";
    const juce::String productId = "0x0042";
    const juce::String vendorId  = "0x07CF";
    const juce::String deviceName = "Keys 61";
    constexpr int seconds  = 300;
    constexpr int keyCount = 61;

    const juce::String oldLoginBody    = "{\"email\":\"player@example.com\",\"password\":\"secret\"}";
    const juce::String oldSignupBody   = "{\"email\":\"player@example.com\",\"password\":\"secret\",\"data\":{\"name\":\"Player\"}}";
    const juce::String oldPlaytimeBody = "{\"user_id\":\"user-1\",\"pid\":\"0x0042\",\"vid\":\"0x07CF\",\"seconds\":300}";
    const juce::String oldDeviceBody   = "{\"user_id\":\"user-1\",\"vendor_id\":\"0x07CF\",\"product_id\":\"0x0042\",\"device_name\":\"Keys 61\",\"nr_keys\":61}";

    const juce::String awkwardPassword = "pa\"ss\\word\nend";
    const juce::String awkwardName     = "My \"Keys\" \\ 88";
    const juce::String passwordKey     = "password";
    const juce::String dataKey         = "data";
    const juce::String nameKey         = "name";
    const juce::String deviceNameKey   = "device_name";

    const juce::String tokenReply       = "{\"access_token\":\"tok\",\"user\":{\"id\":\"user-1\",\"email\":\"player@example.com\"}}";
    const juce::String token            = "tok";
    const juce::String descriptionReply = "{\"error\":\"invalid_grant\",\"error_description\":\"Invalid login credentials\"}";
    const juce::String description      = "Invalid login credentials";
    const juce::String messageReply     = "{\"msg\":\"Email rate limit\"}";
    const juce::String messageText      = "Email rate limit";
    const juce::String connectionReply  = "{\"error\":\"connection failed\"}";
    const juce::String confirmReply     = "{\"confirmation_required\":\"true\"}";
    // What the live auth proxy answers for an account whose email is not confirmed yet (Supabase, 2026-11-28).
    const juce::String unconfirmedReply = "{\"code\":400,\"error_code\":\"email_not_confirmed\",\"msg\":\"Email not confirmed\"}";
    // And for a wrong password: a rejection with the server's words.
    const juce::String wrongPasswordReply = "{\"code\":400,\"error_code\":\"invalid_credentials\",\"msg\":\"Invalid login credentials\"}";
    const juce::String strangeReply     = "{\"hello\":\"world\"}";
    const juce::String junkReply        = "<html>502</html>";
    const juce::String emptyReply;

    const juce::String sessionReply     = "{\"access_token\":\"tok\",\"user\":{\"id\":\"user-1\"}}";
    const juce::String userReply        = "{\"id\":\"user-1\"}";
    const juce::String confirmSentReply = "{\"id\":\"user-1\",\"confirmation_sent_at\":\"2026-11-27T22:00:00Z\"}";
    const juce::String signupErrorReply = "{\"error\":\"User already registered\"}";
    const juce::String signupErrorText  = "User already registered";

    const juce::String otherEmail = "someone@example.com";
    const juce::String lastEmailKey = "LastEmail";   // the settings file's key: persisted text
}

using namespace BackendJsonTestValues;

class BackendJsonTest : public juce::UnitTest
{
public:
    BackendJsonTest() : juce::UnitTest (BackendJsonTestValues::testName, BackendJsonTestValues::testCategory) {}

    void runTest() override
    {
        using Login  = BackendJson::LoginReply::Outcome;
        using Signup = BackendJson::SignupReply::Outcome;

        beginTest (plainBodies);
        expectEquals (BackendJson::loginBody (email, password), oldLoginBody);
        expectEquals (BackendJson::signupBody (email, password, username), oldSignupBody);
        expectEquals (BackendJson::playtimeBody (userId, productId, vendorId, seconds), oldPlaytimeBody);
        expectEquals (BackendJson::deviceBody (userId, vendorId, productId, deviceName, keyCount), oldDeviceBody);

        beginTest (escapedBodies);
        {
            const auto login = juce::JSON::parse (BackendJson::loginBody (email, awkwardPassword));
            expect (login.isObject());
            expectEquals (login[passwordKey.toRawUTF8()].toString(), awkwardPassword);

            const auto signup = juce::JSON::parse (BackendJson::signupBody (email, awkwardPassword, awkwardName));
            expect (signup.isObject());
            expectEquals (signup[dataKey.toRawUTF8()][nameKey.toRawUTF8()].toString(), awkwardName);

            const auto device = juce::JSON::parse (BackendJson::deviceBody (userId, vendorId, productId, awkwardName, keyCount));
            expect (device.isObject());
            expectEquals (device[deviceNameKey.toRawUTF8()].toString(), awkwardName);
        }

        beginTest (loginReplies);
        {
            const auto signedIn = BackendJson::readLoginReply (tokenReply);
            expect (signedIn.outcome == Login::signedIn);
            expectEquals (signedIn.accessToken, token);
            expectEquals (signedIn.userId, userId);

            const auto described = BackendJson::readLoginReply (descriptionReply);
            expect (described.outcome == Login::rejected);
            expectEquals (described.message, description);   // the description wins over the error code

            const auto message = BackendJson::readLoginReply (messageReply);
            expect (message.outcome == Login::rejected);
            expectEquals (message.message, messageText);

            const auto wrongPassword = BackendJson::readLoginReply (wrongPasswordReply);
            expect (wrongPassword.outcome == Login::rejected);
            expectEquals (wrongPassword.message, description);

            expectEquals (BackendJson::connectionFailedBody(), connectionReply);   // what SupabaseClient reports offline
            expect (BackendJson::readLoginReply (connectionReply).outcome == Login::noConnection);

            expect (BackendJson::readLoginReply (confirmReply).outcome == Login::confirmEmail);
            expect (BackendJson::readLoginReply (unconfirmedReply).outcome == Login::confirmEmail);
            expect (BackendJson::readLoginReply (strangeReply).outcome == Login::unexpected);
            expect (BackendJson::readLoginReply (junkReply).outcome == Login::unreadable);
            expect (BackendJson::readLoginReply (emptyReply).outcome == Login::unreadable);
        }

        beginTest (signupReplies);
        {
            expect (BackendJson::readSignupReply (sessionReply).outcome == Signup::created);
            expect (BackendJson::readSignupReply (userReply).outcome == Signup::created);
            expect (BackendJson::readSignupReply (confirmSentReply).outcome == Signup::createdCheckEmail);

            expect (BackendJson::readSignupReply (connectionReply).outcome == Signup::noConnection);

            const auto rejected = BackendJson::readSignupReply (signupErrorReply);
            expect (rejected.outcome == Signup::rejected);
            expectEquals (rejected.message, signupErrorText);

            expect (BackendJson::readSignupReply (strangeReply).outcome == Signup::notCreated);
            expect (BackendJson::readSignupReply (junkReply).outcome == Signup::unreadable);
            expect (BackendJson::readSignupReply (emptyReply).outcome == Signup::unreadable);   // what signup() returned before
        }

        beginTest (rememberedEmail);
        {
            juce::PropertySet settings;
            expect (RememberedEmail::load (&settings).isEmpty());
            RememberedEmail::save (&settings, email);
            expectEquals (RememberedEmail::load (&settings), email);
            RememberedEmail::forget (&settings);
            expect (RememberedEmail::load (&settings).isEmpty());
            RememberedEmail::save (&settings, email);
            RememberedEmail::save (&settings, {});
            expect (! settings.containsKey (lastEmailKey));                // an empty email leaves no key behind
            expect (RememberedEmail::load (nullptr).isEmpty());   // no settings file: nothing remembered, no crash
            RememberedEmail::save (nullptr, email);
            RememberedEmail::forget (nullptr);
        }

        beginTest (rememberedRow);
        {
            constexpr bool logIn  = false;
            constexpr bool signUp = true;
            expect (RememberedEmail::showRow (email, email, logIn));
            expect (! RememberedEmail::showRow (email, email, signUp));
            expect (! RememberedEmail::showRow (email, otherEmail, logIn));        // typed another address
            expect (! RememberedEmail::showRow ({}, {}, logIn));                    // nothing remembered
        }
    }
};

static BackendJsonTest backendJsonTest;
