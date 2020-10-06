#include "rendering/VLOpenSpaceProcLoader.h"
#ifndef WIN32
#include <openspace/engine/windowdelegate.h>
#include "vislinkmodule.h"
#endif

namespace openspace {

	vislink::VLProc VLOpenSpaceProcLoader::getProc(const char* name) {
#ifdef WIN32
        return NULL;
#else
		WindowDelegate& windowDelegate = *static_cast<WindowDelegate*>((static_cast<VisLinkModule*>(module)->getPluginInfo().windowDelegate));
		std::cout << "Using windows delegate" << std::endl;
		return windowDelegate.openGLProcedureAddress(name);
#endif
	}

}
