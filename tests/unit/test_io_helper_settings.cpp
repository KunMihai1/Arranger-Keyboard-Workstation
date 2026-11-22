#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "IOHelper.h"

namespace IOHelperSettingsTestValues
{
    const juce::String playbackTestName = "PlaybackSettingsIOHelper";
    const juce::String effectsTestName  = "EffectSettingsIOHelper";
    const juce::String sectionTestName  = "SectionIOHelper";
    const juce::String testCategory     = "Unit";

    const juce::String playbackRoundTrip        = "Save and load roundtrip - full range keyboard";
    const juce::String wrongDeviceGivesDefaults = "Load with wrong VID/PID returns defaults";
    const juce::String emptyGivesDefaults       = "Load from empty properties returns defaults";
    const juce::String smallKeyboardClamps      = "Save with small keyboard range clamps to base octave";
    const juce::String effectsRoundTrip         = "Save and load roundtrip uses style and device key";
    const juce::String sectionsRoundTrip        = "Save and load roundtrip";
    const juce::String missingFileGivesEmpty    = "Load from non-existent file gives empty map";
    const juce::String invalidJsonGivesEmpty    = "Load from empty/invalid JSON gives empty map";
    const juce::String severalStylesRoundTrip   = "Multiple styles save and load correctly";

    const juce::String styleKey       = "style_default";
    const juce::String vendorId       = "1234";
    const juce::String productId      = "5678";
    const juce::String otherVendorId  = "9999";
    const juce::String otherProductId = "0000";
    const juce::String smallVendorId  = "AAAA";
    const juce::String smallProductId = "BBBB";

    // Playback ranges (MIDI notes).
    constexpr int startNote       = 36;
    constexpr int endNote         = 96;
    constexpr int leftHandBound   = 60;
    constexpr int rightHandBound  = 72;
    constexpr int lowestKey       = 21;    // an 88-key keyboard
    constexpr int highestKey      = 108;
    constexpr int notSet          = -1;
    constexpr int smallStartNote  = 48;    // C3
    constexpr int smallEndNote    = 72;    // C5
    constexpr int smallLeftBound  = 55;
    constexpr int smallRightBound = 65;
    constexpr int baseOctaveC     = 60;    // a small keyboard's notes map onto the octave above middle C
    constexpr int semitonesPerOctave = 12;

    // Effect values per device.
    constexpr int leftHandChannel   = 1;
    constexpr int rightHandChannel  = 16;
    constexpr int deviceOneReverb   = 25;
    constexpr int deviceOneVolume   = 90;
    constexpr int deviceOneBrightness = 100;
    constexpr int deviceTwoReverb   = 75;
    constexpr int deviceTwoVolume   = 30;
    constexpr int deviceTwoBrightness = 40;
    constexpr int savedReverb       = 12;
    constexpr int savedVolume       = 34;
    constexpr int defaultEffectValue = 64;

    // Sections.
    const juce::String jsonExtension = ".json";
    const juce::String missingSectionFile = "C:\\this_section_file_does_not_exist.json";
    const juce::String invalidJson   = "not valid json";
    const juce::String styleA        = "style_A";
    const juce::String introId       = "s1";
    const juce::String introName     = "Intro";
    const juce::String verseId       = "s2";
    const juce::String verseName     = "Verse";
    const juce::String rockStyle     = "Rock";
    const juce::String jazzStyle     = "Jazz";
    const juce::String partAId       = "a1";
    const juce::String partAName     = "PartA";
    const juce::String partBId       = "b1";
    const juce::String partBName     = "PartB";

    constexpr double sectionStart    = 0.0;
    constexpr double introEnd        = 8.5;
    constexpr double verseEnd        = 25.0;
    constexpr double partAEnd        = 10.0;
    constexpr double partBEnd        = 5.0;
    constexpr int    firstBar        = 1;
    constexpr int    introEndBar     = 4;
    constexpr int    verseStartBar   = 5;
    constexpr int    verseEndBar     = 12;
    constexpr int    partAEndBar     = 8;
    constexpr int    partBEndBar     = 4;
    constexpr double timeTolerance   = 0.001;
    constexpr size_t present         = 1;
}

using namespace IOHelperSettingsTestValues;

class PlaybackSettingsIOHelperTest : public juce::UnitTest
{
public:
    PlaybackSettingsIOHelperTest() : juce::UnitTest(IOHelperSettingsTestValues::playbackTestName, IOHelperSettingsTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(playbackRoundTrip);
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = startNote;
            original.endNote = endNote;
            original.leftHandBound = leftHandBound;
            original.rightHandBound = rightHandBound;
            original.VID = vendorId;
            original.PID = productId;

            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, lowestKey, highestKey, styleKey);

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, vendorId, productId, styleKey);

            expect(loaded.startNote == startNote);
            expect(loaded.endNote == endNote);
            expect(loaded.leftHandBound == leftHandBound);
            expect(loaded.rightHandBound == rightHandBound);
            expect(loaded.VID == vendorId);
            expect(loaded.PID == productId);

        }

        beginTest(wrongDeviceGivesDefaults);
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = startNote;
            original.endNote = endNote;
            original.leftHandBound = leftHandBound;
            original.rightHandBound = rightHandBound;
            original.VID = vendorId;
            original.PID = productId;

            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, lowestKey, highestKey, styleKey);

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, otherVendorId, otherProductId, styleKey);

            expect(loaded.startNote == notSet);
            expect(loaded.endNote == notSet);

        }

        beginTest(emptyGivesDefaults);
        {
            juce::PropertySet properties;

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, vendorId, productId, styleKey);

            expect(loaded.startNote == notSet);
            expect(loaded.endNote == notSet);
            expect(loaded.leftHandBound == notSet);
            expect(loaded.rightHandBound == notSet);

        }

        beginTest(smallKeyboardClamps);
        {
            juce::PropertySet properties;

            PlayBackSettings original;
            original.startNote = smallStartNote;   // C3
            original.endNote = smallEndNote;       // C5
            original.leftHandBound = smallLeftBound;
            original.rightHandBound = smallRightBound;
            original.VID = smallVendorId;
            original.PID = smallProductId;

            // highest - lowest = 72 - 48 = 24, which is < 49, so clamping kicks in
            PlaybackSettingsIOHelper::savePlaybackSettings(&properties, original, smallStartNote, smallEndNote, styleKey);

            PlayBackSettings loaded = PlaybackSettingsIOHelper::loadPlaybackSettings(&properties, smallVendorId, smallProductId, styleKey);

            // Values should be remapped to base 60:  60 + (note % 12)
            expect(loaded.startNote == baseOctaveC + (smallStartNote % semitonesPerOctave));
            expect(loaded.endNote == baseOctaveC + (smallEndNote % semitonesPerOctave));

        }
    }
};

static PlaybackSettingsIOHelperTest playbackSettingsIOHelperTest;



class EffectSettingsIOHelperTest : public juce::UnitTest
{
public:
    EffectSettingsIOHelperTest() : juce::UnitTest(IOHelperSettingsTestValues::effectsTestName, IOHelperSettingsTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(effectsRoundTrip);
        {
            juce::PropertySet properties;

            SoundSettings deviceOne;
            deviceOne.reverb = deviceOneReverb;
            deviceOne.volume = deviceOneVolume;
            deviceOne.brightness = deviceOneBrightness;

            SoundSettings deviceTwo;
            deviceTwo.reverb = deviceTwoReverb;
            deviceTwo.volume = deviceTwoVolume;
            deviceTwo.brightness = deviceTwoBrightness;

            EffectSettingsIOHelper::saveEffectsStyle(&properties, styleKey, vendorId, productId, leftHandChannel, deviceOne);
            EffectSettingsIOHelper::saveEffectsStyle(&properties, styleKey, otherVendorId, otherProductId, leftHandChannel, deviceTwo);

            SoundSettings loadedOne = EffectSettingsIOHelper::loadEffectsStyle(&properties, styleKey, vendorId, productId, leftHandChannel);
            SoundSettings loadedTwo = EffectSettingsIOHelper::loadEffectsStyle(&properties, styleKey, otherVendorId, otherProductId, leftHandChannel);

            expect(loadedOne.reverb == deviceOneReverb);
            expect(loadedOne.volume == deviceOneVolume);
            expect(loadedOne.brightness == deviceOneBrightness);

            expect(loadedTwo.reverb == deviceTwoReverb);
            expect(loadedTwo.volume == deviceTwoVolume);
            expect(loadedTwo.brightness == deviceTwoBrightness);

        }

        beginTest(wrongDeviceGivesDefaults);
        {
            juce::PropertySet properties;

            SoundSettings original;
            original.reverb = savedReverb;
            original.volume = savedVolume;

            EffectSettingsIOHelper::saveEffectsStyle(&properties, styleKey, vendorId, productId, rightHandChannel, original);

            SoundSettings loaded = EffectSettingsIOHelper::loadEffectsStyle(&properties, styleKey, otherVendorId, otherProductId, rightHandChannel);

            expect(loaded.reverb == defaultEffectValue);
            expect(loaded.volume == defaultEffectValue);

        }
    }
};

static EffectSettingsIOHelperTest effectSettingsIOHelperTest;



class SectionIOHelperTest : public juce::UnitTest
{
public:
    SectionIOHelperTest() : juce::UnitTest(IOHelperSettingsTestValues::sectionTestName, IOHelperSettingsTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(sectionsRoundTrip);
        {
            auto tempFile = juce::File::createTempFile(jsonExtension);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> original;

            StyleSection intro;
            intro.id = introId;
            intro.name = introName;
            intro.startTimeSeconds = sectionStart;
            intro.endTimeSeconds = introEnd;
            intro.startBar = firstBar;
            intro.endBar = introEndBar;

            StyleSection verse;
            verse.id = verseId;
            verse.name = verseName;
            verse.startTimeSeconds = introEnd;
            verse.endTimeSeconds = verseEnd;
            verse.startBar = verseStartBar;
            verse.endBar = verseEndBar;

            original[styleA][introName] = intro;
            original[styleA][verseName] = verse;

            SectionIOHelper::saveToFile(tempFile, original);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);

            expect(loaded.count(styleA) == present);
            expect(loaded[styleA].count(introName) == present);
            expect(loaded[styleA].count(verseName) == present);

            expect(loaded[styleA][introName].id == introId);
            expect(loaded[styleA][introName].name == introName);
            expect(std::abs(loaded[styleA][introName].startTimeSeconds - sectionStart) < timeTolerance);
            expect(std::abs(loaded[styleA][introName].endTimeSeconds - introEnd) < timeTolerance);
            expect(loaded[styleA][introName].startBar == firstBar);
            expect(loaded[styleA][introName].endBar == introEndBar);

            expect(loaded[styleA][verseName].id == verseId);
            expect(loaded[styleA][verseName].startBar == verseStartBar);

            tempFile.deleteFile();
        }

        beginTest(missingFileGivesEmpty);
        {
            juce::File nonExistent(missingSectionFile);
            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(nonExistent, loaded);
            expect(loaded.empty());
        }

        beginTest(invalidJsonGivesEmpty);
        {
            auto tempFile = juce::File::createTempFile(jsonExtension);
            tempFile.replaceWithText(invalidJson);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);
            expect(loaded.empty());

            tempFile.deleteFile();
        }

        beginTest(severalStylesRoundTrip);
        {
            auto tempFile = juce::File::createTempFile(jsonExtension);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> original;

            StyleSection partA;
            partA.id = partAId; partA.name = partAName;
            partA.startTimeSeconds = sectionStart; partA.endTimeSeconds = partAEnd;
            partA.startBar = firstBar; partA.endBar = partAEndBar;

            StyleSection partB;
            partB.id = partBId; partB.name = partBName;
            partB.startTimeSeconds = sectionStart; partB.endTimeSeconds = partBEnd;
            partB.startBar = firstBar; partB.endBar = partBEndBar;

            original[rockStyle][partAName] = partA;
            original[jazzStyle][partBName] = partB;

            SectionIOHelper::saveToFile(tempFile, original);

            std::unordered_map<juce::String, std::unordered_map<juce::String, StyleSection>> loaded;
            SectionIOHelper::loadFromFile(tempFile, loaded);

            expect(loaded.count(rockStyle) == present);
            expect(loaded.count(jazzStyle) == present);
            expect(loaded[rockStyle][partAName].id == partAId);
            expect(loaded[jazzStyle][partBName].id == partBId);

            tempFile.deleteFile();
        }
    }
};

static SectionIOHelperTest sectionIOHelperTest;
