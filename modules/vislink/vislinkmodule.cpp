/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2019                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

//#include "OpenGL.h"
#include <glbinding/gl45core/gl.h>

#include<iostream>
#include "vislinkmodule.h"
#include "rendering/renderabletest.h"
#include "rendering/renderablevislink.h"
#include <openspace/util/factorymanager.h>
#include <ghoul/misc/assert.h>
#include <ghoul/misc/templatefactory.h>

using namespace vislink;

namespace openspace {

    VisLinkModule::VisLinkModule() : OpenSpaceModule(Name) {}

    VisLinkModule::~VisLinkModule() {
        serverThread->join();
        delete serverThread;
        delete visLinkServer;
    }

    void VisLinkModule::internalInitialize(const ghoul::Dictionary&) {
        visLinkServer = new Server();
        serverThread = new std::thread(&VisLinkModule::runServer, this);

        auto fRenderable = FactoryManager::ref().factory<Renderable>();
        ghoul_assert(fRenderable, "No renderable factory existed");

        std::cout << "Initialize vislink" << std::endl;

        ghoul::TemplateFactory<Renderable>::FactoryFunction function =
            [this](bool use, const ghoul::Dictionary& dictionary, ghoul::MemoryPoolBase*) -> Renderable* {
            return use ? new RenderableVisLink(dictionary, this) : nullptr;
        };
        fRenderable->registerClass("RenderableVisLink", function);
    }


    void VisLinkModule::runServer() {
        while (true) {
            //std::cout << "Running" << std::endl;
            visLinkServer->service();
        }
    }

} // namespace openspace

