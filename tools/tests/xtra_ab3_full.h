// xtra_ab3_full.h - a full AmiBlitz3 header for the tests of shared/xtra
// (test_xtra_ab3, test_xtra_ab3_update)
#ifndef XTRA_AB3_FULL_H
#define XTRA_AB3_FULL_H

#include <string>

// a header in the layout of AmiBlitz3's write_AB3Header
// (CLIModeSupport.ab3, commit b1a9dee150ef)
inline const std::string full =
    "; XTRA\n"
    "; Embedded .xtra Header\n"
    "; \n"
    "; General Info\n"
    "; -------------------------------------------------------\n"
    "; ExePath         = \"Development:AmiBlitz3/Tests\"\n"
    "; ExeFile         = \"\"\n"
    "; CreateIcon      = 1\n"
    "; Residents       = \",all.res\"\n"
    "; \n"
    "; Compiler\n"
    "; -------------------------------------------------------\n"
    "; StringBuffer    = 20000\n"
    "; MakeSmallest    = 1\n"
    "; FuncOptimize    = 1\n"
    "; Version         = 3.9.11\n"
    "; NumberOfBuilds  = 537\n"
    "; \n"
    "; Debugger\n"
    "; -------------------------------------------------------\n"
    "; CliArgs         = \"PATH=music:sounds\"\n"
    "; StackSize       = 64000\n"
    "; RuntimeDebug    = 0\n"
    "; DebugInfo       = 1\n"
    "; CreateDbgFile   = 1\n"
    "; OverflowCheck   = 0\n"
    "; AssemblerCheck  = 1\n"
    "; InterruptCheck  = 0\n"
    "; AutoRun         = 0\n"
    "; \n"
    "; Editor\n"
    "; -------------------------------------------------------\n"
    "; CursorLine      = 145\n"
    "; CursorColumn    = 12\n"
    "; LabelSearch     = \"errors\"\n"
    "; LabelRemark     = 1\n"
    "; LabelAll        = 1\n"
    "; LabelCase       = 1\n"
    "; LabelPosition   = 7\n"
    "; \n"
    "; Blitz Objects\n"
    "; -------------------------------------------------------\n"
    "; Max Screen      = 5\n"
    "; Max Req-Lib     = 5\n"
    "; Max ChunkyBuffer= 2\n"
    "; /XTRA\n";

#endif
