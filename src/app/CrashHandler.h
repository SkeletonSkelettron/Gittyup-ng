//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

// Leaves a minidump and a report of what happened when the application
// crashes on Windows: exceptions like access violations and stack overflows,
// fatal Qt messages, uncaught C++ exceptions, abort() and invalid parameters
// of the C runtime. They are written to the folder of the application in the
// temporary folder, and a message tells where, unless 'quiet'.
namespace CrashHandler {

void install(bool quiet);

} // namespace CrashHandler

#endif
