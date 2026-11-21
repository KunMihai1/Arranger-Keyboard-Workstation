#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "IOHelper.h"





class PlaybackSettingsIOHelperTest : public juce::UnitTest
{
public:
    PlaybackSettingsIOHelperTest() : juce::UnitTest("PlaybackSettingsIOHelper", "Unit") {}

    void runTest() override
    {
        beginTest("Save and load roundtrip - full range keyboard");
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = 36;
            original.endNote = 96;
            original.leftHandBound = 60;
            original.rightHandBound = 72;
            original.VID = "1234";
            original.PID = "5678";

            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, 21, 108, "style_default");

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, "1234", "5678", "style_default");

            expect(loaded.startNote == 36);
            expect(loaded.endNote == 96);
            expect(loaded.leftHandBound == 60);
            expect(loaded.rightHandBound == 72);
            expect(loaded.VID == "1234");
            expect(loaded.PID == "5678");

        }

        beginTest("Load with wrong VID/PID returns defaults");
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = 36;
            original.endNote = 96;
            original.leftHandBound = 60;
            original.rightHandBound = 72;
            original.VID = "1234";
            original.PID = "5678";

            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, 21, 108, "style_default");

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, "9999", "0000", "style_default");

            expect(loaded.startNote == -1);
            expect(loaded.endNote == -1);

        }

        beginTest("Load from empty properties returns defaults");
        {
            juce::PropertySet properties;

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, "1234", "5678", "style_default");

            expect(loaded.startNote == -1);
            expect(loaded.endNote == -1);
            expect(loaded.leftHandBound == -1);
            expect(loaded.rightHandBound == -1);

        }

        beginTest("Save with small keyboard range clamps to base octave");
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = 48;   // C3
            original.endNote = 72;     // C5
            original.leftHandBound = 55;
            original.rightHandBound = 65;
            original.VID = "AAAA";
            original.PID = "BBBB";

            // highest - lowest = 72 - 48 = 24, which is < 49, so clamping kicks in
            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, 48, 72, "style_default");

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, "AAAA", "BBBB", "style_default");

            // Values should be remapped to base 60:  60 + (note % 12)
            expect(loaded.startNote == 60 + (48 % 12)); 
            expect(loaded.endNote == 60 + (72 % 12));    

        }
    }
};

static PlaybackSettingsIOHelperTest playbackSettingsIOHelperTest;



class EffectSettingsIOHelperTest : public juce::UnitTest
{
public:
    EffectSettingsIOHelperTest() : juce::UnitTest("EffectSettingsIOHelper", "Unit") {}

    void runTest() override
    {
        beginTest("Save and load roundtrip uses style and device key");
        {
            juce::PropertySet properties;

            SoundSettings deviceOne;
            deviceOne.reverb = 25;
            deviceOne.volume = 90;
            deviceOne.brightness = 100;

            SoundSettings deviceTwo;
            deviceTwo.reverb = 75;
            deviceTwo.volume = 30;
            deviceTwo.brightness = 40;

            EffectSettingsIOHelper::saveEffectsStyle(&properties, "style_default", "1234", "5678", 1, deviceOne);
            EffectSettingsIOHelper::saveEffectsStyle(&properties, "style_default", "9999", "0000", 1, deviceTwo);

            SoundSettings loadedOne = EffectSettingsIOHelper::loadEffectsStyle(&properties, "style_default", "1234", "5678", 1);
            SoundSettings loadedTwo = EffectSettingsIOHelper::loadEffectsStyle(&properties, "style_default", "9999", "0000", 1);

            expect(loadedOne.reverb == 25);
            expect(loadedOne.volume == 90);
            expect(loadedOne.brightness == 100);

            expect(loadedTwo.reverb == 75);
            expect(loadedTwo.volume == 30);
            expect(loadedTwo.brightness == 40);

        }

        beginTest("Load with wrong VID/PID returns defaults");
        {
            juce::PropertySet properties;

            SoundSettings original;
            original.reverb = 12;
            original.volume = 34;

            EffectSettingsIOHelper::saveEffectsStyle(&properties, "style_default", "1234", "5678", 16, original);

            SoundSettings loaded = EffectSettingsIOHelper::loadEffectsStyle(&properties, "style_default", "9999", "0000", 16);

            expect(loaded.reverb == 64);
            expect(loaded.volume == 64);

        }
    }
};

static EffectSettingsIOHelperTest effectSettingsIOHelperTest;



class SectionIOHelperTest : public juce::UnitTest
{
public:
    SectionIOHelperTest() : juce::UnitTest("SectionIOHelper", "Unit") {}

    void runTest() override
    {
        beginTest("Save and load roundtrip");
        {
            auto tempFile = juce::File::createTempFile(".json");

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> original;

            StyleSection intro;
            intro.id = "s1";
            intro.name = "Intro";
            intro.startTimeSeconds = 0.0;
            intro.endTimeSeconds = 8.5;
            intro.startBar = 1;
            intro.endBar = 4;

            StyleSection verse;
            verse.id = "s2";
            verse.name = "Verse";
            verse.startTimeSeconds = 8.5;
            verse.endTimeSeconds = 25.0;
            verse.startBar = 5;
            verse.endBar = 12;

            original["style_A"]["Intro"] = intro;
            original["style_A"]["Verse"] = verse;

            SectionIOHelper::saveToFile(tempFile, original);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);

            expect(loaded.count("style_A") == 1);
            expect(loaded["style_A"].count("Intro") == 1);
            expect(loaded["style_A"].count("Verse") == 1);

            expect(loaded["style_A"]["Intro"].id == "s1");
            expect(loaded["style_A"]["Intro"].name == "Intro");
            expect(std::abs(loaded["style_A"]["Intro"].startTimeSeconds - 0.0) < 0.001);
            expect(std::abs(loaded["style_A"]["Intro"].endTimeSeconds - 8.5) < 0.001);
            expect(loaded["style_A"]["Intro"].startBar == 1);
            expect(loaded["style_A"]["Intro"].endBar == 4);

            expect(loaded["style_A"]["Verse"].id == "s2");
            expect(loaded["style_A"]["Verse"].startBar == 5);

            tempFile.deleteFile();
        }

        beginTest("Load from non-existent file gives empty map");
        {
            juce::File nonExistent("C:\\this_section_file_does_not_exist.json");
            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(nonExistent, loaded);
            expect(loaded.empty());
        }

        beginTest("Load from empty/invalid JSON gives empty map");
        {
            auto tempFile = juce::File::createTempFile(".json");
            tempFile.replaceWithText("not valid json");

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);
            expect(loaded.empty());

            tempFile.deleteFile();
        }

        beginTest("Multiple styles save and load correctly");
        {
            auto tempFile = juce::File::createTempFile(".json");

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> original;

            StyleSection a;
            a.id = "a1"; a.name = "PartA";
            a.startTimeSeconds = 0.0; a.endTimeSeconds = 10.0;
            a.startBar = 1; a.endBar = 8;

            StyleSection b;
            b.id = "b1"; b.name = "PartB";
            b.startTimeSeconds = 0.0; b.endTimeSeconds = 5.0;
            b.startBar = 1; b.endBar = 4;

            original["Rock"]["PartA"] = a;
            original["Jazz"]["PartB"] = b;

            SectionIOHelper::saveToFile(tempFile, original);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);

            expect(loaded.count("Rock") == 1);
            expect(loaded.count("Jazz") == 1);
            expect(loaded["Rock"]["PartA"].id == "a1");
            expect(loaded["Jazz"]["PartB"].id == "b1");

            tempFile.deleteFile();
        }
    }
};

static SectionIOHelperTest sectionIOHelperTest;
