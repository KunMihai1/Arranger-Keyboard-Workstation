/*
  ==============================================================================

    MidiNotesTableModel.cpp
    Created: 15 Jul 2025 8:46:13pm
    Author:  Kisuke

  ==============================================================================
*/

#include "MidiNotesTableModel.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageFonts.h"
#include "Common/AppColours.h"

namespace
{
    constexpr float cellFontHeight   = 13.0f;   // board PageNotes .grid: 13 px Share Tech Mono
    constexpr float barPerVelocity   = 0.9f;    // board barWidth = velocity x 0.9
    constexpr int   velocityTextWidth = 36;     // the number, then the bar (board: 28 + gap 8)
    constexpr int   barHeight        = 7;
    constexpr float barAlpha         = 0.75f;
    constexpr int   indexTextInset   = 12;      // the "#" column keeps the grid's padding
    constexpr int   firstRowNumber   = 1;
    constexpr float halfOf           = 2.0f;
    constexpr int   cellTextSpare    = 4;
    constexpr int   cellTop          = 0;
    constexpr int   cellLeft         = 0;
    constexpr int   shortestNoteName = 2;      // a letter and an octave digit
    constexpr int   nameStart        = 0;
    constexpr int   notFound         = -1;
    constexpr double noTimeStamp     = -1;     // timestamp getters: no note to report
    constexpr double songStart       = 0.0;
    constexpr int   firstRow         = 0;
    constexpr int   oneRow           = 1;
    constexpr int   halves           = 2;      // binary search midpoint
    constexpr int   bytesEqual       = 0;      // memcmp result
    constexpr int   semitonesPerOctave = 12;
    constexpr int   lowestMidiNote   = 0;
    constexpr int   highestMidiNote  = 127;
    const juce::String octaveCharacters = "0123456789-";
    constexpr juce::juce_wchar negativeSign = '-';
    const juce::StringArray pitchClassNames { "C", "C#", "D", "D#", "E", "F",
                                              "F#", "G", "G#", "A", "A#", "B" };

    juce::String noteName (int noteNumber)
    {
        return juce::MidiMessage::getMidiNoteName(noteNumber, NoteNames::useSharps, NoteNames::includeOctave, NoteNames::middleCOctave);
    }
}

MidiNotesTableModel::MidiNotesTableModel(const juce::MidiMessageSequence& seq, int trackChannel, std::unordered_map<int, MidiChangeInfo>& map): channel{trackChannel}, changesMap{&map}
{
    for (int eventIndex {}; eventIndex < seq.getNumEvents(); ++eventIndex)
    {
        const auto* event = seq.getEventPointer(eventIndex);
        if (event != nullptr && event->message.isNoteOn())
        {
            int indexInNoteOn = noteOnEvents.size();
            noteOnEvents.push_back({ event, eventIndex, indexInNoteOn });
            //originalIndexToRowMap[i] = noteOnEvents.size()-1;
        }
    }
}

int MidiNotesTableModel::getNumRows()
{
    return static_cast<int>(noteOnEvents.size());
}

void MidiNotesTableModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    if (newSortColumnId == NoteTableColumns::note)
    {
        std::stable_sort(noteOnEvents.begin(), noteOnEvents.end(),
            [isForwards](const EventWithIndex& first, const EventWithIndex& second)
            {
                if (isForwards)
                    return first.event->message.getNoteNumber() < second.event->message.getNoteNumber();
                else return first.event->message.getNoteNumber() > second.event->message.getNoteNumber();
            });
    }
    else if (newSortColumnId == NoteTableColumns::time)
    {
        std::stable_sort(noteOnEvents.begin(), noteOnEvents.end(),
            [isForwards](const EventWithIndex& first, const EventWithIndex& second)
            {
                if (isForwards)
                    return first.event->message.getTimeStamp() < second.event->message.getTimeStamp();
                else return first.event->message.getTimeStamp() > second.event->message.getTimeStamp();
            });
    }
    else if (newSortColumnId == NoteTableColumns::velocity)
    {
        std::stable_sort(noteOnEvents.begin(), noteOnEvents.end(),
            [isForwards](const EventWithIndex& first, const EventWithIndex& second)
            {
                if (isForwards)
                    return first.event->message.getVelocity() < second.event->message.getVelocity();
                else return first.event->message.getVelocity() > second.event->message.getVelocity();
            });
    }

    if (refreshData)
        refreshData();
}

float MidiNotesTableModel::velocityBarWidth(int velocity, float pageScale)
{
    return static_cast<float>(velocity) * barPerVelocity * pageScale;
}

void MidiNotesTableModel::setScale(float newScale)
{
    scale = newScale;
}

void MidiNotesTableModel::paintRowBackground(juce::Graphics& g, int, int, int, bool rowIsSelected)
{
    // Board PageNotes: rows on the LCD, the selected one inverted.
    if (rowIsSelected)
        g.fillAll(StagePalette::lcdInk);
}

void MidiNotesTableModel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool isSelected)
{
    if (columnId == NoteTableColumns::index)
    {
        g.setColour(StageLcd::textColour(isSelected));
        g.setFont(StageFonts::lcd(cellFontHeight * scale));
        g.drawText(juce::String(rowNumber + firstRowNumber), juce::Rectangle<float>(indexTextInset * scale, float {}, static_cast<float>(width), static_cast<float>(height)),
                   juce::Justification::centredLeft, false);
        return;
    }

    if (columnId == NoteTableColumns::channel || columnId == NoteTableColumns::velocity || columnId == NoteTableColumns::time)
    {
        if (rowNumber < firstRow || rowNumber >= noteOnEvents.size()) return;
        auto* event = noteOnEvents[rowNumber].event;

        if (event == nullptr)
            return;

        const auto& msg = event->message;
        juce::String text;

        if (msg.isNoteOn())
        {
            switch (columnId)
            {
            case NoteTableColumns::note:     text = noteName(msg.getNoteNumber()); break;
            case NoteTableColumns::time:     text = juce::String(msg.getTimeStamp()); break;
            case NoteTableColumns::velocity: text = juce::String(msg.getVelocity()); break;
            case NoteTableColumns::channel: text = juce::String(this->channel); break;
            default: break;
            }
        }

        g.setColour(StageLcd::textColour(isSelected));
        g.setFont(StageFonts::lcd(cellFontHeight * scale));
        g.drawText(text, cellLeft, cellTop, width - cellTextSpare, height, juce::Justification::centredLeft);

        // Velocity: the number, then a bar as long as the velocity (board .vbar, 75 % of the text colour).
        if (columnId == NoteTableColumns::velocity && msg.isNoteOn())
        {
            const auto barLength = velocityBarWidth(msg.getVelocity(), scale);
            g.setColour(StageLcd::textColour(isSelected).withMultipliedAlpha(barAlpha));
            g.fillRect(juce::Rectangle<float>(velocityTextWidth * scale, (static_cast<float>(height) - barHeight * scale) / halfOf,
                                              juce::jmin(barLength, static_cast<float>(width) - velocityTextWidth * scale), barHeight * scale));
        }
    }
}


juce::Component* MidiNotesTableModel::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    if (columnId == NoteTableColumns::channel || columnId == NoteTableColumns::velocity || columnId == NoteTableColumns::time
        || columnId == NoteTableColumns::index)   // the "#" column is drawn, not edited
        return nullptr;


    if (rowNumber < firstRow || rowNumber >= noteOnEvents.size())
        return nullptr;

    const auto* event = noteOnEvents[rowNumber].event;
    if (event == nullptr)
        return nullptr;

    int originalIndex = noteOnEvents[rowNumber].originalIndex;
    int changeMapIndex = noteOnEvents[rowNumber].indexForChangesMap;

    //auto* label = dynamic_cast<juce::Label*> (existingComponentToUpdate);

    auto* label = dynamic_cast<SelectableLabel*> (existingComponentToUpdate);


    if (label == nullptr)
    {
        label = new SelectableLabel{};
        label->setWantsKeyboardFocus(true);
        label->setEditable(false, true, false);
    }
    // The note on the LCD row (double-click edits it, as before).
    label->setFont(StageFonts::lcd(cellFontHeight * scale));
    label->setBorderSize({});

    label->onEditorHide = [this, rowNumber, columnId, label, originalIndex,changeMapIndex]()
    {
        auto* noteOnEvent = noteOnEvents[rowNumber].event;
        if (noteOnEvent == nullptr)
            return;

        auto oldMsg = noteOnEvent->message;
        juce::MidiMessage newMsg;

        if (columnId == NoteTableColumns::note)
        {
            juce::String input = label->getText().trim();
            juce::String normalizedInput = input.toUpperCase();

            int newNoteNumber = getMidiNoteNumberFromName(normalizedInput);
            if (newNoteNumber == NoteNames::invalidNote)
            {
                label->setText(noteName(oldMsg.getNoteNumber()), juce::dontSendNotification);
                return;
            }

            label->setText(
                noteName(newNoteNumber),
                juce::dontSendNotification
            );

            newMsg = juce::MidiMessage::noteOn(oldMsg.getChannel(), newNoteNumber, (juce::uint8)oldMsg.getVelocity());
            newMsg.setTimeStamp(oldMsg.getTimeStamp());

            if (noteOnEvent->noteOffObject != nullptr)
            {
                auto* noteOffEvent = noteOnEvent->noteOffObject;
                juce::MidiMessage offMsg = juce::MidiMessage::noteOff(
                    noteOffEvent->message.getChannel(),
                    newNoteNumber
                );
                offMsg.setTimeStamp(noteOffEvent->message.getTimeStamp());
                noteOffEvent->message = offMsg;
            }
        }
        /*
        else if (columnId == 2)
        {
            double newTime = label->getText().getDoubleValue();
            if (newTime < 0)
            {
                label->setText(juce::String(oldMsg.getTimeStamp(), 6), juce::dontSendNotification);
                return;
            }
            newMsg = juce::MidiMessage::noteOn(oldMsg.getChannel(), oldMsg.getNoteNumber(), oldMsg.getVelocity());
            newMsg.setTimeStamp(newTime);
            if (e->noteOffObject != nullptr)
            {
                auto* noteOffEvent = e->noteOffObject;
                double oldNoteOnTime = oldMsg.getTimeStamp();
                double oldNoteOffTime = noteOffEvent->message.getTimeStamp();
                double duration = oldNoteOffTime - oldNoteOnTime;

                double newNoteOffTime = newTime + duration;
                noteOffEvent->message.setTimeStamp(newNoteOffTime);
            }
            
        }
        else if (columnId == 3)
        {
            int newVelocity = label->getText().getIntValue();
            if (newVelocity < 0 || newVelocity>127)
            {
                label->setText(juce::String(oldMsg.getVelocity()), juce::dontSendNotification);
                return;
            }
            newMsg = juce::MidiMessage::noteOn(oldMsg.getChannel(), oldMsg.getNoteNumber(), (juce::uint8)newVelocity);

            newMsg.setTimeStamp(oldMsg.getTimeStamp());
        }
        */

        if (changesMap->find(changeMapIndex) != changesMap->end())
        {
            auto& current = (*changesMap)[changeMapIndex];
            handleMapExistingCase(newMsg, current);
        }
        else
        {
            MidiChangeInfo current;
            handleMapNonExistingCase(oldMsg, newMsg, current);
            (*changesMap)[changeMapIndex] = current;
        }

        auto& current = (*changesMap)[changeMapIndex];
        if (validForErase(current))
            changesMap->erase(changeMapIndex);

        const_cast<juce::MidiMessageSequence::MidiEventHolder*>(noteOnEvent)->message = newMsg;

        if (!areMidiMessagesEqual(newMsg, oldMsg) && onUpdate)
            onUpdate(rowNumber);
    };

    label->onClick = [this, rowNumber]()
    {
        if (onRequestSelectRow)
            onRequestSelectRow(rowNumber);
    };

    const auto& message = event->message;
    if (message.isNoteOn())
    {
        if (columnId == NoteTableColumns::note)
        {
            auto shownName = noteName(message.getNoteNumber());
            label->setText(shownName, juce::dontSendNotification);
        }
        
        else if (columnId == NoteTableColumns::time)
        {
            auto noteTime = juce::String(message.getTimeStamp());
            label->setText(noteTime, juce::dontSendNotification);
        }
        else if (columnId == NoteTableColumns::velocity)
        {
            auto noteVelocity = juce::String(message.getVelocity());
            label->setText(noteVelocity, juce::dontSendNotification);
        }
        
    }
    label->setColour(juce::Label::textColourId, StageLcd::textColour(isRowSelected));   // the LCD's ink (inverted when selected)

    return label;
}


int MidiNotesTableModel::getMidiNoteNumberFromName(const juce::String& noteName, int octaveNoteMiddleC)
{
    juce::String name = noteName.trim().toUpperCase();
    if (name.length() < shortestNoteName)
        return NoteNames::invalidNote;

    int digit = findFirstDigitName(noteName);
    if (digit == notFound)
        return NoteNames::invalidNote;

    juce::String notePart = name.substring(nameStart, digit);
    juce::String octavePart = name.substring(digit);

    if (!octavePart.containsOnly(octaveCharacters))
        return NoteNames::invalidNote;

    int noteIndex = pitchClassNames.indexOf(notePart);
    int octave = octavePart.getIntValue();


    if (noteIndex == notFound)
        return NoteNames::invalidNote;

    int midiNumber = (octave - octaveNoteMiddleC + NoteNames::octavesBelowMiddleC) * semitonesPerOctave + noteIndex;

    if (midiNumber < lowestMidiNote || midiNumber > highestMidiNote)
        return NoteNames::invalidNote;

    return midiNumber;
}

int MidiNotesTableModel::findFirstDigitName(const juce::String& name)
{
    for (int characterIndex {}; characterIndex < name.length(); characterIndex++)
    {
        if (juce::CharacterFunctions::isDigit(name[characterIndex]) || name[characterIndex] == negativeSign)
            return characterIndex;
    }
    return notFound;
}

void MidiNotesTableModel::handleMapNonExistingCase(juce::MidiMessage& oldMessage, juce::MidiMessage& newMessage, MidiChangeInfo& information)
{
    information.oldNumber = oldMessage.getNoteNumber();
    information.oldTimeStamp = oldMessage.getTimeStamp();
    information.oldVelocity = oldMessage.getVelocity();
    information.oldBPMchange = getCurrentBpm();

    information.newNumber = newMessage.getNoteNumber();
    information.newTimeStamp = newMessage.getTimeStamp();
    information.newVelocity = newMessage.getVelocity();
    information.newBPMchange = getCurrentBpm();
}

void MidiNotesTableModel::handleMapExistingCase(juce::MidiMessage& newMessage, MidiChangeInfo& information)
{
    information.newNumber = newMessage.getNoteNumber();
    information.newTimeStamp = newMessage.getTimeStamp();
    information.newVelocity = newMessage.getVelocity();
}

bool MidiNotesTableModel::areMidiMessagesEqual(const juce::MidiMessage& first, const juce::MidiMessage& second)
{
    if (first.getTimeStamp() != second.getTimeStamp())
        return false;

    if (first.getRawDataSize() != second.getRawDataSize())
        return false;

    return std::memcmp(first.getRawData(), second.getRawData(), first.getRawDataSize()) == bytesEqual; // comparing bytes
}

void MidiNotesTableModel::refreshVectorFromSequence(const juce::MidiMessageSequence& seq)
{
    noteOnEvents.clear();
    //originalIndexToRowMap.clear();
    for (int eventIndex {}; eventIndex < seq.getNumEvents(); eventIndex++)
    {
        const auto* event = seq.getEventPointer(eventIndex);
        if (event != nullptr && event->message.isNoteOn())
        {
            noteOnEvents.push_back({ event, eventIndex, static_cast<int>(noteOnEvents.size())});
            //originalIndexToRowMap[i] = static_cast<int>(noteOnEvents.size()) - 1;
        }
    }
}

int MidiNotesTableModel::getOriginalIndexFromRow(int row)
{
    if (row >= firstRow && row < noteOnEvents.size())
        return noteOnEvents[row].originalIndex;
    return notFound;
}

int MidiNotesTableModel::getChangesMapIndexFromRow(int row)
{
    if (row >= firstRow && row < noteOnEvents.size())
        return noteOnEvents[row].indexForChangesMap;
    return notFound;
}

double MidiNotesTableModel::getFirstNoteOnTimeStamps()
{
    if (noteOnEvents.empty())
        return noTimeStamp;

    return noteOnEvents.front().event->message.getTimeStamp();
}

double MidiNotesTableModel::getPreviousNoteOnTimeStamp(int currentIndex)
{
    if (noteOnEvents.empty() || currentIndex == NoteTableColumns::noRow)
        return noTimeStamp;

    if (currentIndex == firstRow)
        return songStart;

    else return noteOnEvents[currentIndex - oneRow].event->message.getTimeStamp();
}

double MidiNotesTableModel::getCurrentNoteOnTimeStamp(int currentIndex)
{
    if (noteOnEvents.empty() || currentIndex == NoteTableColumns::noRow)
        return noTimeStamp;

    return noteOnEvents[currentIndex].event->message.getTimeStamp();
}

double MidiNotesTableModel::getNextNoteOnTimeStamp(int currentIndex)
{
    if (noteOnEvents.empty() || currentIndex == NoteTableColumns::noRow)
        return noTimeStamp;

    if (currentIndex >= noteOnEvents.size())
        return noTimeStamp;

    if (currentIndex == noteOnEvents.size() - oneRow)
        return noteOnEvents[currentIndex].event->message.getTimeStamp();

    return noteOnEvents[currentIndex + oneRow].event->message.getTimeStamp();
}

std::vector<MidiNotesTableModel::EventWithIndex> MidiNotesTableModel::getEvents()
{
    return noteOnEvents;
}

int MidiNotesTableModel::getRowFromTime(double currentTime)
{
    if (noteOnEvents.empty())
        return NoteTableColumns::noRow;

    int left {};
    int right = static_cast<int>(noteOnEvents.size()) - oneRow;
    int result = NoteTableColumns::noRow;

    while (left <= right)
    {
        int middle = (left + right) / halves;
        double middleTime = noteOnEvents[middle].event->message.getTimeStamp();

        if (middleTime <= currentTime)
        {
            result = middle;    
            left = middle + oneRow;
        }
        else
        {
            right = middle - oneRow;
        }
    }

    return result; 
    }

void MidiNotesTableModel::updateCurrentRowBasedOnTime(double currentTime)
{
    int toHighlitRow = getRowFromTime(currentTime);

    if (highlightedRow != toHighlitRow)
    {
        highlightedRow = toHighlitRow;
        if (onMidPlayRepaint)
            onMidPlayRepaint(highlightedRow);
    }
}