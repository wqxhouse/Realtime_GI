#include "Realtime_GI.h"

// Scene scripts ///////////////////////////////////////
#include "CornellBoxScript.h"
#include "SponzaScript.h"
#include "DropBoxesScript.h"
////////////////////////////////////////////////////////

inline void Realtime_GI::LoadScenes()
{
	AddScene(new DropBoxesScript);
	//AddScene(new SponzaScript);
}