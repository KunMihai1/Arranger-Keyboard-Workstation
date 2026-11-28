#include "BackendJson.h"

namespace
{
    // Request fields (the Supabase functions' names, persisted on the server: keep the exact text).
    namespace RequestKeys
    {
        const juce::Identifier email      { "email" };
        const juce::Identifier password   { "password" };
        const juce::Identifier data       { "data" };
        const juce::Identifier name       { "name" };
        const juce::Identifier userId     { "user_id" };
        const juce::Identifier productId  { "pid" };
        const juce::Identifier vendorId   { "vid" };
        const juce::Identifier seconds    { "seconds" };
        const juce::Identifier vendorIdLong  { "vendor_id" };
        const juce::Identifier productIdLong { "product_id" };
        const juce::Identifier deviceName { "device_name" };
        const juce::Identifier keyCount   { "nr_keys" };
    }

    // Fields of the auth server's replies.
    namespace ReplyKeys
    {
        const juce::Identifier error                { "error" };
        const juce::Identifier errorDescription     { "error_description" };
        const juce::Identifier message              { "msg" };
        const juce::Identifier errorCode            { "error_code" };
        const juce::String     emailNotConfirmed    = "email_not_confirmed";
        const juce::Identifier confirmationRequired { "confirmation_required" };
        const juce::Identifier user                 { "user" };
        const juce::Identifier id                   { "id" };
        const juce::Identifier accessToken          { "access_token" };
        const juce::Identifier confirmationSentAt   { "confirmation_sent_at" };
        const juce::String     trueText             = "true";
    }

    const juce::String connectionFailedReply = "{\"error\":\"connection failed\"}";

    juce::String compact (juce::DynamicObject* object)
    {
        return juce::JSON::toString (juce::var (object), juce::JSON::FormatOptions{}.withSpacing (juce::JSON::Spacing::none));
    }

    /** The first of the given fields the reply has, as text ("" when it has none). */
    juce::String firstText (const juce::DynamicObject& reply, std::initializer_list<juce::Identifier> keys)
    {
        for (const auto& key : keys)
            if (reply.hasProperty (key))
                return reply.getProperty (key).toString();
        return {};
    }
}

juce::String BackendJson::connectionFailedBody()
{
    return connectionFailedReply;
}

juce::String BackendJson::loginBody (const juce::String& email, const juce::String& password)
{
    auto* body = new juce::DynamicObject();
    body->setProperty (RequestKeys::email, email);
    body->setProperty (RequestKeys::password, password);
    return compact (body);
}

juce::String BackendJson::signupBody (const juce::String& email, const juce::String& password, const juce::String& username)
{
    auto* profile = new juce::DynamicObject();
    profile->setProperty (RequestKeys::name, username);

    auto* body = new juce::DynamicObject();
    body->setProperty (RequestKeys::email, email);
    body->setProperty (RequestKeys::password, password);
    body->setProperty (RequestKeys::data, juce::var (profile));
    return compact (body);
}

juce::String BackendJson::playtimeBody (const juce::String& userId, const juce::String& productId, const juce::String& vendorId, int seconds)
{
    auto* body = new juce::DynamicObject();
    body->setProperty (RequestKeys::userId, userId);
    body->setProperty (RequestKeys::productId, productId);
    body->setProperty (RequestKeys::vendorId, vendorId);
    body->setProperty (RequestKeys::seconds, seconds);
    return compact (body);
}

juce::String BackendJson::deviceBody (const juce::String& userId, const juce::String& vendorId, const juce::String& productId,
                                      const juce::String& deviceName, int keyCount)
{
    auto* body = new juce::DynamicObject();
    body->setProperty (RequestKeys::userId, userId);
    body->setProperty (RequestKeys::vendorIdLong, vendorId);
    body->setProperty (RequestKeys::productIdLong, productId);
    body->setProperty (RequestKeys::deviceName, deviceName);
    body->setProperty (RequestKeys::keyCount, keyCount);
    return compact (body);
}

BackendJson::LoginReply BackendJson::readLoginReply (const juce::String& body)
{
    LoginReply reply;
    if (body == connectionFailedReply)
    {
        reply.outcome = LoginReply::Outcome::noConnection;
        return reply;
    }

    const auto parsed = juce::JSON::parse (body);
    const auto* object = parsed.getDynamicObject();
    if (object == nullptr)
        return reply;

    // The live server says "not confirmed" as an error code; older replies carried confirmation_required.
    if (object->getProperty (ReplyKeys::errorCode).toString() == ReplyKeys::emailNotConfirmed)
    {
        reply.outcome = LoginReply::Outcome::confirmEmail;
        return reply;
    }

    if (object->hasProperty (ReplyKeys::error) || object->hasProperty (ReplyKeys::errorDescription) || object->hasProperty (ReplyKeys::message))
    {
        reply.outcome = LoginReply::Outcome::rejected;
        reply.message = firstText (*object, { ReplyKeys::errorDescription, ReplyKeys::error, ReplyKeys::message });
        return reply;
    }

    if (object->getProperty (ReplyKeys::confirmationRequired).toString() == ReplyKeys::trueText)
    {
        reply.outcome = LoginReply::Outcome::confirmEmail;
        return reply;
    }

    if (! object->hasProperty (ReplyKeys::accessToken))
    {
        reply.outcome = LoginReply::Outcome::unexpected;
        return reply;
    }

    reply.outcome     = LoginReply::Outcome::signedIn;
    reply.accessToken = object->getProperty (ReplyKeys::accessToken).toString();
    if (const auto* user = object->getProperty (ReplyKeys::user).getDynamicObject())
        reply.userId = user->getProperty (ReplyKeys::id).toString();
    return reply;
}

BackendJson::SignupReply BackendJson::readSignupReply (const juce::String& body)
{
    SignupReply reply;
    if (body == connectionFailedReply)
    {
        reply.outcome = SignupReply::Outcome::noConnection;
        return reply;
    }

    const auto parsed = juce::JSON::parse (body);
    const auto* object = parsed.getDynamicObject();
    if (object == nullptr)
        return reply;

    if (object->hasProperty (ReplyKeys::error) || object->hasProperty (ReplyKeys::errorDescription))
    {
        reply.outcome = SignupReply::Outcome::rejected;
        reply.message = firstText (*object, { ReplyKeys::errorDescription, ReplyKeys::error });
        return reply;
    }

    const bool isSession = object->hasProperty (ReplyKeys::accessToken);
    const bool isUser    = object->hasProperty (ReplyKeys::id);
    if (! isSession && ! isUser)
        reply.outcome = SignupReply::Outcome::notCreated;
    else if (isUser && object->hasProperty (ReplyKeys::confirmationSentAt))
        reply.outcome = SignupReply::Outcome::createdCheckEmail;
    else
        reply.outcome = SignupReply::Outcome::created;
    return reply;
}
