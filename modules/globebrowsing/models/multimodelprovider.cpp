/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2017                                                               *
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

#include <modules/globebrowsing/models/multimodelprovider.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/scene/scene.h>

#include <ghoul/logging/logmanager.h>

namespace {
	const std::string _loggerCat = "MultiModelProvider";
}

namespace openspace {
namespace globebrowsing {
MultiModelProvider::MultiModelProvider(const ghoul::Dictionary& dictionary) 
	: ModelProvider(dictionary) {
}

std::vector<std::shared_ptr<Subsite>> MultiModelProvider::calculate(const std::vector<std::vector<std::shared_ptr<Subsite>>> subsites, const RenderData& data) {
	std::vector<std::shared_ptr<Subsite>> subsitesInsideRadius;
	float radius = 20;

	SceneGraphNode* _parent = OsEng.renderEngine().scene()->sceneGraphNode("Mars");
	RenderableGlobe* rg = (RenderableGlobe*)_parent->renderable();

	double ellipsoidShrinkTerm = rg->interactionDepthBelowEllipsoid();
	glm::dvec3 center = _parent->worldPosition();
	glm::dmat4 globeModelTransform = rg->modelTransform();
	glm::dmat4 globeModelInverseTransform = rg->inverseModelTransform();
	glm::dvec3 cameraPos = data.camera.positionVec3();
	glm::dvec4 cameraPositionModelSpace = globeModelInverseTransform * glm::dvec4(cameraPos, 1.0);
	glm::dvec3 cameraPositionProjected = rg->ellipsoid().geodeticSurfaceProjection(cameraPositionModelSpace);
	
	for (auto s : subsites) {
		for (auto s1 : s) {
			glm::dvec3 temp = rg->ellipsoid().cartesianPosition({ s1->geodetic , 0 });
			if (glm::distance(cameraPositionProjected, temp) < radius) {
				subsitesInsideRadius.push_back(s1);
			}
		}
	}

	return subsitesInsideRadius;
}

bool MultiModelProvider::initialize() {
	return true;
}
}
}