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

#include <modules/toyvolume/rendering/renderabletest.h>

#include <openspace/engine/globals.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/rendering/raycastermanager.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/glm.h>


namespace {
    constexpr const char* _loggerCat = "Renderable Test";
    constexpr openspace::properties::Property::PropertyInfo SizeInfo = {
        "Size",
        "Size",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo ScalingExponentInfo = {
        "ScalingExponent",
        "Scaling Exponent",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo StepSizeInfo = {
        "StepSize",
        "Step Size",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo TranslationInfo = {
        "Translation",
        "Translation",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo RotationInfo = {
        "Rotation",
        "Euler rotation",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo ColorInfo = {
        "Color",
        "Color",
        "" // @TODO Missing documentation
    };

    constexpr openspace::properties::Property::PropertyInfo DownscaleVolumeRenderingInfo = {
        "Downscale",
        "Downscale Factor Volume Rendering",
        "This value set the downscaling factor"
        " when rendering the current volume."
    };
} // namespace

namespace openspace {

RenderableTest::RenderableTest(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _size(SizeInfo, glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f), glm::vec3(10.f))
    , _scalingExponent(ScalingExponentInfo, 1, -10, 20)
    , _stepSize(StepSizeInfo, 0.02f, 0.01f, 1.f )
    , _translation(TranslationInfo, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(10.f))
    , _rotation(RotationInfo, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0), glm::vec3(6.28f))
    , _color(ColorInfo, glm::vec4(1.f, 0.f, 0.f, 0.1f), glm::vec4(0.f), glm::vec4(1.f))
    , _downScaleVolumeRendering(DownscaleVolumeRenderingInfo, 1.f, 0.1f, 1.f)
{
    if (dictionary.hasKeyAndValue<double>(ScalingExponentInfo.identifier)) {
        _scalingExponent = static_cast<int>(
            dictionary.value<double>(ScalingExponentInfo.identifier)
        );
    }

    if (dictionary.hasKeyAndValue<glm::vec3>(SizeInfo.identifier)) {
        _size = dictionary.value<glm::vec3>(SizeInfo.identifier);
    }

    if (dictionary.hasKeyAndValue<glm::vec3>(TranslationInfo.identifier)) {
        _translation = dictionary.value<glm::vec3>(TranslationInfo.identifier);
    }

    if (dictionary.hasKeyAndValue<glm::vec3>(RotationInfo.identifier)) {
        _rotation = dictionary.value<glm::vec3>(RotationInfo.identifier);
    }

    if (dictionary.hasKeyAndValue<glm::vec4>(ColorInfo.identifier)) {
        _color = dictionary.value<glm::vec4>(ColorInfo.identifier);
    }

    if (dictionary.hasKeyAndValue<double>(StepSizeInfo.identifier)) {
        _stepSize = static_cast<float>(dictionary.value<double>(StepSizeInfo.identifier));
    }

    _downScaleVolumeRendering.setVisibility(
        openspace::properties::Property::Visibility::Developer
    );
    if (dictionary.hasKey("Downscale")) {
        _downScaleVolumeRendering = dictionary.value<float>("Downscale");
    }

    /*if (dictionary.hasKey("Steps")) {
        _rayCastSteps = static_cast<int>(dictionary.value<float>("Steps"));
    }
    else {
        LINFO("Number of raycasting steps not specified for ToyVolume."
            " Using default value.");
    }*/

}

RenderableTest::~RenderableTest() {}


    /// Compiles shader
GLuint compileShader(const std::string& shaderText, GLenum shaderType) {
    const char* source = shaderText.c_str();
    int length = static_cast<int>(shaderText.size());
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);
    GLboolean status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        std::cerr << &log[0];
    }

    return shader;
}

/// links shader program
void linkShaderProgram(GLuint shaderProgram) {
    glLinkProgram(shaderProgram);
    GLboolean status;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetProgramInfoLog(shaderProgram, length, &length, &log[0]);
        std::cerr << "Error compiling program: " << &log[0] << std::endl;
    }
}

void initGL() {

}

void RenderableTest::initializeGL() {

	    // Init GL
           /* glEnable(GL_DEPTH_TEST);
            glClearDepth(1.0f);
            glDepthFunc(GL_LEQUAL);
            glClearColor(0, 0, 0, 1);*/

            // Create VBO
            GLfloat vertices[]  = { 
                -0.5f, -0.5f, 0.0f, 
                0.5f, -0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                -0.5f, 0.5f, 0.0f, 
                -0.5f, -0.5f, 0.0f};

            // normal array
            GLfloat normals[]   = { 0, 0, 1,   0, 0, 1,   0, 0, 1,    0, 0, 1,   0, 0, 1,  0, 0, 1    };    // v6-v5-v4


            // color array
            GLfloat colors[]    = { 1, 0, 0,   0, 0, 0,   0, 1, 0,   0, 1, 0,   1, 1, 0 ,  1, 0, 0};    // v6-v5-v4

            // Allocate space and send Vertex Data
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)+sizeof(normals)+sizeof(colors), 0, GL_STATIC_DRAW);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(normals), normals);
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices)+sizeof(normals), sizeof(colors), colors);

            // Create vao
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), static_cast<char*>(0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), static_cast<char*>(0) + sizeof(vertices));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), static_cast<char*>(0) + sizeof(vertices) + sizeof(normals));

            // Create shader
            std::string vertexShader =
                    "#version 330 \n"
                    "layout(location = 0) in vec3 position; "
                    "layout(location = 1) in vec3 normal; "
                    "layout(location = 2) in vec3 color; "
                    ""
                    "uniform mat4 ProjectionMatrix; "
                    "uniform mat4 ViewMatrix; "
                    "uniform mat4 ModelMatrix; "
                    "uniform mat4 NormalMatrix; "
                    ""
                    "out vec3 col;"
                    ""
                    "void main() { "
                    "   gl_Position = ProjectionMatrix*ViewMatrix*ModelMatrix*vec4(position, 1.0); "
                    "   col = color;"
                    "}";
            vshader = compileShader(vertexShader, GL_VERTEX_SHADER);

            std::string fragmentShader =
                    "#version 330 \n"
                    "in vec3 col;"
                    "out vec4 colorOut;"
                    "uniform sampler2D tex; "
                    ""
                    "void main() { "
                    //"   vec4 texColor = texture(tex, col.xy);"
                    //"   colorOut = texColor; "
                    "   colorOut = vec4(1,0,0,1); "
                    "}";
            fshader = compileShader(fragmentShader, GL_FRAGMENT_SHADER); 

            // Create shader program
            shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vshader);
            glAttachShader(shaderProgram, fshader);
            linkShaderProgram(shaderProgram);


            GLenum format = GL_RGBA;
            GLenum internalFormat = GL_RGBA;
            GLenum type = GL_UNSIGNED_BYTE;

            /*mainImage.addComponent(new Image("app/textures/test.png"));
            mainImage.update();
            Image* image = mainImage.getComponent<Image>();*/

            /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);*/

    //_raycaster = std::make_unique<ToyVolumeRaycaster>(_color);
    //_raycaster->initialize();

    //global::raycasterManager.attachRaycaster(*_raycaster.get());

    /*std::function<void(bool)> onChange = [&](bool enabled) {
        if (enabled) {
            global::raycasterManager.attachRaycaster(*_raycaster.get());
        }
        else {
            global::raycasterManager.detachRaycaster(*_raycaster.get());
        }
    };*/

    //onEnabledChange(onChange);

    addProperty(_size);
    addProperty(_scalingExponent);
    addProperty(_stepSize);
    addProperty(_translation);
    addProperty(_rotation);
    addProperty(_color);
    addProperty(_downScaleVolumeRendering);
}

void RenderableTest::deinitializeGL() {
    /*if (_raycaster) {
        global::raycasterManager.detachRaycaster(*_raycaster.get());
        _raycaster = nullptr;
    }*/
}

bool RenderableTest::isReady() const {
    // @TODO isReady function needs to be filled
    return true;
}



void RenderableTest::update(const UpdateData& data) {
    /*if (_raycaster) {
        glm::mat4 transform = glm::translate(
            glm::mat4(1.0),
            static_cast<glm::vec3>(_translation) *
                std::pow(10.0f, static_cast<float>(_scalingExponent))
        );
        glm::vec3 eulerRotation = static_cast<glm::vec3>(_rotation);
        transform = glm::rotate(transform, eulerRotation.x, glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, eulerRotation.y, glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, eulerRotation.z,  glm::vec3(0, 0, 1));

        transform = glm::scale(
            transform,
            static_cast<glm::vec3>(_size) *
                std::pow(10.0f, static_cast<float>(_scalingExponent))
        );

        _raycaster->setColor(_color);
        _raycaster->setStepSize(_stepSize);
        _raycaster->setModelTransform(transform);
        _raycaster->setTime(data.time.j2000Seconds());
        _raycaster->setDownscaleRender(_downScaleVolumeRendering);
        _raycaster->setMaxSteps(_rayCastSteps);
    }*/
}

void RenderableTest::render(const RenderData& data, RendererTasks& tasks) {
    /*RaycasterTask task { _raycaster.get(), data };
    tasks.raycasterTasks.push_back(task);*/
        /*const glm::mat4 modelTransform =
        	glm::mat4(glm::translate(glm::dmat4(1.0), data.modelTransform.translation) *
        	data.modelTransform.rotation *
        	glm::scale(glm::dmat4(1.0), glm::dvec3(data.modelTransform.scale)) *
        	glm::dmat4(1.0));
    const glm::dmat4 modelViewTransform =
        data.camera.combinedViewMatrix() * modelTransform;

    	_shader->setUniform("modelViewProjectionTransform",
        data.camera.projectionMatrix() * glm::mat4(modelViewTransform));

    	_shader->setUniform("modelViewTransform",
        glm::mat4(data.camera.combinedViewMatrix() * glm::dmat4(modelViewTransform)));*/

        glm::mat4 view = glm::mat4(data.camera.combinedViewMatrix());
        glm::mat4 proj = glm::mat4(data.camera.projectionMatrix());
        glm::mat4 model = glm::mat4(glm::translate(glm::dmat4(1.0), data.modelTransform.translation) *
        	glm::dmat4(data.modelTransform.rotation) *
        	glm::scale(glm::dmat4(1.0), glm::dvec3(data.modelTransform.scale)) *
        	glm::dmat4(1.0));

        model = model*glm::scale(
            glm::mat4(1.0),
            static_cast<glm::vec3>(_size) *
                std::pow(10.0f, static_cast<float>(_scalingExponent))
        );

        // Set shader parameters
        glUseProgram(shaderProgram);
        GLint loc = glGetUniformLocation(shaderProgram, "ProjectionMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
        loc = glGetUniformLocation(shaderProgram, "ViewMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
        loc = glGetUniformLocation(shaderProgram, "ModelMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));

        // Draw quad
        glBindVertexArray(vao);
        /*glActiveTexture(GL_TEXTURE0+0);
        glBindTexture(GL_TEXTURE_2D, externalTexture);
        loc = glGetUniformLocation(shaderProgram, "tex");
        glUniform1i(loc, 0);*/

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
}

} // namespace openspace
