#pragma once

#include <vector>
#include <string>
#include "Shader.hpp"

class SkyBox {
public:
    SkyBox();
    void init(); 
    void Load(std::vector<std::string> faces);
    void Draw(gps::Shader& shader);
    void cleanup();

private:
    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    unsigned int cubemapTexture = 0;
};
