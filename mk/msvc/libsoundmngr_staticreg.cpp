// This file is automatically generated.
#include "cssysdef.h"
#include "csutil/scf.h"

// Put static linking stuff into own section.
// The idea is that this allows the section to be swapped out but not
// swapped in again b/c something else in it was needed.
#if !defined(CS_DEBUG) && defined(CS_COMPILER_MSVC)
#pragma const_seg(".CSmetai")
#pragma comment(linker, "/section:.CSmetai,r")
#pragma code_seg(".CSmeta")
#pragma comment(linker, "/section:.CSmeta,er")
#pragma comment(linker, "/merge:.CSmetai=.CSmeta")
#endif

namespace csStaticPluginInit
{
static char const metainfo_soundmngr[] =
"<?xml version=\"1.0\"?>"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>crystalspace.planeshift.sound.soundmngr</name>"
"        <implementation>SoundManager</implementation>"
"        <description>Sound Manager</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef SoundManager_FACTORY_REGISTER_DEFINED 
  #define SoundManager_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(SoundManager) 
  #endif

class soundmngr
{
SCF_REGISTER_STATIC_LIBRARY(soundmngr,metainfo_soundmngr)
  #ifndef SoundManager_FACTORY_REGISTERED 
  #define SoundManager_FACTORY_REGISTERED 
    SoundManager_StaticInit SoundManager_static_init__; 
  #endif
public:
 soundmngr();
};
soundmngr::soundmngr() {}

}
