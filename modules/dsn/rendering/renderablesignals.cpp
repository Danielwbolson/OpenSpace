/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2018                                                               *
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

#include <modules/dsn/rendering/renderablesignals.h>

#include <modules/base/basemodule.h>
#include <openspace/documentation/documentation.h>
#include <openspace/documentation/verifier.h>
#include <openspace/engine/globals.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/opengl/programobject.h>
#include <openspace/interaction/navigationhandler.h>

namespace {
    constexpr const char* ProgramName = "SignalsProgram";
    constexpr const char* _loggerCat = "RenderableSignals";
    constexpr const char* KeyStationSites = "StationSites";
    constexpr const char* KeySpacecraftIdMap = "SpacecraftIdMap";
    constexpr const char* KeyStationSize = "Size";
    constexpr const char* KeyStationSiteColor = "SiteColor";

    constexpr const std::array <const char*, openspace::RenderableSignals::uniformCacheSize> UniformNames = {
        "modelViewStation","modelViewSpacecraft", "projectionTransform", "baseOpacity",
        "signalSpeedFactor", "segmentSizeFactor", "spacingSizeFactor", "fadeFactor"
    };

    constexpr openspace::properties::Property::PropertyInfo SiteColorsInfo = {
        "SiteColors",
        "SiteColors",
        "This value determines the RGB main color for the communication "
        "signals to and from different sites on Earth."
    };

    constexpr openspace::properties::Property::PropertyInfo LineWidthInfo = {
        "LineWidth",
        "Line Width",
        "This value specifies the line width of the signals. "
    };

    constexpr openspace::properties::Property::PropertyInfo BaseOpacityInfo = {
         "BaseOpacity",
         "Base Opacity",
         "This value specifies the base opacity of all the signal transmissions "
    };

    constexpr openspace::properties::Property::PropertyInfo SignalSpeedInfo = {
        "SignalSpeed",
        "Signal Speed",
        "Speed of signal transmission segments "
    };

    constexpr openspace::properties::Property::PropertyInfo SegmentSizeInfo = {
        "SegmentSize",
        "Segment Size",
        "Size of signal transmission segments "
    };

    constexpr openspace::properties::Property::PropertyInfo SpacingSizeInfo = {
        "SpacingSize",
        "Spacing Size",
        "Size of spacing between signal transmission segments "
    };

    constexpr openspace::properties::Property::PropertyInfo FadeFactorInfo = {
        "FadeFactor",
        "Fade Factor",
        "Factor of fading at edges of signal transmission segments "
    };
} // namespace

namespace openspace {

documentation::Documentation RenderableSignals::Documentation() {
    using namespace documentation;
    return {
        "Renderable Signals",
        "dsn_renderable_renderablesignals",
        {
            {
                SiteColorsInfo.identifier,
                new TableVerifier,
                Optional::No,
                SiteColorsInfo.description
            },
            {
                KeyStationSites,
                new TableVerifier,
                Optional::No,
                "This is a map of the individual stations to their "
                "respective site location on Earth."
            },
            {
                KeySpacecraftIdMap,
                new TableVerifier,
                Optional::No,
                "This is a map of the signal data abbreviations "
                "to the respective spacecraft asset file identifier. "
            },
            {
                LineWidthInfo.identifier,
                new DoubleVerifier,
                Optional::Yes,
                LineWidthInfo.description
            }, 
            {
               BaseOpacityInfo.identifier,
               new DoubleVerifier,
               Optional::Yes,
               BaseOpacityInfo.description
            },
            {
               SignalSpeedInfo.identifier,
               new DoubleVerifier,
               Optional::Yes,
               SignalSpeedInfo.description
            },
            {
               SegmentSizeInfo.identifier,
               new DoubleVerifier,
               Optional::Yes,
               SegmentSizeInfo.description
            },
            {
               SpacingSizeInfo.identifier,
               new DoubleVerifier,
               Optional::Yes,
               SpacingSizeInfo.description
            },
            {
               FadeFactorInfo.identifier,
               new DoubleVerifier,
               Optional::Yes,
               FadeFactorInfo.description
            }
        }
    };
}

RenderableSignals::RenderableSignals(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _lineWidth(LineWidthInfo, 2.5f, 1.f, 10.f)
    , _baseOpacity(BaseOpacityInfo, 0.3f, 0.0f, 1.0f)
    , _signalSpeedFactor(SignalSpeedInfo, 2.0f, 1.0f, 10.0f)
    , _segmentSizeFactor(SegmentSizeInfo, 10.0f, 1.0f, 100.0f)
    , _spacingSizeFactor(SpacingSizeInfo, 0.0f, 0.0f, 10.0f)
    , _fadeFactor(FadeFactorInfo, 0.5f, 0.1f, 0.5f)
{
    documentation::testSpecificationAndThrow(
        Documentation(),
        dictionary,
        "RenderableSignals"
    );

    if (dictionary.hasKeyAndValue<ghoul::Dictionary>(SiteColorsInfo.identifier)) {
        ghoul::Dictionary siteColorDictionary = dictionary.value<ghoul::Dictionary>(SiteColorsInfo.identifier);
        std::vector<std::string> siteNames = siteColorDictionary.keys();

        for (int siteIndex = 0; siteIndex < siteNames.size(); siteIndex++)
        {
            const char* siteColorIdentifier = siteNames.at(siteIndex).c_str();
            openspace::properties::Property::PropertyInfo SiteColorsInfo = {
                siteColorIdentifier,
                siteColorIdentifier,
                "This value determines the RGB main color for signals "
                "of communication to and from different sites on Earth."
            };
            std::string site = siteNames[siteIndex];
            glm::vec3 siteColor = siteColorDictionary.value<glm::vec3>(siteNames.at(siteIndex));
            _siteColors.push_back( std::make_unique<properties::Vec4Property>(
                            SiteColorsInfo,glm::vec4(siteColor,1.0), glm::vec4(0.f), glm::vec4(1.f))
            );
            _siteToIndex[siteNames.at(siteIndex)] = siteIndex;
            addProperty(_siteColors.back().get());
        }
    }

    if (dictionary.hasKeyAndValue<ghoul::Dictionary>(KeyStationSites)) {
        ghoul::Dictionary stationDictionary = dictionary.value<ghoul::Dictionary>(KeyStationSites);
        std::vector<std::string> stations = stationDictionary.keys();

        // loop the stations
        for (int i = 0; i < stations.size(); i++)
        {   
            std::string station = stations.at(i);

            ghoul::Dictionary stationPropertyDictionary = stationDictionary.value<ghoul::Dictionary>(stations.at(i));
            // loop the properties of the station
            float size = stationPropertyDictionary.value<float>(KeyStationSize);
            _stationToSize[stations.at(i)] = size;

            std::string site = stationPropertyDictionary.value<std::string>(KeyStationSiteColor);
            _stationToSite[stations.at(i)] = site;
        }
    }
 
    if (dictionary.hasKeyAndValue<double>(LineWidthInfo.identifier)) {
        _lineWidth = static_cast<float>(dictionary.value<double>(
            LineWidthInfo.identifier
        ));
    }
    addProperty(_lineWidth);

    if (dictionary.hasKeyAndValue<double>(BaseOpacityInfo.identifier)) {
        _baseOpacity = static_cast<float>(dictionary.value<double>(
            BaseOpacityInfo.identifier
        ));
    }
    addProperty(_baseOpacity);

    addProperty(_signalSpeedFactor);
    addProperty(_segmentSizeFactor);
    addProperty(_spacingSizeFactor);
    addProperty(_fadeFactor);


    std::unique_ptr<ghoul::Dictionary> dictionaryPtr = std::make_unique<ghoul::Dictionary>(dictionary);
    extractData(dictionaryPtr);
}

void RenderableSignals::initializeGL() {
    _programObject = BaseModule::ProgramObjectManager.request(
        ProgramName,
        []() -> std::unique_ptr<ghoul::opengl::ProgramObject> {
            return global::renderEngine.buildRenderProgram(
                ProgramName,
                absPath("${MODULE_DSN}/shaders/renderablesignals_vs.glsl"),
                absPath("${MODULE_DSN}/shaders/renderablesignals_fs.glsl")
            );
        }
    );

    ghoul::opengl::updateUniformLocations(*_programObject, _uniformCache, UniformNames);

    setRenderBin(Renderable::RenderBin::Overlay);

    // We don't need an index buffer, so we keep it at the default value of 0
    glGenVertexArrays(1, &_lineRenderInformation._vaoID);
    glGenBuffers(1, &_lineRenderInformation._vBufferID);

    updateVertexAttributes();
}

void RenderableSignals::deinitializeGL() {

    glDeleteVertexArrays(1, &_lineRenderInformation._vaoID);
    glDeleteBuffers(1, &_lineRenderInformation._vBufferID);

    BaseModule::ProgramObjectManager.release(
        ProgramName,
        [](ghoul::opengl::ProgramObject* p) {
            global::renderEngine.removeRenderProgram(p);
        }
    );
    _programObject = nullptr;
}

bool RenderableSignals::isReady() const {
    return _programObject != nullptr;
}

// Unbind buffers and arrays
inline void unbindGL() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RenderableSignals::updateVertexAttributes() {

    // position attributes
    glVertexAttribPointer(_vaLocVer, _sizeThreeVal, GL_FLOAT, GL_FALSE, 
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout) +
                        sizeof(FloatsVBOLayout),
                        (void*)0);
    glEnableVertexAttribArray(_vaLocVer);
    // color attributes
    glVertexAttribPointer(_vaLocCol, _sizeFourVal, GL_FLOAT, GL_FALSE,
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout) +
                        sizeof(FloatsVBOLayout),
                        (void*)(sizeof(PositionVBOLayout)));
    glEnableVertexAttribArray(_vaLocCol);
    // distance attributes
    glVertexAttribPointer(_vaLocDist, _sizeOneVal, GL_FLOAT, GL_FALSE,
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout) +
                        sizeof(FloatsVBOLayout),
                        (void*)(sizeof(PositionVBOLayout) + sizeof(ColorVBOLayout)));
    glEnableVertexAttribArray(_vaLocDist);
    // active time attribute
    glVertexAttribPointer(_vaLocTimeSinceStart, _sizeOneVal, GL_FLOAT, GL_FALSE,
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout) +
                        sizeof(FloatsVBOLayout),
                        (void*)(sizeof(PositionVBOLayout) + sizeof(ColorVBOLayout) + sizeof(float)));
    glEnableVertexAttribArray(_vaLocTimeSinceStart);
    // total transmission time attribute
    glVertexAttribPointer(_vaLocTransmissionTime, _sizeOneVal, GL_FLOAT, GL_FALSE,
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout) +
                        sizeof(FloatsVBOLayout),
                        (void*)(sizeof(PositionVBOLayout) + sizeof(ColorVBOLayout) + 2 * sizeof(float)));
    glEnableVertexAttribArray(_vaLocTransmissionTime);
};

void RenderableSignals::render(const RenderData& data, RendererTasks&) {
    _programObject->activate();

    //The stations are statically translated with respect to Earth
    //glm::dmat4 modelTransformStation = global::renderEngine.scene()->sceneGraphNode("Earth")->modelTransform();

    _programObject->setUniform(_uniformCache.modelViewStation,
        data.camera.combinedViewMatrix() * _lineRenderInformation._localTransformStation);

    _programObject->setUniform(_uniformCache.modelViewSpacecraft,
        data.camera.combinedViewMatrix()  * _lineRenderInformation._localTransformSpacecraft);

    _programObject->setUniform(_uniformCache.projection, data.camera.sgctInternal.projectionMatrix());

    _programObject->setUniform(_uniformCache.baseOpacity, _baseOpacity);
    _programObject->setUniform(_uniformCache.signalSpeedFactor, _signalSpeedFactor);
    _programObject->setUniform(_uniformCache.segmentSizeFactor, _segmentSizeFactor);
    _programObject->setUniform(_uniformCache.spacingSizeFactor, _spacingSizeFactor);
    _programObject->setUniform(_uniformCache.fadeFactor, _fadeFactor);

    const bool usingFramebufferRenderer =
        global::renderEngine.rendererImplementation() ==
        RenderEngine::RendererImplementation::Framebuffer;

    if (usingFramebufferRenderer) {
        glDepthMask(false);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    glLineWidth(_lineWidth);

    glBindVertexArray(_lineRenderInformation._vaoID);

    glDrawArrays(
        GL_LINES,
        _lineRenderInformation.first,
        _lineRenderInformation.countLines
    );

    //unbind vertex array and buffers
    unbindGL();

    if (usingFramebufferRenderer) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(true);
    }

    _programObject->deactivate();
}

void RenderableSignals::update(const UpdateData& data) {

    double currentTime = data.time.j2000Seconds();

    //Bool if the current time is within the timeframe for the currently loaded data
    const bool isTimeInFileInterval = (currentTime >= SignalManager::signalData.sequenceStartTime) &&
        (currentTime < SignalManager::signalData.sequenceEndTime);

    //Reload data if it is not relevant anymore
    if (!isTimeInFileInterval || SignalManager::signalData.needsUpdate) {

        //Bool if the current time is within the timeframe for all of our data
        const bool haveDataForTime = (currentTime >= SignalManager::fileStartTimes.front()) &&
            (currentTime < SignalManager::fileStartTimes.back());

        if (!haveDataForTime) {
            LWARNING(fmt::format("No signal data available for the time {}", data.time.UTC()));
        }

        int activeFileIndex = DataFileHelper::findFileIndexForCurrentTime(currentTime, SignalManager::fileStartTimes);
        //parse data for that file
        //LDEBUG(fmt::format("{}: Reloading SignalData for time {}", _identifier, data.time.UTC()));
        SignalManager::updateSignalData(activeFileIndex, _signalSizeBuffer);
    }

    // Make space for the vertex renderinformation
    _vertexArray.clear();

    //update focusnode information used to calculate spacecraft positions
    _focusNode = global::navigationHandler.focusNode();
    _lineRenderInformation._localTransformSpacecraft = glm::translate(glm::dmat4(1.0), _focusNode->worldPosition());
    _lineRenderInformation._localTransformStation = glm::translate(glm::dmat4(1.0), global::renderEngine.scene()->sceneGraphNode("Earth")->worldPosition());

    //Todo; keep track of active index for signalvector, or swap for loop for binary search
    for (int i = 0; i < SignalManager::signalData.signals.size(); i++) {

        SignalManager::Signal currentSignal = SignalManager::signalData.signals[i];
        if (isSignalActive(currentTime, currentSignal)) {
            currentSignal.timeSinceStart = currentTime - currentSignal.startTransmission;
            pushSignalDataToVertexArray(currentSignal);
        }
    };

 
    // ... and upload them to the GPU
    glBindVertexArray(_lineRenderInformation._vaoID);
    glBindBuffer(GL_ARRAY_BUFFER, _lineRenderInformation._vBufferID);
    glBufferData(
        GL_ARRAY_BUFFER,
        _vertexArray.size() * sizeof(float),
        _vertexArray.data(),
        GL_STATIC_DRAW
    );

    updateVertexAttributes();

    // Update the number of lines to render
    _lineRenderInformation.countLines = static_cast<GLsizei>(_vertexArray.size() / 
                                (_sizeThreeVal + _sizeFourVal + 3 *_sizeOneVal));

    //unbind vertexArray
    unbindGL();
}

// Todo: handle signalIsSending, not only signalIsActive for the signal segments
bool RenderableSignals::isSignalActive(double currentTime, SignalManager::Signal signal) {
    
    double startTimeInSeconds = signal.startTransmission;
    double endTimeInSeconds = signal.endTransmission + signal.lightTravelTime;

    if (startTimeInSeconds <= currentTime && endTimeInSeconds >= currentTime)
        return true;

    return false;
}

void RenderableSignals::extractData(std::unique_ptr<ghoul::Dictionary> &dictionary) {

    if (!SignalManager::extractMandatoryInfoFromDictionary(_identifier, dictionary)) {
        LERROR(fmt::format("{}: Did not manage to extract data.", _identifier));
    }
    else {
        LDEBUG(fmt::format("{}: Successfully read data.", _identifier));
    }
}

void RenderableSignals::pushSignalDataToVertexArray(SignalManager::Signal signal) {

    glm::vec4 color = getStationColor(signal.dishName);
    glm::dvec3 posStation = getPositionForGeocentricSceneGraphNode(signal.dishName.c_str());
    glm::dvec3 posSpacecraft = getSuitablePrecisionPositionForSceneGraphNode(signal.spacecraft.c_str());
    double distance = getDistance(signal.dishName, signal.spacecraft);
    double timeSinceStart = signal.timeSinceStart;

    //fill the render array
    _vertexArray.push_back(posStation.x);
    _vertexArray.push_back(posStation.y);
    _vertexArray.push_back(posStation.z);

    _vertexArray.push_back(color.r);
    _vertexArray.push_back(color.g);
    _vertexArray.push_back(color.b);
    _vertexArray.push_back(color.a);

    if (signal.direction == "uplink") {
        _vertexArray.push_back(0.0);
    }
    else {
        _vertexArray.push_back(distance);
    }
    _vertexArray.push_back(timeSinceStart);
    _vertexArray.push_back(signal.endTransmission-signal.startTransmission);

    _vertexArray.push_back(posSpacecraft.x);
    _vertexArray.push_back(posSpacecraft.y);
    _vertexArray.push_back(posSpacecraft.z);

    _vertexArray.push_back(color.r);
    _vertexArray.push_back(color.g);
    _vertexArray.push_back(color.b);
    _vertexArray.push_back(color.a);

    if (signal.direction == "downlink") {
        _vertexArray.push_back(0.0);
    }
    else {
        _vertexArray.push_back(distance);
    }
    _vertexArray.push_back(timeSinceStart);
    _vertexArray.push_back(signal.endTransmission - signal.startTransmission);
}


glm::dvec3 RenderableSignals::getCoordinatePosFromFocusNode(SceneGraphNode* node) {

    glm::dvec3 nodePos = node->worldPosition();
    glm::dvec3 focusNodePos = _focusNode->worldPosition();

    glm::dvec3 diff = glm::dvec3(nodePos.x - focusNodePos.x, nodePos.y - focusNodePos.y,
        nodePos.z - focusNodePos.z);

    return diff;
}

glm::dvec3 RenderableSignals::getSuitablePrecisionPositionForSceneGraphNode(std::string id) {

    glm::dvec3 position;

    if (global::renderEngine.scene()->sceneGraphNode(id)) {
        SceneGraphNode* spacecraftNode = global::renderEngine.scene()->sceneGraphNode(id);
        position = getCoordinatePosFromFocusNode(spacecraftNode);
    }
    else {
        LERROR(fmt::format("No scenegraphnode found for the spacecraft {}", id));
    }

    return position;
}

glm::dvec3 RenderableSignals::getPositionForGeocentricSceneGraphNode(const char* id) {

    glm::dvec3 position;

    if (global::renderEngine.scene()->sceneGraphNode(id)) {

        glm::dvec3 earthPos = global::renderEngine.scene()->sceneGraphNode("Earth")->worldPosition();
        glm::dvec3 stationPos= global::renderEngine.scene()->sceneGraphNode(id)->worldPosition();

        glm::dvec3 earthSurfacePos = stationPos - earthPos;
        glm::dvec3 heightAboveSurfacePos = glm::normalize(earthSurfacePos);
        heightAboveSurfacePos.x = heightAboveSurfacePos.x * _stationToSize.at(id);
        heightAboveSurfacePos.y = heightAboveSurfacePos.y * _stationToSize.at(id);
        heightAboveSurfacePos.z = heightAboveSurfacePos.z * _stationToSize.at(id);
        position = earthSurfacePos + heightAboveSurfacePos;

    }
    else {
        LERROR(fmt::format("No scenegraphnode found for the station dish {}, "
                            "drawing line from center of Earth", id));
        position = glm::dvec3(0, 0, 0);
    }

    return position;
}

glm::vec4 RenderableSignals::getStationColor(std::string dishidentifier) {

    glm::vec4 color(0.0f, 0.0f, 0.0f, 0.0f);
    std::string site;

    try {
        site = _stationToSite.at(dishidentifier);
    }
    catch (const std::exception& e) {
        LERROR(fmt::format("Station {} has no site location color, "
               "add it to your stationSiteMap in your asset file.", dishidentifier));
    }

    int siteIndex = _siteToIndex.at(site);
    color = _siteColors[siteIndex]->value();

    return color;
}

double RenderableSignals::getDistance(std::string nodeIdA, std::string nodeIdB) {

    glm::dvec3 posA = global::renderEngine.scene()->sceneGraphNode(nodeIdA)->worldPosition();
    glm::dvec3 posB = global::renderEngine.scene()->sceneGraphNode(nodeIdB)->worldPosition();

    return glm::distance(posA, posB);
}

} // namespace openspace
