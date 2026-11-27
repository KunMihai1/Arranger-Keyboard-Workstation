#include "ArrangerStyleListComponent.h"
#include "IOHelper.h"
#include "Stage/StageAlert.h"
#include "UI/Sections/SectionsPageLayout.h"

namespace
{
    const juce::String deleteTitle        = "Delete configuration";
    const juce::String deleteQuestionStart = "Delete \"";
    const juce::String deleteQuestionEnd   = "\"?\nThis cannot be undone.";
    const juce::String confirmDelete      = "Delete";
    const juce::String styleFilePattern   = "*.style";
    constexpr bool     searchSubfolders   = false;
    const char* const  activeMarkUtf8     = "\xe2\x97\x8f";   // "●": the active configuration
    constexpr int      buttonCount        = 4;
    constexpr int      oneGapFewer        = 1;
    constexpr bool     verticalOnly       = false;
    constexpr bool     scrollsByWheel     = true;

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }
}

ArrangerStyleListComponent::ArrangerStyleListComponent()
{
    addAndMakeVisible (heading);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setOutlineThickness (int {});
    list.getViewport()->setScrollBarsShown (verticalOnly, verticalOnly, scrollsByWheel, verticalOnly);   // scrolls by wheel, as the LCD lists
    list.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (list);
    for (auto* button : { &newBtn, &editBtn, &loadBtn, &deleteBtn })
    {
        button->setMouseClickGrabsKeyboardFocus (false);   // the PC keyboard keeps playing after a press
        addAndMakeVisible (button);
    }

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

        const juce::Component::SafePointer<ArrangerStyleListComponent> safeList (this);
        StageAlert::showQuestion (deleteTitle,
            deleteQuestionStart + file.getFileNameWithoutExtension() + deleteQuestionEnd,
            confirmDelete,
            [safeThis = safeList, file]
            {
                file.deleteFile();
                if (safeThis == nullptr) return;
                if (safeThis->onDeleteStyle) safeThis->onDeleteStyle (file);   // let host clear it if it was active
                safeThis->refresh();
            }, this);
    };

    loadBtn.onClick = [this]
    {
        const int selectedRow = list.getSelectedRow();
        if (isRow (selectedRow) && onLoadStyle) onLoadStyle (files[selectedRow]);
    };

    setScale (scale);
    refresh();
}

ArrangerStyleListComponent::~ArrangerStyleListComponent()
{
    list.setModel (nullptr);
}

void ArrangerStyleListComponent::setScale (float newScale)
{
    scale = newScale;
    heading.setScale (scale);
    for (auto* button : { &newBtn, &editBtn, &loadBtn, &deleteBtn })
        button->setScale (scale);
    list.setRowHeight (scaled (StageMetrics::lcdRowHeight, scale));
    resized();
    repaint();
}

void ArrangerStyleListComponent::refresh()
{
    const auto selected = getSelectedFile();
    files.clear();
    auto folder = IOHelper::getArrangerStylesFolder();
    for (auto& styleFile : folder.findChildFiles (juce::File::findFiles, searchSubfolders, styleFilePattern))
        files.add (styleFile);
    list.updateContent();
    selectFile (selected);   // keep the selection across a rescan (a save, a delete)
    repaint();
}

juce::File ArrangerStyleListComponent::getSelectedFile() const
{
    const int selectedRow = list.getSelectedRow();
    return isRow (selectedRow) ? files[selectedRow] : juce::File();
}

void ArrangerStyleListComponent::selectFile (const juce::File& file)
{
    const int row = files.indexOf (file);
    if (isRow (row))
        list.selectRow (row);
    else
        list.deselectAllRows();
}

SegButton& ArrangerStyleListComponent::getButton (const juce::String& text)
{
    for (auto* button : { &newBtn, &editBtn, &loadBtn })
        if (button->getButtonText() == text)
            return *button;
    return deleteBtn;
}

int ArrangerStyleListComponent::getNumRows() { return files.size(); }

void ArrangerStyleListComponent::setActiveConfigName (const juce::String& name)
{
    if (activeName == name) return;
    activeName = name;
    list.repaint();
}

void ArrangerStyleListComponent::paintListBoxItem (int row, juce::Graphics& g, int rowWidth, int rowHeight, bool selected)
{
    if (! isRow (row)) return;
    const auto rowName  = files[row].getFileNameWithoutExtension();
    const bool isActive = activeName.isNotEmpty() && rowName == activeName;
    StageLcd::paintRow (g, { rowWidth, rowHeight }, scale, rowName,
                        isActive ? juce::String::fromUTF8 (activeMarkUtf8) : juce::String(), selected);
}

void ArrangerStyleListComponent::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    if (isRow (row) && onLoadStyle) onLoadStyle (files[row]);
}

void ArrangerStyleListComponent::paint (juce::Graphics& g)
{
    StageLcd::paintPanel (g, list.getBounds().expanded (int {}, scaled (SectionsPageMetrics::listPaddingY, scale)).toFloat(), scale);
}

void ArrangerStyleListComponent::resized()
{
    // The page gives this component the left column from the heading down to the buttons (SectionsPageLayout).
    using namespace SectionsPageMetrics;
    auto area = getLocalBounds();
    heading.setBounds (area.removeFromTop (scaled (headingHeight, scale)));
    area.removeFromTop (scaled (itemGap, scale));
    auto buttons = area.removeFromBottom (scaled (buttonHeight, scale));
    area.removeFromBottom (scaled (itemGap, scale));
    list.setBounds (area.reduced (int {}, scaled (listPaddingY, scale)));

    // Four buttons share the width exactly.
    const int gap = scaled (buttonGap, scale);
    const float width = static_cast<float> (buttons.getWidth() - (buttonCount - oneGapFewer) * gap) / static_cast<float> (buttonCount);
    float left = static_cast<float> (buttons.getX());
    for (auto* button : { &newBtn, &editBtn, &loadBtn, &deleteBtn })
    {
        button->setBounds (juce::Rectangle<float> (left, static_cast<float> (buttons.getY()), width, static_cast<float> (buttons.getHeight())).toNearestInt());
        left += width + static_cast<float> (gap);
    }
}
