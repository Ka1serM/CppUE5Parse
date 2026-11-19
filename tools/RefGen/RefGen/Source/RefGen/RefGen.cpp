#include "Modules/ModuleManager.h"

#if defined(_MSC_VER) && _MSC_VER < 1910
// The launcher's 4.1x engines were built with an early VS2015 that mangles an empty template pack as $$V; later
// v140 compilers emit $$$V. Point the module boilerplate's imports (4.12 and 4.10 forms) at the engine's spelling (unused when it matches).
#pragma comment(linker, "/alternatename:__imp_?GetObjectArrayForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAVFFixedUObjectArray@@$$$V@@XZ=__imp_?GetObjectArrayForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAVFFixedUObjectArray@@$$V@@XZ")
#pragma comment(linker, "/alternatename:__imp_?GetObjectArrayForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAPEAPEAVUObjectBase@@$$$V@@XZ=__imp_?GetObjectArrayForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAPEAPEAVUObjectBase@@$$V@@XZ")
#pragma comment(linker, "/alternatename:__imp_?GetSerialNumberBlocksForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAPEAPEAH$$$V@@XZ=__imp_?GetSerialNumberBlocksForDebugVisualizersDelegate@FCoreDelegates@@SAAEAV?$TBaseDelegate@PEAPEAPEAH$$V@@XZ")
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, RefGen, "RefGen");
