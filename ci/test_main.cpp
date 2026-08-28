#include <JuceHeader.h>
#include <iostream>

// Headless console entry for the test target. Mirrors the category flags the app's old
// --run-tests path used, but as a standalone main() so the production app carries no test code.
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // MessageManager for component/message-thread tests

    juce::String commandLine;
    for (int i = 1; i < argc; ++i)
        commandLine += juce::String (argv[i]) + " ";

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    if (commandLine.contains ("--arranger-tests"))
        runner.runTestsInCategory ("Arranger");
    else if (commandLine.contains ("--unit-tests"))
        runner.runTestsInCategory ("Unit");
    else if (commandLine.contains ("--integration-tests"))
        runner.runTestsInCategory ("Integration");
    else
        runner.runAllTests();

    int pass = 0, fail = 0;
    juce::StringArray lines;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* r = runner.getResult (i);
        pass += r->passes;
        fail += r->failures;
        lines.add (r->unitTestName + " | passes: " + juce::String (r->passes)
                   + " | failures: " + juce::String (r->failures));
        for (auto& m : r->messages)
            if (m.isNotEmpty())
                lines.add ("    " + m);
    }
    lines.add ("TOTAL | passes: " + juce::String (pass) + " | failures: " + juce::String (fail));

    const juce::String report = lines.joinIntoString ("\n");
    std::cout << report << std::endl;                                   // CI logs
    juce::File::getCurrentWorkingDirectory()
        .getChildFile ("test-results.txt")
        .replaceWithText (report);                                      // artifact

    return fail;   // exit code = number of failing assertions (0 = green)
}
