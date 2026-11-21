#include <JuceHeader.h>
#include <iostream>

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
    const juce::String totalLabel      = "TOTAL";
    const juce::String messageIndent   = "    ";
    const juce::String lineBreak       = "\n";
}

// Prints each "Starting test: ..." line as it happens (flushed), so when a test crashes the process
// the CI log still shows which test was running. JUCE's default only writes to the debugger output.
class ConsoleTestRunner : public juce::UnitTestRunner
{
    void logMessage (const juce::String& message) override
    {
        std::cout << message << std::endl;
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

    int totalPasses {}, totalFailures {};
    juce::StringArray lines;
    for (int resultIndex {}; resultIndex < runner.getNumResults(); ++resultIndex)
    {
        auto* result = runner.getResult (resultIndex);
        totalPasses   += result->passes;
        totalFailures += result->failures;
        lines.add (result->unitTestName + TestReport::fieldSeparator + juce::String (result->passes)
                   + TestReport::failuresLabel + juce::String (result->failures));
        for (auto& message : result->messages)
            if (message.isNotEmpty())
                lines.add (TestReport::messageIndent + message);
    }
    lines.add (TestReport::totalLabel + TestReport::fieldSeparator + juce::String (totalPasses)
               + TestReport::failuresLabel + juce::String (totalFailures));

    const juce::String report = lines.joinIntoString (TestReport::lineBreak);
    std::cout << report << std::endl;                                   // CI logs
    juce::File::getCurrentWorkingDirectory()
        .getChildFile (TestReport::resultsFileName)
        .replaceWithText (report);                                      // artifact

    return totalFailures;   // exit code = number of failing assertions (0 = green)
}
