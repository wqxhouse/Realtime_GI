#include "Realtime_GI.h"

// Scene scripts ///////////////////////////////////////
#include "CornellBoxScript.h"

////////////////////////////////////////////////////////

inline void Realtime_GI::LoadScenes()
{
	AddScene(new CornellBoxScript);
}