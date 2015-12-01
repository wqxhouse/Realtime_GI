#include "Realtime_GI.h"

// Scene scripts ///////////////////////////////////////
#include "CornellBoxScript.h"
#include "SponzaScript.h"

////////////////////////////////////////////////////////

inline void Realtime_GI::LoadScenes()
{
	AddScene(new CornellBoxScript);
	//AddScene(new SponzaScript);
}