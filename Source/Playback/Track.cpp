#include "displayGUI.h"
#include "PersistedKeys.h"
#include "UI/Stage/StageFonts.h"
#include "UI/Stage/StageMetrics.h"
#include "Common/AppColours.h"

namespace
{
    namespace Text
    {
        const juce::String percussionNoInstrument = "Since this is a track for percussion, the instrument change won't take effect!\n"
                                                    "If you want to change anything, you must change the original notes!";
        const juce::String renameItem        = "Rename";
        const juce::String deleteItem        = "Delete";
        const juce::String copyItem          = "Copy";
        const juce::String pasteItem         = "Paste";
        const juce::String notesInfoItem     = "Notes information";
        const juce::String renameTitle       = "Rename track";
        const juce::String nameFieldLabel    = "Track name";
        const juce::String okButton          = "OK";
        const juce::String removeTitle       = "Remove track";
        const juce::String sureRemoveTrack   = "Empty this slot? The track stays in the library.";
        const juce::String removeButton      = "Remove";
        const juce::String trackCopied       = "Track copied";
        const juce::String drums             = "Drums";
        const juce::String asRecorded        = "As recorded";
        const char* const  emptyUtf8         = "\xe2\x80\x94";   // U+2014 em dash: an empty slot
        const juce::String lineBreak         = "\n";
        const juce::String space             = " ";
    }

    constexpr int    firstTrackChannel   = 2;       // style tracks use channels 2..16
    constexpr int    lastTrackChannel    = 16;
    constexpr int    drumChannel         = 10;
    constexpr float  halves              = 2.0f;
    constexpr float  noSquash            = 1.0f;   // Label minimum horizontal scale: cut, never squash
    constexpr int    firstProgram        = 0;
    constexpr int    lastProgram         = 127;    // General MIDI programs 0..127
    constexpr int    tooltipOffset       = 4;
    constexpr int    noDesktopFlags      = 0;
    constexpr int    infoBorder          = 2;
    constexpr int    infoWidth           = 300;
    constexpr int    infoHeight          = 100;
}

Track::Track()
{
    // StageFader is the board's 0-127 channel fader (CC7).
    volumeSlider.setComponentID(TrackStripIds::fader);
    volumeSlider.onValueChange = [this]()
    {
        volumeLabel.setText(juce::String(juce::roundToInt(volumeSlider.getValue())), juce::dontSendNotification);
    };

    volumeSlider.onDragEnd = [this]()
    {
        if (getTypeOfTrack() == TrackTypeConversion::toString(TrackType::Percussion) && syncVolumePercussionTracks)
            syncVolumePercussionTracks(volumeSlider.getValue());

        if (onChange)
            onChange();

        if (static_cast<int>(volumeSlider.getValue()) != TrackEntryDefaults::keepCurrent && isPlaying())
            notify(channel, static_cast<int>(volumeSlider.getValue()), TrackEntryDefaults::keepCurrent);
    };

    addAndMakeVisible(volumeSlider);


    volumeLabel.setText(juce::String(juce::roundToInt(volumeSlider.getValue())), juce::dontSendNotification);
    volumeLabel.setComponentID(TrackStripIds::volume);
    volumeLabel.setColour(juce::Label::textColourId, StagePalette::silkscreenDimText);
    volumeLabel.setJustificationType(juce::Justification::centred);
    volumeLabel.setBorderSize({});
    volumeLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(volumeLabel);

    // The name sits on an LCD strip (.tname) that paint() draws behind it.
    nameLabel.setText(StyleTrackJsonKeys::emptySlot, juce::dontSendNotification);
    nameLabel.setComponentID(TrackStripIds::name);
    nameLabel.setColour(juce::Label::textColourId, StagePalette::lcdInk);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setBorderSize({});
    nameLabel.setMinimumHorizontalScale(noSquash);

    nameLabel.setInterceptsMouseClicks(true, false);
    nameLabel.addMouseListener(this, false);
    nameLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addAndMakeVisible(nameLabel);

    instrumentChooserButton.setComponentID(TrackStripIds::instrument);
    instrumentChooserButton.onClick = [this] {
        if (channel >= firstTrackChannel && channel <= lastTrackChannel)
        {
            if (channel != drumChannel)
                openInstrumentChooser();
            else if (onInfo)
                onInfo(Text::percussionNoInstrument.replace(Text::lineBreak, Text::space));   // one line for the screen
            else
                showInstantInfo(Text::percussionNoInstrument);
        }
    };
    addAndMakeVisible(instrumentChooserButton);

    notesButton.setComponentID(TrackStripIds::notes);
    notesButton.setBoardHeight(StageMetrics::stripNotesHeight);
    notesButton.setPaddingSide(StageMetrics::stripNotesPadding);
    notesButton.onClick = [this] { showNotesInformation(); };
    addAndMakeVisible(notesButton);

    instrumentlist = instrumentListBuild();
    setScale(StageMetrics::referenceScale);


}

Track::~Track()
{
    volumeSlider.onDragEnd = nullptr;
    volumeSlider.onValueChange = nullptr; //in case of alt f4 it will crash otherwise since these listeners will propagate the onChange
}

void Track::setSlotNumber(int oneBasedSlot)
{
    slotNumber = oneBasedSlot;
    repaint();
}

void Track::setScale(float newScale)
{
    scale = newScale;
    volumeSlider.setScale(scale);
    instrumentChooserButton.setScale(scale);
    notesButton.setScale(scale);
    nameLabel.setFont(StageFonts::lcd(StageMetrics::stripNameTextSize * scale));
    volumeLabel.setFont(StageFonts::lcd(StageMetrics::silkLabelSize * scale));
    refreshInstrumentCaption();
    resized();
    repaint();
}

juce::String Track::instrumentCaption() const
{
    if (nameLabel.getText() == StyleTrackJsonKeys::emptySlot)
        return juce::String::fromUTF8(Text::emptyUtf8);
    if (typeOfTrack == TrackTypeConversion::toString(TrackType::Percussion))
        return Text::drums;
    if (usedInstrumentNumber >= firstProgram && usedInstrumentNumber <= lastProgram)
        return instrumentlist[usedInstrumentNumber];
    return Text::asRecorded;
}

void Track::chooseInstrument(int instrumentIndex)
{
    setInstrumentNumber(instrumentIndex, true);
    if (onInstrumentChosen)
        onInstrumentChosen(instrumentIndex);
    if (onChange)
        onChange();
}

void Track::refreshInstrumentCaption()
{
    instrumentChooserButton.setButtonText(instrumentCaption());
}

void Track::resized()
{
    // Board .strip, top to bottom with 6 px gaps: number, name, instrument, fader, value, Notes;
    // everything centred in the strip right of its 1 px border.
    using namespace StageMetrics;
    auto scaled = [this] (int boardLength) { return static_cast<float>(boardLength) * scale; };
    const auto content = getLocalBounds().toFloat().withTrimmedLeft(scaled(stripBorder));
    const auto centreX = content.getCentreX();
    auto top = scaled(stripPadding + stripNumberHeight + stripGap);
    auto place = [&] (juce::Component& part, float width, float height, int boardHeight)
    {
        part.setBounds(juce::Rectangle<float>(centreX - width / halves, top, width, height).toNearestInt());
        top += scaled(boardHeight + stripGap);
    };
    place(nameLabel, scaled(stripNameWidth), scaled(stripNameHeight), stripNameHeight);
    place(instrumentChooserButton, scaled(instrumentWidth), scaled(instrumentHeight + instrumentDropEdge), instrumentHeight);
    place(volumeSlider, scaled(faderWidth), scaled(faderHeight), faderHeight);
    place(volumeLabel, content.getWidth(), scaled(stripValueHeight), stripValueHeight);
    place(notesButton, static_cast<float>(notesButton.getPreferredWidth()), scaled(stripNotesHeight), stripNotesHeight);
}

void Track::paint(juce::Graphics& g)
{
    using namespace StageMetrics;
    // .strip border-left, the slot number (silk-dim), and the LCD strip behind the name (.tname).
    const auto border = static_cast<float>(stripBorder) * scale;
    g.setColour(StagePalette::stripBorder);
    g.fillRect(getLocalBounds().toFloat().withWidth(border));

    const auto number = getLocalBounds().toFloat().withTrimmedLeft(border)
                            .withTrimmedTop(static_cast<float>(stripPadding) * scale)
                            .withHeight(static_cast<float>(stripNumberHeight) * scale);
    g.setColour(StagePalette::silkscreenDimText);
    g.setFont(StageFonts::silkscreen(silkLabelSize * scale, false, silkDimSpacing));
    g.drawText(juce::String(slotNumber), number, juce::Justification::centred, false);

    StageLcd::paintPanel(g, nameLabel.getBounds().toFloat(), scale);
}

void Track::mouseEnter(const juce::MouseEvent& ev)
{
    if (ev.eventComponent == &nameLabel && toolTipWindow == nullptr)
    {
        toolTipWindow = std::make_unique<CustomToolTip>(nameLabel.getText(), scale);   // sized to the whole name

        auto labelBounds = nameLabel.getScreenBounds();
        int tooltipX = labelBounds.getX() + tooltipOffset;
        int tooltipY = labelBounds.getBottom() + tooltipOffset;

        toolTipWindow->setTopLeftPosition(tooltipX, tooltipY);
        toolTipWindow->addToDesktop(noDesktopFlags);
        toolTipWindow->toFront(true);
        toolTipWindow->setVisible(true);
    }
}

void Track::mouseExit(const juce::MouseEvent& ev)
{
    if (toolTipWindow != nullptr)
    {
        toolTipWindow->setVisible(false);
        toolTipWindow = nullptr;
    }
}

juce::String Track::getName() const
{
    return this->nameLabel.getText();
}

juce::DynamicObject* Track::getJson() const
{

    auto* obj = new juce::DynamicObject{};

    obj->setProperty(StyleTrackJsonKeys::name, nameLabel.getText());
    obj->setProperty(StyleTrackJsonKeys::type, typeOfTrack);
    obj->setProperty(StyleTrackJsonKeys::volume, volumeSlider.getValue());
    obj->setProperty(StyleTrackJsonKeys::instrumentNumber, usedInstrumentNumber);
    obj->setProperty(StyleTrackJsonKeys::uuid, uniqueIdentifierTrack.toString());

    return obj;
}

void Track::showInstantInfo(const juce::String& text)
{
    auto label = std::make_unique<juce::Label>();
    label->setText(text, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::lightyellow);
    label->setColour(juce::Label::textColourId, juce::Colours::black);
    label->setBorderSize(juce::BorderSize<int>(infoBorder));
    label->setSize(infoWidth, infoHeight);

    auto* display = findParentComponentOfClass<Display>();
    if (display != nullptr)
    {
        juce::CallOutBox::launchAsynchronously(std::move(label), display->getScreenBounds(), nullptr);
    }
}

juce::var Track::loadJson(const juce::File& file)
{
    if (file.exists())
        return juce::var{};

    juce::String jsonString = file.loadFileAsString();
    juce::var jsonVar = juce::JSON::parse(jsonString);

    if (jsonVar.isUndefined())
        return juce::var{};

    return jsonVar;
}

void Track::openInstrumentChooser()
{
    auto* display = findParentComponentOfClass<Display>();
    if (display == nullptr)
        return;

    const juce::Component::SafePointer<Track> safeThis (this);
    display->showInstrumentPicker(slotNumber, usedInstrumentNumber, [safeThis](int program)
    {
        if (safeThis != nullptr)
            safeThis->chooseInstrument(program);   // keeps the track name (owner, 2026-11-23)
    });
}

juce::StringArray Track::instrumentListBuild()
{
    // The 128 General MIDI program names, in program order.
    static const juce::StringArray generalMidiInstrumentNames = {
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar harmonics",
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "SynthStrings 1",
    "SynthStrings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "Synth Brass 1",
    "Synth Brass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot"
    };
    return generalMidiInstrumentNames;
}

void Track::setInstrumentNumber(int newInstrumentNumber, bool shouldNotify)
{
    this->usedInstrumentNumber = newInstrumentNumber;
    refreshInstrumentCaption();
    if (usedInstrumentNumber != noInstrument && shouldNotify)
        notify(channel, TrackEntryDefaults::keepCurrent, usedInstrumentNumber);
}

int Track::getInstrumentNumber() const
{
    return usedInstrumentNumber;
}

void Track::setVolumeSlider(double value)
{
    this->volumeSlider.setValue(value);
}

void Track::setVolumeLabel(const juce::String& value, bool shouldNotify)
{
    volumeLabel.setText(juce::String(juce::roundToInt(value.getDoubleValue())), juce::dontSendNotification);   // board: 100, not 100.0
    if (value.getIntValue() != TrackEntryDefaults::keepCurrent && shouldNotify)
        notify(channel, value.getIntValue(), TrackEntryDefaults::keepCurrent);
}

void Track::setNameLabel(const juce::String& name)
{
    nameLabel.setText(name, juce::dontSendNotification);
    refreshInstrumentCaption();
}

void Track::mouseDown(const juce::MouseEvent& event)
{
    if (event.eventComponent == &nameLabel)
    {
        if (event.mods.isLeftButtonDown())
        {
            if (onRequestTrackSelection)
            {
                onRequestTrackSelection([this](const juce::String& selectedTrack, const juce::Uuid& uuid, const juce::String& type)
                    {
                        setNameLabel(selectedTrack);
                        setUUID(uuid);
                        setTypeOfTrack(type);
                        if (type.trim() == TrackTypeNames::percussion)
                            this->channel = drumChannel;
                    }
                );
            }
        }
        else if(event.mods.isRightButtonDown()){
            juce::PopupMenu menu;
            menu.addItem(Text::renameItem, [this]()
                {
                    renameOneTrack();
                });
            menu.addItem(Text::deleteItem, [this]()
                {
                    deleteOneTrack();
                });
            menu.addItem(Text::copyItem, [this]()
                {
                    copyOneTrack();
                });

            menu.addItem(Text::pasteItem, [this]()
                {
                    pasteOneTrack();
                });

            menu.addItem(Text::notesInfoItem, [this]()
                {
                   showNotesInformation();
                });

            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&nameLabel));
        }
    }
}

void Track::setUUID(const juce::Uuid& newUUID)
{
    this->uniqueIdentifierTrack = newUUID;
}

void Track::setTypeOfTrack(const juce::String& newType)
{
    this->typeOfTrack = newType;
    refreshInstrumentCaption();
}

juce::String Track::getTypeOfTrack() const
{
    return this->typeOfTrack;
}

juce::Uuid Track::getUsedID() const
{
    return uniqueIdentifierTrack;
}

void Track::setChannel(int newChannel)
{
    this->channel = newChannel;
}

int Track::getChannel() const
{
    return this->channel;
}

double Track::getVolume() const
{
    return this->volumeSlider.getValue();
}

void Track::copyFrom(const Track& other)
{
    setNameLabel(other.getName());
    setInstrumentNumber(other.getInstrumentNumber());
    setTypeOfTrack(other.getTypeOfTrack());
    setVolumeSlider(other.getVolume());
    setVolumeLabel(juce::String(other.getVolume()));
    setUUID(other.getUsedID());
}

void Track::pasteFrom(const Track& source)
{
    copyFrom(source);
}

StageDialog* Track::getOpenDialog() const
{
    return dialog != nullptr && dialog->isVisible() ? dialog.get() : nullptr;
}

void Track::renameOneTrack()
{
    dialog = std::make_unique<StageDialog>(StageDialogOptions { Text::renameTitle, Text::nameFieldLabel, Text::okButton, true, nameLabel.getText() },
        [this](const juce::String& newNameToPut)
        {
            setNameLabel(newNameToPut);
            if (onChange)
                onChange();
        });
    dialog->showOver(*this, scale);
}

void Track::deleteOneTrack()
{
    dialog = std::make_unique<StageDialog>(StageDialogOptions { Text::removeTitle, Text::sureRemoveTrack, Text::removeButton },
        [this](const juce::String&)
        {
            setNameLabel(StyleTrackJsonKeys::emptySlot);
            setTypeOfTrack(StyleTrackJsonKeys::emptySlot);
            setInstrumentNumber(noInstrument);
            setUUID(juce::Uuid{ StyleTrackJsonKeys::noTrackId });

            if (onChange)
                onChange();
        });
    dialog->showOver(*this, scale);
}

void Track::copyOneTrack()
{
    if (onCopy)
        onCopy(this);

    if (onInfo)
        onInfo(Text::trackCopied);
}

void Track::pasteOneTrack()
{
    if (onPaste)
        onPaste(this);

    if (onChange)
        onChange();
}

void Track::showNotesInformation()
{
    if (onShowInformation)
        onShowInformation(uniqueIdentifierTrack, channel);
}
