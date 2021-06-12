#ifndef GAME_ENVIRONMENTMAP_H
#define GAME_ENVIRONMENTMAP_H

#include <graphics/cube_map.h>

class EnvironmentMap
{
  public:
    SharedCubeMap original;
    SharedCubeMap irradianceMap;
    SharedCubeMap prefilteredReflectionMap;

    /**
     * Creates the irradiance map needed for the PBR shader.
     *
     * @param resolution    the irradiance map averages all surrounding radiance uniformly so it doesn't have a lot of high frequency details
     * @param sampleDelta   decreasing or increasing the sample delta will increase or decrease the accuracy respectively
     */
    void createIrradianceMap(unsigned int resolution=32, float sampleDelta=0.015);

    /**
     * Creates reflection map(s) needed for the PBR shader. Different mipmap levels are used for different roughness values.
     *
     * @param resolution
     */
    void prefilterReflectionMap(unsigned int resolution=128);

    /**
     * Returns (OR CREATES the first time) a BRDF lookup texture looking like this: https://learnopengl.com/img/pbr/ibl_brdf_lut.png
     * This is needed for the PBR shader.
     *
     * THIS FUNCTION WILL UNBIND THE CURRENT FRAMEBUFFER! (actually only the first time this function is called)
     */
    static Texture &getBRDFLookUpTexture();
};


#endif
