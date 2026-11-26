#include "SettingsWindow.h"
#include "MidiHandler.h"
#include "AppColours.h"
#include "IOHelper.h"
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace Text
    {
        const juce::String outputEngine = "Output engine";
        const juce::String midiIn       = "MIDI in";
        const juce::String midiOut      = "MIDI out";
        const juce::String audioOut     = "Audio out (internal SFZ synth)";
        const juce::String keyboard     = "Keyboard";
        const juce::String chordMode    = "Chord mode";
        const juce::String minorScale   = "Minor scale";
        const juce::String externalMidi = "External MIDI";        // OutputEngineIds::externalMidi
        const juce::String internalSfz  = "Internal SFZ synth";   // OutputEngineIds::internalSfz
        const juce::String keysSuffix   = " keys";
        const juce::String midiHeading     = "MIDI";
        const juce::String arrangerHeading = "Arranger";
        const juce::String chordsHeading   = "Chords";
        const juce::String displayHeading  = "Display";
        const juce::String noValue      = "-";                    // no key count for this device (the LCD font has no dash glyph)
    }

    constexpr int   deviceRefreshMs     = 750;
    constexpr int   firstListIndex      = 0;
    constexpr int   firstChordMode      = 0;     // ChordMode values as stored
    constexpr int   lastChordMode       = 2;
    constexpr int   unknownKeyCount     = 0;
    constexpr size_t keyboardSizeCount  = 4;
    constexpr int    keys49 = 49;   // the supported keyboard sizes (today's "Update key count" accepted these)
    constexpr int    keys61 = 61;
    constexpr int    keys76 = 76;
    constexpr int    keys88 = 88;
    const std::array<int, keyboardSizeCount> keyboardSizes { keys49, keys61, keys76, keys88 };

    // Board StageRevised openPage=settings, scale 1, relative to the page content (28, 180).
    namespace Board
    {
        constexpr int columnGap      = 32;
        constexpr int columnGaps     = 3;    // between four columns
        constexpr size_t midiColumn     = 0;
        constexpr size_t arrangerColumn = 1;
        constexpr size_t chordsColumn   = 2;
        constexpr size_t displayColumn  = 3;
        constexpr int labelHeight    = 12;
        constexpr int fieldGap       = 8;    // MIDI and Chords columns: label, gap, field, gap, label ...
        constexpr int rowGap         = 6;    // Arranger and Display columns: rows 22 high, 6 apart
        constexpr int keyboardGap    = 6;    // Keyboard label to its field, field to Add device
        constexpr int displayExtra   = 6;    // "Particle style" sits 6 lower (margin-top)
        constexpr int addDeviceWidth = 88;   // 87.84: "Add device..." at the seg size
    }
}

//==============================================================================
SettingsPageRects settingsPageLayout (juce::Rectangle<int> content, float scale)
{
    using namespace StageMetrics;
    auto scaled = [scale] (int boardLength) { return static_cast<float> (boardLength) * scale; };
    const auto area = content.toFloat();
    const auto columnWidth = (area.getWidth() - scaled (Board::columnGap) * static_cast<float> (Board::columnGaps))
                             / static_cast<float> (SettingsPageRects::columnCount);

    SettingsPageRects rects;
    std::array<juce::Rectangle<float>, SettingsPageRects::columnCount> columns;
    for (int column {}; column < SettingsPageRects::columnCount; ++column)
    {
        columns[static_cast<size_t> (column)] = { area.getX() + static_cast<float> (column) * (columnWidth + scaled (Board::columnGap)),
                                                  area.getY(), columnWidth, area.getHeight() };
        rects.columns[static_cast<size_t> (column)] = columns[static_cast<size_t> (column)].toNearestInt();
        rects.headings[static_cast<size_t> (column)] = columns[static_cast<size_t> (column)].withHeight (scaled (headingHeight)).toNearestInt();
    }

    // MIDI: label, field, label, field ... then Keyboard [field][Add device...]
    auto midi = columns[Board::midiColumn];
    auto top = midi.getY() + scaled (headingHeight + Board::fieldGap);
    for (size_t field {}; field < rects.fields.size(); ++field)
    {
        rects.fieldLabels[field] = juce::Rectangle<float> (midi.getX(), top, columnWidth, scaled (Board::labelHeight)).toNearestInt();
        top += scaled (Board::labelHeight + Board::fieldGap);
        rects.fields[field] = juce::Rectangle<float> (midi.getX(), top, columnWidth, scaled (fieldHeight)).toNearestInt();
        top += scaled (fieldHeight + Board::fieldGap);
    }
    rects.keyboardLabel = juce::Rectangle<float> (midi.getX(), top, columnWidth, scaled (Board::labelHeight)).toNearestInt();
    top += scaled (Board::labelHeight + Board::keyboardGap);
    rects.keyboardRow = juce::Rectangle<float> (midi.getX(), top, columnWidth, scaled (fieldHeight)).toNearestInt();

    // Arranger: seven LED rows.
    const auto arranger = columns[Board::arrangerColumn];
    for (size_t row {}; row < rects.arrangerToggles.size(); ++row)
        rects.arrangerToggles[row] = juce::Rectangle<float> (arranger.getX(),
                                                             arranger.getY() + scaled (headingHeight + Board::rowGap)
                                                                 + static_cast<float> (row) * scaled (rowToggleHeight + Board::rowGap),
                                                             columnWidth, scaled (rowToggleHeight)).toNearestInt();

    // Chords: label, seg row, label, seg row.
    const auto chords = columns[Board::chordsColumn];
    top = chords.getY() + scaled (headingHeight + Board::fieldGap);
    rects.chordModeLabel = juce::Rectangle<float> (chords.getX(), top, columnWidth, scaled (Board::labelHeight)).toNearestInt();
    top += scaled (Board::labelHeight + Board::fieldGap);
    rects.chordModeRow = juce::Rectangle<float> (chords.getX(), top, columnWidth, scaled (segHeight)).toNearestInt();
    top += scaled (segHeight + Board::fieldGap);
    rects.minorScaleLabel = juce::Rectangle<float> (chords.getX(), top, columnWidth, scaled (Board::labelHeight)).toNearestInt();
    top += scaled (Board::labelHeight + Board::fieldGap);
    rects.minorScaleRow = juce::Rectangle<float> (chords.getX(), top, columnWidth, scaled (segHeight)).toNearestInt();

    // Display: three LED rows, then "Particle style" and its seg row.
    const auto display = columns[Board::displayColumn];
    top = display.getY() + scaled (headingHeight + Board::rowGap);
    for (size_t row {}; row < rects.displayToggles.size(); ++row)
    {
        rects.displayToggles[row] = juce::Rectangle<float> (display.getX(), top, columnWidth, scaled (rowToggleHeight)).toNearestInt();
        top += scaled (rowToggleHeight + Board::rowGap);
    }
    top += scaled (Board::displayExtra);
    rects.particleStyleLabel = juce::Rectangle<float> (display.getX(), top, columnWidth, scaled (Board::labelHeight)).toNearestInt();
    top += scaled (Board::labelHeight + Board::rowGap);
    rects.particleStyleRow = juce::Rectangle<float> (display.getX(), top, columnWidth, scaled (segHeight)).toNearestInt();
    return rects;
}

std::vector<int> keyboardFieldSizes (int storedKeys)
{
    std::vector<int> sizes (keyboardSizes.begin(), keyboardSizes.end());
    if (storedKeys > unknownKeyCount && std::find (sizes.begin(), sizes.end(), storedKeys) == sizes.end())
    {
        sizes.push_back (storedKeys);
        std::sort (sizes.begin(), sizes.end());
    }
    return sizes;
}

//==============================================================================
MIDIWindow::MIDIWindow(MidiDevice& mdevice, std::vector<std::string>& devicesListIN, std::vector<std::string>& devicesListOUT
    , std::vector<std::string>& devicesListAudioOUT, juce::PropertiesFile* prop)
    : MIDIDevice{ mdevice }, devicesListIN{ devicesListIN }, devicesListOUT{ devicesListOUT }, devicesListAudioOUT{ devicesListAudioOUT }, propertyFile{ prop },
      outputEngineTypeLabel (Text::outputEngine, SilkLabel::Tone::dim),
      midiDevicesLabelIN (Text::midiIn, SilkLabel::Tone::dim),
      midiDevicesLabelOUT (Text::midiOut, SilkLabel::Tone::dim),
      audioDevicesLabelOUT (Text::audioOut, SilkLabel::Tone::dim),
      keyboardLabel (Text::keyboard, SilkLabel::Tone::dim),
      chordModeLabel (Text::chordMode, SilkLabel::Tone::dim),
      nttMinorScaleLabel (Text::minorScale, SilkLabel::Tone::dim)
{
    allInit();
    populateCBIN();
    populateCBOUT();
    populateCBaudioOUT();
    populateCBengine();
    restoreCBoxes(); // Apply saved settings
}

MIDIWindow::~MIDIWindow()
{
    stopTimer();
}

void MIDIWindow::pageShown()
{
    populateCBIN();  //selected id is set to 1 implicitly
    populateCBOUT(); //selected id is set to 1 implicitly
    restoreCBoxes();
    refreshKeyboardField();
    startTimer(deviceRefreshMs);
}

void MIDIWindow::pageHidden()
{
    stopTimer();
}

void MIDIWindow::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &this->comboBoxDevicesIN)
    {
        int index = comboBoxDevicesIN.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListIN.size())
        {
            this->MIDIDevice.setDeviceIN(index);
            lastIndexIN = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::midiInIndex, lastIndexIN); propertyFile->saveIfNeeded(); }
        }
        refreshKeyboardField();
    }
    else if (comboBoxThatHasChanged == &this->comboBoxDevicesOUT)
    {
        int index = comboBoxDevicesOUT.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListOUT.size())
        {
            this->MIDIDevice.setDeviceOUT(index);
            lastIndexOUT = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::midiOutIndex, lastIndexOUT); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &this->comboBoxOutputEngineType)
    {
        lastIndexEngineOption = comboBoxOutputEngineType.getSelectedId();
        if (propertyFile) { propertyFile->setValue(SettingsFileKeys::engineOptionIndex, lastIndexEngineOption); propertyFile->saveIfNeeded(); }
        switchBetweenEngineOptions(lastIndexEngineOption);
        if (onOutputEngineChanged && isMidiDeviceOpen && isMidiDeviceOpen())
            onOutputEngineChanged(lastIndexEngineOption);
    }
    else if(comboBoxThatHasChanged == &this->comboBoxAudioDevicesOUT)
    {
        int index = comboBoxAudioDevicesOUT.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListAudioOUT.size())
        {
            this->MIDIDevice.setDeviceAudioOUT(index);
            lastIndexAudioOut = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::audioOutIndex, lastIndexAudioOut); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &keyboardField)
    {
        // Today's "Update key count": the chosen size is stored for the MIDI-in device.
        if (setKeyCount && keyboardField.getSelectedId() != unknownKeyCount)
            setKeyCount(comboBoxDevicesIN.getText(), keyboardField.getSelectedId());
    }
}

void MIDIWindow::timerCallback()
{
    if (! isShowing())
        return;   // another page replaced Settings: nothing to refresh until it shows again

    std::vector<std::string> devicesNewIN;
    this->MIDIDevice.getAvailableDevicesMidiIN(devicesNewIN);
    if (devicesNewIN != this->devicesListIN)
    {
        if (this->comboBoxDevicesIN.isPopupActive())
            this->comboBoxDevicesIN.hidePopup();
        devicesListIN = devicesNewIN;
        populateCBIN();
        refreshKeyboardField();
    }
    std::vector<std::string> devicesNewOUT;
    this->MIDIDevice.getAvailableDevicesMidiOUT(devicesNewOUT);
    if (devicesNewOUT != this->devicesListOUT)
    {
        if (this->comboBoxDevicesOUT.isPopupActive())
            this->comboBoxDevicesOUT.hidePopup();
        devicesListOUT = devicesNewOUT;
        populateCBOUT();
    }

    std::vector<std::string> devicesNewAudioOUT;
    MIDIDevice.getAvailableAudioDevicesOUT(devicesNewAudioOUT);
    if (devicesNewAudioOUT != devicesListAudioOUT)
    {
        if (comboBoxAudioDevicesOUT.isPopupActive())
            comboBoxAudioDevicesOUT.hidePopup();

        devicesListAudioOUT = devicesNewAudioOUT;
        populateCBaudioOUT();
    }
}

void MIDIWindow::refreshKeyboardField()
{
    const int keys = getKeyCount ? getKeyCount(comboBoxDevicesIN.getText()) : unknownKeyCount;
    keyboardField.clear(juce::dontSendNotification);
    if (keys == unknownKeyCount)
    {
        keyboardField.addItem(Text::noValue, firstItemId);
        keyboardField.setSelectedId(firstItemId, juce::dontSendNotification);
        keyboardField.setEnabled(false);
    }
    else
    {
        for (const auto size : keyboardFieldSizes(keys))
            keyboardField.addItem(juce::String(size) + Text::keysSuffix, size);   // item id = key count
        keyboardField.setSelectedId(keys, juce::dontSendNotification);
        keyboardField.setEnabled(true);
    }
    keyboardField.repaint();
}

void MIDIWindow::setScale(float newScale)
{
    scale = newScale;
    for (auto* field : { &comboBoxOutputEngineType, &comboBoxDevicesIN, &comboBoxDevicesOUT, &comboBoxAudioDevicesOUT, &keyboardField })
        field->setScale(scale);
    for (auto* button : { &addDeviceButton, &chordModeFingered, &chordModeSingle, &chordModeFull,
                          &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic })
        button->setScale(scale);
    for (auto* toggle : { &arrangerModeToggle, &chordBassInversionToggle, &chordMemoryToggle, &synchroStartToggle,
                          &countInToggle, &chordZoneMuteToggle, &nttEnabledToggle })
        toggle->setScale(scale);
    for (auto* label : { &outputEngineTypeLabel, &midiDevicesLabelIN, &midiDevicesLabelOUT, &audioDevicesLabelOUT,
                         &keyboardLabel, &chordModeLabel, &nttMinorScaleLabel })
        label->setScale(scale);
    resized();
    repaint();
}

void MIDIWindow::paint(juce::Graphics& g)
{
    const auto rects = settingsPageLayout(getLocalBounds(), scale);
    const std::array<juce::String, SettingsPageRects::columnCount> headings { Text::midiHeading, Text::arrangerHeading,
                                                                             Text::chordsHeading, Text::displayHeading };
    for (size_t column {}; column < headings.size(); ++column)
        StageLcd::paintHeading(g, rects.headings[column].toFloat(), scale, headings[column], {});
}

void MIDIWindow::resized()
{
    const auto rects = settingsPageLayout(getLocalBounds(), scale);
    const std::array<SilkLabel*, SettingsPageRects::fieldCount> labels { &outputEngineTypeLabel, &midiDevicesLabelIN, &midiDevicesLabelOUT, &audioDevicesLabelOUT };
    const std::array<StageField*, SettingsPageRects::fieldCount> fields { &comboBoxOutputEngineType, &comboBoxDevicesIN, &comboBoxDevicesOUT, &comboBoxAudioDevicesOUT };
    for (size_t field {}; field < fields.size(); ++field)
    {
        labels[field]->setBounds(rects.fieldLabels[field]);
        fields[field]->setBounds(rects.fields[field]);
    }

    keyboardLabel.setBounds(rects.keyboardLabel);
    auto keyboardRow = rects.keyboardRow.toFloat();
    const auto addWidth = static_cast<float>(Board::addDeviceWidth) * scale;
    addDeviceButton.setBounds(keyboardRow.removeFromRight(addWidth).withTrimmedTop(keyboardRow.getHeight() - static_cast<float>(addDeviceButton.getPreferredHeight())).toNearestInt());
    keyboardField.setBounds(keyboardRow.withTrimmedRight(static_cast<float>(Board::keyboardGap) * scale).toNearestInt());

    const std::array<RowToggle*, SettingsPageRects::arrangerRows> toggles { &arrangerModeToggle, &chordBassInversionToggle, &chordMemoryToggle,
                                                                           &synchroStartToggle, &countInToggle, &chordZoneMuteToggle, &nttEnabledToggle };
    for (size_t row {}; row < toggles.size(); ++row)
        toggles[row]->setBounds(toggles[row]->boundsForBoardBox(rects.arrangerToggles[row]));

    chordModeLabel.setBounds(rects.chordModeLabel);
    nttMinorScaleLabel.setBounds(rects.minorScaleLabel);
    auto placeRow = [] (juce::Rectangle<int> row, std::initializer_list<SegButton*> buttons)
    {
        auto left = static_cast<float>(row.getX());   // adjacent, as the board
        for (auto* button : buttons)
        {
            button->setBounds(juce::Rectangle<float>(left, static_cast<float>(row.getY()), button->getExactWidth(),
                                                     static_cast<float>(row.getHeight())).toNearestInt());
            left += button->getExactWidth();
        }
    };
    placeRow(rects.chordModeRow, { &chordModeFingered, &chordModeSingle, &chordModeFull });
    placeRow(rects.minorScaleRow, { &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic });
}

void MIDIWindow::refreshChoiceLeds()
{
    for (auto* button : { &chordModeFingered, &chordModeSingle, &chordModeFull, &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic })
        button->setLedOn(button->getToggleState());
}

void MIDIWindow::allInit()
{
    if (propertyFile != nullptr)
    {
        lastIndexIN = propertyFile->getIntValue(SettingsFileKeys::midiInIndex, firstItemId);
        lastIndexOUT = propertyFile->getIntValue(SettingsFileKeys::midiOutIndex, firstItemId);
        lastIndexEngineOption = propertyFile->getIntValue(SettingsFileKeys::engineOptionIndex, firstItemId);
        lastIndexAudioOut = propertyFile->getIntValue(SettingsFileKeys::audioOutIndex, firstItemId);
    }

    for (auto* label : { &outputEngineTypeLabel, &midiDevicesLabelIN, &midiDevicesLabelOUT, &audioDevicesLabelOUT,
                         &keyboardLabel, &chordModeLabel, &nttMinorScaleLabel })
        addAndMakeVisible(*label);
    for (auto* field : { &comboBoxOutputEngineType, &comboBoxDevicesIN, &comboBoxDevicesOUT, &comboBoxAudioDevicesOUT, &keyboardField })
    {
        field->addListener(this);
        addAndMakeVisible(*field);
    }

    addAndMakeVisible(addDeviceButton);
    addDeviceButton.onClick = [this]()
    {
        if (onAddDevice)
            onAddDevice(comboBoxDevicesIN.getText());
    };

    auto bindToggle = [this](RowToggle& toggle, const juce::String& key, bool fallback, std::function<void(bool)>* callback)
    {
        addAndMakeVisible(toggle);
        toggle.setToggleState(propertyFile != nullptr ? propertyFile->getBoolValue(key, fallback) : fallback, juce::dontSendNotification);
        toggle.setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        toggle.onClick = [this, &toggle, key, callback]()
        {
            const bool on = toggle.getToggleState();
            if (propertyFile != nullptr) { propertyFile->setValue(key, on); propertyFile->saveIfNeeded(); }
            if (*callback) (*callback)(on);
        };
    };
    // Same keys and defaults as before (Chord memory and Chord transposition default on).
    bindToggle(arrangerModeToggle,       SettingsFileKeys::arrangerModeEnabled, false, &onArrangerModeChanged);
    bindToggle(chordBassInversionToggle, SettingsFileKeys::chordBassInversion,  false, &onChordBassInversionChanged);
    bindToggle(chordMemoryToggle,        SettingsFileKeys::chordMemory,         true,  &onChordMemoryChanged);
    bindToggle(synchroStartToggle,       SettingsFileKeys::synchroStart,        false, &onSynchroStartChanged);
    bindToggle(countInToggle,            SettingsFileKeys::countIn,             false, &onCountInChanged);
    bindToggle(chordZoneMuteToggle,      SettingsFileKeys::chordZoneMute,       false, &onChordZoneMuteChanged);
    bindToggle(nttEnabledToggle,         SettingsFileKeys::nttEnabled,          true,  &onNttEnabledChanged);

    // Phase 5: 3-way chord-recognition mode selector (replaces the old Full-Keyboard on/off toggle).
    auto readStoredMode = [this]() -> ChordMode
    {
        if (propertyFile == nullptr) return ChordMode::Fingered;
        if (propertyFile->containsKey(SettingsFileKeys::chordMode))
            return (ChordMode) juce::jlimit(firstChordMode, lastChordMode, propertyFile->getIntValue(SettingsFileKeys::chordMode, firstChordMode));
        // migrate legacy bool: Full-Keyboard on -> FullKeyboard, else Fingered
        return propertyFile->getBoolValue(SettingsFileKeys::chordFullKeyboard, false) ? ChordMode::FullKeyboard
                                                                      : ChordMode::Fingered;
    };

    const int kChordModeRadio = 0x5C40DE;   // unique non-zero radio group id
    for (auto* button : { &chordModeFingered, &chordModeSingle, &chordModeFull })
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kChordModeRadio);
        button->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        addAndMakeVisible(*button);
    }

    const ChordMode storedMode = readStoredMode();
    (storedMode == ChordMode::FullKeyboard   ? chordModeFull
     : storedMode == ChordMode::SingleFinger ? chordModeSingle
     :                                         chordModeFingered).setToggleState(true, juce::dontSendNotification);

    auto pickMode = [this](ChordMode mode)
    {
        refreshChoiceLeds();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::chordMode, (int) mode); propertyFile->saveIfNeeded(); }
        if (onChordModeChanged) onChordModeChanged(mode);
    };
    chordModeFingered.onClick = [pickMode]{ pickMode(ChordMode::Fingered); };
    chordModeSingle.onClick   = [pickMode]{ pickMode(ChordMode::SingleFinger); };
    chordModeFull.onClick     = [pickMode]{ pickMode(ChordMode::FullKeyboard); };

    // Phase 7a: curated minor-scale selector for NTT passing-tone snap (Dorian default).
    const int kNttMinorRadio = 0x77017A;   // unique non-zero radio group id
    int minorIdx {};
    for (auto* button : { &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic })
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kNttMinorRadio);
        button->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        const int idx = minorIdx++;
        button->onClick = [this, idx]()
        {
            refreshChoiceLeds();
            if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::nttMinorScale, idx); propertyFile->saveIfNeeded(); }
            if (onNttMinorScaleChanged) onNttMinorScaleChanged(idx);
        };
        addAndMakeVisible(*button);
    }
    const int storedMinor = propertyFile != nullptr ? juce::jlimit(MinorScaleButtons::dorian, MinorScaleButtons::harmonic, propertyFile->getIntValue(SettingsFileKeys::nttMinorScale, MinorScaleButtons::dorian)) : MinorScaleButtons::dorian;
    (storedMinor == MinorScaleButtons::aeolian ? nttMinorAeolian : storedMinor == MinorScaleButtons::harmonic ? nttMinorHarmonic : nttMinorDorian)
        .setToggleState(true, juce::dontSendNotification);
    refreshChoiceLeds();
}

void MIDIWindow::populateCBIN()
{
    this->MIDIDevice.getAvailableDevicesMidiIN(this->devicesListIN);
    this->comboBoxDevicesIN.clear();
    int id = firstItemId;
    for (const auto& device : devicesListIN) {
        this->comboBoxDevicesIN.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesIN.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::populateCBOUT()
{
    this->MIDIDevice.getAvailableDevicesMidiOUT(this->devicesListOUT);
    this->comboBoxDevicesOUT.clear();
    int id = firstItemId;
    for (const auto& device : devicesListOUT) {
        this->comboBoxDevicesOUT.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesOUT.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::populateCBengine()
{
    this->comboBoxOutputEngineType.clear();
    int id = firstItemId;
    for (const auto& engine : { Text::externalMidi, Text::internalSfz })
    {
        this->comboBoxOutputEngineType.addItem(juce::String(engine), id);
        id++;
    }
    comboBoxOutputEngineType.setSelectedId(firstItemId, juce::dontSendNotification);
    switchBetweenEngineOptions(firstItemId);
}

void MIDIWindow::populateCBaudioOUT()
{
    MIDIDevice.getAvailableAudioDevicesOUT(this->devicesListAudioOUT);

    comboBoxAudioDevicesOUT.clear();

    int id = firstItemId;
    for (const auto& device : devicesListAudioOUT)
    {
        comboBoxAudioDevicesOUT.addItem(juce::String(device), id);
        id++;
    }

    comboBoxAudioDevicesOUT.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::restoreCBoxes()
{
    if(lastIndexIN <= comboBoxDevicesIN.getNumItems())
        comboBoxDevicesIN.setSelectedId(lastIndexIN, juce::sendNotificationSync);
    if(lastIndexOUT <= comboBoxDevicesOUT.getNumItems())
        comboBoxDevicesOUT.setSelectedId(lastIndexOUT, juce::sendNotificationSync);
    if(lastIndexAudioOut <= comboBoxAudioDevicesOUT.getNumItems())
        comboBoxAudioDevicesOUT.setSelectedId(lastIndexAudioOut, juce::sendNotificationSync);
    if(lastIndexEngineOption <= comboBoxOutputEngineType.getNumItems())
        comboBoxOutputEngineType.setSelectedId(lastIndexEngineOption, juce::sendNotificationSync);
}

void MIDIWindow::switchBetweenEngineOptions(int selectedId)
{
    // Today the window showed MIDI out for the external engine and Audio out for the internal synth. The page
    // shows both (board) and disables the one that does not apply.
    const bool internal = selectedId == OutputEngineIds::internalSfz;
    comboBoxDevicesOUT.setEnabled(! internal);
    comboBoxAudioDevicesOUT.setEnabled(internal);
    midiDevicesLabelOUT.setAlpha(internal ? StageMetrics::disabledLabelAlpha : StageMetrics::enabledAlpha);
    audioDevicesLabelOUT.setAlpha(internal ? StageMetrics::enabledAlpha : StageMetrics::disabledLabelAlpha);
}
