#include "displayGUI.h"
#include "PersistedKeys.h"

namespace
{
    namespace Text
    {
        const juce::String instrumentButton  = "Instrument";
        const juce::String percussionNoInstrument = "Since this is a track for percussion, the instrument change won't take effect!\n"
                                                    "If you want to change anything, you must change the original notes!";
        const juce::String renameItem        = "Rename";
        const juce::String deleteItem        = "Delete";
        const juce::String copyItem          = "Copy";
        const juce::String pasteItem         = "Paste";
        const juce::String notesInfoItem     = "Notes information";
        const juce::String renameTitle       = "Rename track";
        const juce::String renamePrompt      = "Enter a name for your track";
        const juce::String nameField         = "nameEditor";
        const juce::String nameFieldLabel    = "Track Name:";
        const juce::String okButton          = "OK";
        const juce::String cancelButton      = "Cancel";
        const juce::String confirmRemove     = "Confirm Remove";
        const juce::String sureRemoveTrack   = "Are you sure you want to remove the selected track?";
        const juce::String removeButton      = "Remove";
        const juce::String trackInfoTitle    = "Track info";
        const juce::String trackCopied       = "The track has been succesfully copied!";
    }

    constexpr int    lowestVolume        = 0;
    constexpr int    highestVolume       = 127;     // MIDI CC7 range
    constexpr int    volumeStep          = 1;
    constexpr int    firstTrackChannel   = 2;       // style tracks use channels 2..16
    constexpr int    lastTrackChannel    = 16;
    constexpr int    drumChannel         = 10;
    constexpr int    dialogOk            = 1;
    constexpr int    dialogCancel        = 0;
    // Layout (pixels)
    constexpr int    sliderHeightDivisor = 2;
    constexpr int    sliderExtraHeight   = 20;
    constexpr int    sliderLeft          = 0;
    constexpr int    sliderRaise         = 10;
    constexpr int    sliderWidth         = 15;
    constexpr int    volumeLabelX        = 20;
    constexpr int    volumeLabelY        = 0;
    constexpr int    volumeLabelWidth    = 30;
    constexpr int    volumeLabelHeight   = 40;
    constexpr int    nameLabelInset      = 40;
    constexpr int    centreDivisor       = 2;
    constexpr int    nameLabelGap        = 20;
    constexpr int    nameLabelHeight     = 20;
    constexpr int    buttonGap           = 10;
    constexpr int    buttonRightSpace    = 5;
    constexpr int    buttonDesiredWidth  = 40;
    constexpr int    buttonAboveName     = 25;
    constexpr int    buttonWidthTrim     = 2;
    constexpr int    buttonHeight        = 20;
    constexpr int    borderThickness     = 1;
    constexpr int    tooltipWidth        = 150;
    constexpr int    tooltipHeight       = 24;
    constexpr int    tooltipOffset       = 4;
    constexpr int    noDesktopFlags      = 0;
    constexpr int    infoBorder          = 2;
    constexpr int    infoWidth           = 300;
    constexpr int    infoHeight          = 100;
}

Track::Track()
{
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setRange(lowestVolume, highestVolume, volumeStep); //changed from 100 to 127 recently
    volumeSlider.onValueChange = [this]()
    {
        volumeLabel.setText(juce::String(volumeSlider.getValue()), juce::dontSendNotification);
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


    volumeLabel.setText(juce::String(volumeSlider.getValue()), juce::dontSendNotification);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colours::crimson);
    addAndMakeVisible(volumeLabel);

    nameLabel.setText(StyleTrackJsonKeys::emptySlot, juce::dontSendNotification);
    //nameLabel.setTooltip(nameLabel.getText());
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    nameLabel.setInterceptsMouseClicks(true, false);
    nameLabel.addMouseListener(this, false);
    nameLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    addAndMakeVisible(nameLabel);

    instrumentChooserButton.setButtonText(Text::instrumentButton);

    instrumentChooserButton.onClick = [this] {
        if (channel >= firstTrackChannel && channel <= lastTrackChannel)
        {
            if (channel != drumChannel)
                openInstrumentChooser();
            else
            {
                showInstantInfo(Text::percussionNoInstrument);
            }
        }
    };
    instrumentChooserButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(instrumentChooserButton);

    instrumentlist = instrumentListBuild();


}

Track::~Track()
{
    volumeSlider.onDragEnd = nullptr;
    volumeSlider.onValueChange = nullptr; //in case of alt f4 it will crash otherwise since these listeners will propagate the onChange
}

void Track::resized()
{
    auto heightOfSlider = getHeight() / sliderHeightDivisor + sliderExtraHeight;
    auto startY = (getHeight() - heightOfSlider) / centreDivisor;
    volumeSlider.setBounds(sliderLeft, startY - sliderRaise, sliderWidth, heightOfSlider);
    volumeLabel.setBounds(volumeLabelX, volumeLabelY, volumeLabelWidth, volumeLabelHeight);
    nameLabel.setBounds((getWidth() - nameLabelInset) / centreDivisor, volumeLabel.getHeight() + volumeLabel.getY() + nameLabelGap, getWidth(), nameLabelHeight);


    int buttonX = volumeSlider.getX() + volumeSlider.getWidth() + buttonGap;
    int maxWidth = getWidth() - buttonX - buttonRightSpace;
    int desiredWidth = buttonDesiredWidth;
    int actualWidth = juce::jmin(desiredWidth, maxWidth);
    instrumentChooserButton.setBounds(buttonX, nameLabel.getY() - buttonAboveName, maxWidth - buttonWidthTrim, buttonHeight);
}

void Track::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds(), borderThickness);
}

void Track::mouseEnter(const juce::MouseEvent& ev)
{
    if (ev.eventComponent == &nameLabel && toolTipWindow == nullptr)
    {
        toolTipWindow = std::make_unique<CustomToolTip>(nameLabel.getText());
        toolTipWindow->setSize(tooltipWidth, tooltipHeight);

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
    std::unique_ptr<InstrumentChooserComponent> chooser = std::make_unique<InstrumentChooserComponent>(instrumentlist);
    chooser->instrumentSelectedFunction = [this](int instrumentIndex, const juce::String& name)
    {
        setInstrumentNumber(instrumentIndex, true);
        nameLabel.setText(name, juce::dontSendNotification);
        if (onChange)
            onChange();

    };

    auto* display = findParentComponentOfClass<Display>();
    auto bounds = display->getScreenBounds();

    chooser->setSize(bounds.getWidth(), bounds.getHeight());

    juce::CallOutBox::launchAsynchronously(std::move(chooser), bounds, nullptr);
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
    volumeLabel.setText(value, juce::dontSendNotification);
    if (value.getIntValue() != TrackEntryDefaults::keepCurrent && shouldNotify)
        notify(channel, value.getIntValue(), TrackEntryDefaults::keepCurrent);
}

void Track::setNameLabel(const juce::String& name)
{
    nameLabel.setText(name, juce::dontSendNotification);
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

void Track::renameOneTrack()
{
    auto* window = new juce::AlertWindow{ Text::renameTitle, Text::renamePrompt, juce::AlertWindow::NoIcon};

    window->addTextEditor(Text::nameField, nameLabel.getText(), Text::nameFieldLabel);

    window->addButton(Text::okButton, dialogOk, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(Text::cancelButton, dialogCancel, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true, juce::ModalCallbackFunction::create([this, window](int result)
        {
            std::unique_ptr<juce::AlertWindow> cleanup{ window };
            if (result != dialogOk)
                return;



            juce::String newNameToPut=window->getTextEditor(Text::nameField)->getText().trim();
            setNameLabel(newNameToPut);
            if (onChange)
                onChange();
        }
    ));

    juce::MessageManager::callAsync([window]()
        {
            if (auto* editor = window->getTextEditor(Text::nameField))
                editor->grabKeyboardFocus();
        });
}

void Track::deleteOneTrack()
{
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::WarningIcon,
        Text::confirmRemove,
        Text::sureRemoveTrack,
        Text::removeButton,
        Text::cancelButton,
        this,
        juce::ModalCallbackFunction::create(
            [this](int result)
            {
                if (result == dialogCancel)
                    return;

                setNameLabel(StyleTrackJsonKeys::emptySlot);
                setTypeOfTrack(StyleTrackJsonKeys::emptySlot);
                setInstrumentNumber(noInstrument);
                setUUID(juce::Uuid{ StyleTrackJsonKeys::noTrackId });

                if (onChange)
                    onChange();
            }
            ));
}

void Track::copyOneTrack()
{
    if (onCopy)
        onCopy(this);

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::trackInfoTitle, Text::trackCopied);
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
