#include "ArrangerStyleListComponent.h"
#include "IOHelper.h"
#include "AppColours.h"

namespace
{
    const juce::String deleteTitle        = "Delete configuration";
    const juce::String deleteQuestionStart = "Delete \"";
    const juce::String deleteQuestionEnd   = "\"?\nThis cannot be undone.";
    const juce::String confirmDelete      = "Delete";
    const juce::String cancelDelete       = "Cancel";
    constexpr int      deleteConfirmed    = 1;   // showOkCancelBox: 1 = Delete, 0 = Cancel

    const juce::String styleFilePattern   = "*.style";
    constexpr bool     searchSubfolders   = false;

    const char* const  checkMarkUtf8      = "\xE2\x9C\x93";   // U+2713 check mark
    const juce::String activeTag          = "active";

    // Row layout (pixels)
    constexpr int   rowPadding       = 6;
    constexpr int   checkMarkWidth   = 14;
    constexpr int   nameAfterCheckX  = 22;
    constexpr int   activeTagWidth   = 48;
    constexpr int   noWidth          = 0;
    constexpr int   rowTop           = 0;
    constexpr float maxTagFontHeight = 12.0f;
    constexpr float tagFontInset     = 4.0f;

    // Panel layout (pixels)
    constexpr int panelMargin   = 6;
    constexpr int titleHeight   = 20;
    constexpr int sectionGap    = 4;
    constexpr int buttonsHeight = 28;
    constexpr int buttonCount   = 5;
    constexpr int buttonInset   = 2;
}

ArrangerStyleListComponent::ArrangerStyleListComponent()
{
    addAndMakeVisible (title);
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (list);
    addAndMakeVisible (newBtn);
    addAndMakeVisible (editBtn);
    addAndMakeVisible (deleteBtn);
    addAndMakeVisible (loadBtn);
    addAndMakeVisible (closeBtn);

    closeBtn.onClick = [this] { if (onClose) onClose(); };
    newBtn.onClick  = [this] { if (onCreateNew) onCreateNew(); };
    editBtn.onClick = [this]
    {
        const int selectedRow = list.getSelectedRow();
        if (isRow (selectedRow) && onEditStyle) onEditStyle (files[selectedRow]);
    };
    deleteBtn.onClick = [this]
    {
        const int selectedRow = list.getSelectedRow();
        if (! isRow (selectedRow)) return;
        const auto file = files[selectedRow];
        juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
            deleteTitle,
            deleteQuestionStart + file.getFileNameWithoutExtension() + deleteQuestionEnd,
            confirmDelete, cancelDelete, this,
            juce::ModalCallbackFunction::create ([this, file] (int result)
            {
                if (result != deleteConfirmed) return;
                file.deleteFile();
                if (onDeleteStyle) onDeleteStyle (file);   // let host clear it if it was active
                refresh();
            }));
    };
    loadBtn.onClick = [this]
    {
        const int selectedRow = list.getSelectedRow();
        if (isRow (selectedRow) && onLoadStyle) onLoadStyle (files[selectedRow]);
    };
    refresh();
}

void ArrangerStyleListComponent::refresh()
{
    files.clear();
    auto folder = IOHelper::getArrangerStylesFolder();
    for (auto& styleFile : folder.findChildFiles (juce::File::findFiles, searchSubfolders, styleFilePattern))
        files.add (styleFile);
    list.updateContent();
    repaint();
}

int ArrangerStyleListComponent::getNumRows() { return files.size(); }

void ArrangerStyleListComponent::setActiveConfigName (const juce::String& name)
{
    if (activeName == name) return;
    activeName = name;
    repaint();
}

void ArrangerStyleListComponent::paintListBoxItem (int row, juce::Graphics& g, int rowWidth, int rowHeight, bool selected)
{
    if (! isRow (row)) return;

    const auto rowName  = files[row].getFileNameWithoutExtension();
    const bool isActive = activeName.isNotEmpty() && rowName == activeName;

    if (selected)
        g.fillAll (AppColours::configSelectedRow);       // clear blue highlight for the selected row
    else if (isActive)
        g.fillAll (AppColours::configActiveRow);         // subtle green wash for the active configuration

    const juce::Colour accent = selected ? juce::Colours::white : AppColours::configActiveMark;
    const juce::Colour text   = selected ? juce::Colours::white : juce::Colours::lightgrey;

    int nameX = rowPadding;
    if (isActive)                                        // leading check mark, name indented to match
    {
        g.setColour (accent);
        g.drawText (juce::String::fromUTF8 (checkMarkUtf8), rowPadding, rowTop, checkMarkWidth, rowHeight, juce::Justification::centredLeft);
        nameX = nameAfterCheckX;
    }

    const int activeLabelW = isActive ? activeTagWidth : noWidth;
    g.setColour (text);
    g.drawText (rowName, nameX, rowTop, rowWidth - nameX - rowPadding - activeLabelW, rowHeight, juce::Justification::centredLeft);

    if (isActive)                                        // trailing "active" tag
    {
        g.setColour (accent);
        g.setFont (juce::jmin (maxTagFontHeight, (float) rowHeight - tagFontInset));
        g.drawText (activeTag, rowWidth - rowPadding - activeLabelW, rowTop, activeLabelW, rowHeight, juce::Justification::centredRight);
    }
}

void ArrangerStyleListComponent::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    if (isRow (row) && onLoadStyle) onLoadStyle (files[row]);
}

void ArrangerStyleListComponent::paint (juce::Graphics& g)
{
    g.fillAll (AppColours::configListBackground);
}

void ArrangerStyleListComponent::resized()
{
    auto area = getLocalBounds().reduced (panelMargin);
    title.setBounds (area.removeFromTop (titleHeight));
    area.removeFromTop (sectionGap);

    auto bottom = area.removeFromBottom (buttonsHeight);
    const int buttonWidth = bottom.getWidth() / buttonCount;
    newBtn.setBounds    (bottom.removeFromLeft (buttonWidth).reduced (buttonInset));
    editBtn.setBounds   (bottom.removeFromLeft (buttonWidth).reduced (buttonInset));
    deleteBtn.setBounds (bottom.removeFromLeft (buttonWidth).reduced (buttonInset));
    loadBtn.setBounds   (bottom.removeFromLeft (buttonWidth).reduced (buttonInset));
    closeBtn.setBounds  (bottom.reduced (buttonInset));

    area.removeFromBottom (sectionGap);
    list.setBounds (area);
}
