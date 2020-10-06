#ifndef __OPENSPACE_CORE___PLUGININFO___H__
#define __OPENSPACE_CORE___PLUGININFO___H__

#include <string>
#include <openspace/util/factorymanager.h>

namespace openspace {


struct PluginInfo {
	FactoryManager* factoryManager;
	std::string path;
	void* windowDelegate;
};

}

#endif