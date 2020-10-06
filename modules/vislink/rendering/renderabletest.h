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

#ifndef __OPENSPACE_MODULE_TOYVOLUME___RENDERABLETEST___H__
#define __OPENSPACE_MODULE_TOYVOLUME___RENDERABLETEST___H__

#include <openspace/rendering/renderable.h>

#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/properties/scalar/intproperty.h>
#include <openspace/properties/vector/vec3property.h>
#include <openspace/properties/vector/vec4property.h>
#include <openspace/util/plugininfo.h>
#include <openspace/util/openspacemodule.h>

#include <ghoul/opengl/ghoul_gl.h>

namespace openspace {

struct RenderData;

class RenderableTest : public Renderable {
public:
    RenderableTest(const ghoul::Dictionary& dictionary);
    ~RenderableTest();

    void initializeGL() override;
    void deinitializeGL() override;
    bool isReady() const override;
    void render(const RenderData& data, RendererTasks& tasks) override;
    void update(const UpdateData& data) override;

private:
    properties::Vec3Property _size;
    properties::IntProperty _scalingExponent;
    properties::FloatProperty _stepSize;
    properties::Vec3Property _translation;
    properties::Vec3Property _rotation;
    properties::Vec4Property _color;
    properties::FloatProperty _downScaleVolumeRendering;

    gl::GLuint vbo, vao, vshader, fshader, shaderProgram, externalTexture;
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_TOYVOLUME___RENDERABLETOYVOLUME___H__
