#include <JuceHeader.h>
#include <iostream>
#include <map>

// Headless console entry for the test target. Mirrors the category flags the app's old
// --run-tests path used, but as a standalone main() so the production app carries no test code.
namespace TestCategory
{
    const juce::String unit        = "Unit";                  // pure logic and single components, no I/O outside temp files
    const juce::String arranger    = "Arranger";              // the arranger engine and its model
    const juce::String integration = "Integration";           // several components together; MIDI-hardware tests skip without devices
    const juce::String display     = "Integration-display";   // opens real windows and reads screen pixels: needs an unlocked desktop with a GPU
    const juce::String api         = "Integration-api";       // talks to the real backend and creates accounts: explicit flag only
}

namespace TestFlag
{
    const juce::String unit        = "--unit-tests";
    const juce::String arranger    = "--arranger-tests";
    const juce::String integration = "--integration-tests";
    const juce::String display     = "--display-tests";
    const juce::String api         = "--api-tests";
}

namespace TestReport
{
    const juce::String resultsFileName = "test-results.txt";
    const juce::String argumentSeparator = " ";
    const juce::String fieldSeparator  = " | passes: ";
    const juce::String failuresLabel   = " | failures: ";
    const juce::String skippedLabel    = " | skipped: ";
    const juce::String totalLabel      = "TOTAL";
    const juce::String messageIndent   = "    ";
    const juce::String lineBreak       = "\n";
    const juce::String skippedHeading  = "SKIPPED - not run in this environment:";
    const juce::String caseSeparator   = " / ";
    const juce::String reasonSeparator = ": ";

    // A test that can't run here (no MIDI device, no display...) logs a message starting with this
    // instead of passing silently. Each one counts as a skipped test case.
    const juce::String skippedMarker = "SKIPPED - ";
}

// Prints each "Starting test: ..." line as it happens (flushed), so when a test crashes the process
// the CI log still shows which test was running. JUCE's default only writes to the debugger output.
// Also collects the skip messages, per test case, so the report can count and list them.
class ConsoleTestRunner : public juce::UnitTestRunner
{
public:
    juce::StringArray skipReasonsFor (int resultIndex) const
    {
        const auto found = skipReasonsByResult.find (resultIndex);
        return found == skipReasonsByResult.end() ? juce::StringArray() : found->second;
    }

private:
    static constexpr int newestResultOffset = 1;   // the case being run is the last result so far

    std::map<int, juce::StringArray> skipReasonsByResult;

    void logMessage (const juce::String& message) override
    {
        std::cout << message << std::endl;

        if (message.startsWith (TestReport::skippedMarker) && getNumResults() >= newestResultOffset)
            skipReasonsByResult[getNumResults() - newestResultOffset]
                .add (message.fromFirstOccurrenceOf (TestReport::skippedMarker, false, false));
    }
};

static juce::Array<juce::UnitTest*> testsInCategories (const juce::StringArray& categories)
{
    juce::Array<juce::UnitTest*> selected;
    for (auto* test : juce::UnitTest::getAllTests())
        if (categories.contains (test->getCategory()))
            selected.add (test);
    return selected;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // MessageManager for component/message-thread tests

    constexpr int firstUserArgumentIndex = 1;   // argv[0] is the program path
    juce::StringArray arguments;
    for (int argumentIndex = firstUserArgumentIndex; argumentIndex < argc; ++argumentIndex)
        arguments.add (argv[argumentIndex]);
    const auto commandLine = arguments.joinIntoString (TestReport::argumentSeparator);

    ConsoleTestRunner runner;
    runner.setAssertOnFailure (false);

    if (commandLine.contains (TestFlag::arranger))
        runner.runTestsInCategory (TestCategory::arranger);
    else if (commandLine.contains (TestFlag::unit))
        runner.runTestsInCategory (TestCategory::unit);
    else if (commandLine.contains (TestFlag::integration))
        runner.runTestsInCategory (TestCategory::integration);
    else if (commandLine.contains (TestFlag::display))
        runner.runTestsInCategory (TestCategory::display);
    else if (commandLine.contains (TestFlag::api))
        runner.runTestsInCategory (TestCategory::api);
    else
        // Everything that is safe to run unattended: never the API tests, which create real accounts.
        runner.runTests (testsInCategories ({ TestCategory::unit, TestCategory::arranger,
                                              TestCategory::integration, TestCategory::display }));

    int totalPasses {}, totalFailures {}, totalSkipped {};
    juce::StringArray lines, skippedLines;
    for (int resultIndex {}; resultIndex < runner.getNumResults(); ++resultIndex)
    {
        auto* result = runner.getResult (resultIndex);
        const auto skipReasons = runner.skipReasonsFor (resultIndex);
        totalPasses   += result->passes;
        totalFailures += result->failures;
        totalSkipped  += skipReasons.size();
        lines.add (result->unitTestName + TestReport::fieldSeparator + juce::String (result->passes)
                   + TestReport::failuresLabel + juce::String (result->failures)
                   + TestReport::skippedLabel + juce::String (skipReasons.size()));
        for (auto& message : result->messages)
            if (message.isNotEmpty())
                lines.add (TestReport::messageIndent + message);
        for (auto& reason : skipReasons)
            skippedLines.add (TestReport::messageIndent + result->unitTestName + TestReport::caseSeparator
                              + result->subcategoryName + TestReport::reasonSeparator + reason);
    }
    if (! skippedLines.isEmpty())
    {
        lines.add (TestReport::skippedHeading);
        lines.addArray (skippedLines);
    }
    // Keep "TOTAL | passes: N" first: CI reads the pass count with that pattern.
    lines.add (TestReport::totalLabel + TestReport::fieldSeparator + juce::String (totalPasses)
               + TestReport::failuresLabel + juce::String (totalFailures)
               + TestReport::skippedLabel + juce::String (totalSkipped));

    const juce::String report = lines.joinIntoString (TestReport::lineBreak);
    std::cout << report << std::endl;                                   // CI logs
    juce::File::getCurrentWorkingDirectory()
        .getChildFile (TestReport::resultsFileName)
        .replaceWithText (report);                                      // artifact

    return totalFailures;   // exit code = number of failing assertions (0 = green)
}
