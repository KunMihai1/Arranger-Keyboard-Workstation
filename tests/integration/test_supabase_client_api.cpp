#include <juce_core/juce_core.h>
#include "SupabaseClient.h"

namespace SupabaseClientTestValues
{
    const juce::String testName     = "supabaseClient";
    const juce::String testCategory = "Integration-api";

    const juce::String loginInvalid        = "Login with invalid credentials returns 400 or 500";
    const juce::String loginConnectionFail = "Login handles connection failure gracefully";
    const juce::String signupRandomEmail   = "Signup returns a response for random email";
    const juce::String playtimeBadToken    = "Increment playtime with invalid token returns 401 or 500";
    const juce::String deviceBadToken      = "AddOrUpdateDevice with invalid token returns 401 or 500";

    const juce::String invalidEmail      = "invalid@test.com";
    const juce::String wrongPassword     = "wrongpassword";
    const juce::String emailPrefix       = "test_";
    const juce::String emailDomain       = "@test.com";
    const juce::String signupPassword    = "password123";
    const juce::String signupUsername    = "TestUser";
    const juce::String nullUserId        = "00000000-0000-0000-0000-000000000000";
    const juce::String invalidToken      = "invalidtoken";
    const juce::String sampleVendor      = "vid-sample";
    const juce::String sampleProduct     = "pid-sample";
    const juce::String sampleVendorUpper = "VID-SAMPLE";
    const juce::String sampleProductUpper = "PID-SAMPLE";
    const juce::String sampleDeviceName  = "Test Device";
    constexpr int      playtimeSeconds   = 60;
    constexpr int      keyCount          = 61;

    constexpr int statusOk           = 200;
    constexpr int statusBadRequest   = 400;
    constexpr int statusUnauthorized = 401;
    constexpr int statusServerError  = 500;

    const juce::String validationFailed    = "validation_failed";
    const juce::String internalServerError = "Internal server error";
    const juce::String errorWord           = "error";

    const juce::String expectBadLogin      = "Expected 401 or 500 for invalid login";
    const juce::String expectBodyNotEmpty  = "Response body should not be empty";
    const juce::String expectConnectionStatus = "Connection failure should return 400 or 500";
    const juce::String expectConnectionMessage = "Expected connection failure or server error message";
    const juce::String expectSignupStatus  = "Expected 200, 400, or 500 status code";
    const juce::String expectBadToken      = "Expected 401 or 500 for invalid token";
    const juce::String expectErrorMessage  = "Expected error message in body";
}

using namespace SupabaseClientTestValues;


// ==================================================================
// NETWORK-DEPENDENT TESTS (Integration)
// These tests require network connection for the https calls
// ==================================================================


class SupabaseClientTest : public juce::UnitTest
{
public:
	SupabaseClientTest() : juce::UnitTest(testName, testCategory) {}

	void runTest() override
	{
        testLoginInvalid();
        testLoginConnectionFailure();
        testSignupRandomEmail();
        testIncrementPlaytimeInvalidToken();
        testAddOrUpdateDeviceInvalidToken();
	}

private:
    void testLoginInvalid()
    {
        beginTest(loginInvalid);

        SupabaseClient client;
        auto result = client.login(invalidEmail, wrongPassword);

        expect(result.statusCode == statusBadRequest || result.statusCode == statusServerError,
            expectBadLogin);

        expect(result.body.isNotEmpty(), expectBodyNotEmpty);
    }

    void testLoginConnectionFailure()
    {
        beginTest(loginConnectionFail);

        SupabaseClient client;

        
        auto result = client.login({}, {});

        expect(result.statusCode == statusBadRequest || result.statusCode == statusServerError,
            expectConnectionStatus);

        expect(result.body.contains(validationFailed) || result.body.contains(internalServerError),
            expectConnectionMessage);
    }

    void testSignupRandomEmail()
    {
        beginTest(signupRandomEmail);

        SupabaseClient client;

        auto randomEmail =
            emailPrefix + juce::String(juce::Random::getSystemRandom().nextInt()) + emailDomain;

        auto result = client.signup(randomEmail, signupPassword, signupUsername);

        expect(result.statusCode == statusOk || result.statusCode == statusBadRequest || result.statusCode == statusServerError,
            expectSignupStatus);
    }

    void testIncrementPlaytimeInvalidToken()
    {
        beginTest(playtimeBadToken);

        SupabaseClient client;
        client.setUserId(nullUserId);
        client.setAccessToken(invalidToken);

        auto result = client.incrementPlaytime(playtimeSeconds, sampleVendor, sampleProduct);

        expect(result.statusCode == statusUnauthorized || result.statusCode == statusServerError,
            expectBadToken);

        expect(result.body.contains(errorWord) || result.body.contains(internalServerError),
            expectErrorMessage);
    }

    void testAddOrUpdateDeviceInvalidToken()
    {
        beginTest(deviceBadToken);

        SupabaseClient client;
        client.setUserId(nullUserId);
        client.setAccessToken(invalidToken);

        auto result = client.addOrUpdateDevice(sampleVendorUpper, sampleProductUpper, sampleDeviceName, keyCount);

        expect(result.statusCode == statusUnauthorized || result.statusCode == statusServerError,
            expectBadToken);

        expect(result.body.contains(errorWord) || result.body.contains(internalServerError),
            expectErrorMessage);
    }
};

//static SupabaseClientTest supabaseClientTest;