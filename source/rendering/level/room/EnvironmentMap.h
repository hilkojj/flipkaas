#ifndef GAME_ENVIRONMENTMAP_H
#define GAME_ENVIRONMENTMAP_H

#include <graphics/cube_map.h>

class EnvironmentMap
{
  public:
    SharedCubeMap original;
    SharedCubeMap irradianceMap;

    /**
     * @param resolution    the irradiance map averages all surrounding radiance uniformly so it doesn't have a lot of high frequency details
     * @param sampleDelta   decreasing or increasing the sample delta will increase or decrease the accuracy respectively
     */
    void createIrradianceMap(unsigned int resolution=32, float sampleDelta=0.015);

};


#endif
