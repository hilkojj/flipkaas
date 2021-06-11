#include <utils/gu_error.h>
#include <graphics/shader_asset.h>
#include <graphics/3d/mesh.h>
#include "EnvironmentMap.h"


void EnvironmentMap::createIrradianceMap(unsigned int resolution, float sampleDelta)
{
    // Code based on: https://learnopengl.com/PBR/IBL/Diffuse-irradiance

    if (!original)
        throw gu_err("There is no original CubeMap!");

    ShaderProgram convolutionShader("convolution shader",
                                    File::readString("assets/shaders/ibl/irradiance_map_convolution.vert").c_str(),
                                    File::readString("assets/shaders/ibl/irradiance_map_convolution.frag").c_str());

    // create cubemap in OpenGL:
    unsigned int irradianceMapId;
    glGenTextures(1, &irradianceMapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapId);
    for (unsigned int i = 0; i < 6; ++i)
    {
        #ifdef EMSCRIPTEN   // using rgbA instead of rgb fixes WebGL error: "Framebuffer not complete. (status: 0x8cd6) COLOR_ATTACHMENT0: Attachment has an effective format of RGB16F, which is not renderable."
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, resolution, resolution, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        #else
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, resolution, resolution, 0,
                     GL_RGB, GL_FLOAT, nullptr);
        #endif
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // create frame and render buffers in OpenGL:
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
    // todo: depth buffer not needed if culling is inverted
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, captureRBO);

    // render to new cubemap using convolution shader:
    mat4 captureProjection = perspective(radians(90.f), 1.f,
                                         .1f, 10.f);
    mat4 captureViews[] =
    {
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3( 1.0f, 0.0f, 0.0f),
               vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f),
               vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3( 0.0f, 1.0f, 0.0f),
               vec3(0.0f, 0.0f, 1.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3( 0.0f, -1.0f, 0.0f),
               vec3(0.0f, 0.0f, -1.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3( 0.0f, 0.0f, 1.0f),
               vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3( 0.0f, 0.0f, -1.0f),
               vec3(0.0f, -1.0f, 0.0f))
    };

    convolutionShader.use();
    original->bind(0);
    glUniform1i(convolutionShader.location("environmentMap"), 0);

    glUniform1f(convolutionShader.location("sampleDelta"), sampleDelta);

    glUniformMatrix4fv(convolutionShader.location("projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(convolutionShader.location("view"), 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMapId, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Mesh::getCube()->render();
    }

    // TODO: destroy framebuffer and attachments
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gu::width, gu::height);

    irradianceMap = std::make_shared<CubeMap>(irradianceMapId, resolution, resolution);
}
