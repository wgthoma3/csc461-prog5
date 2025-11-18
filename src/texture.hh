#pragma once

#include <array>

// Material Format
struct Material {
    float ambient[3];
    float diffuse[3];
    float specular[3];
    int exponent;
    float alpha;
    int texture; // GL id of the texture to use
    // optional constructor for std::array objects
    Material(const std::array<float, 3>& amb,
             const std::array<float, 3>& diff,
             const std::array<float, 3>& spec,
             int n,
             float alpha = 1.0,
             const char* texture = nullptr
         );
};

/**
 * Load raw pixels from texture file
 * Defined under main
 */
int load_texture(const char* path, bool no_mirror = false);

/**
 * Load raw pixels from texture file
 * Defined under main
 */
int load_textures(int cnt, const char* path[]);

void free_textures();

/**
 *
 * HELPFUL MACROS
 *
 */

// binds texture to unit1
#define SET_TEXTURE(ID, S, UNIT) \
{ \
    glBindTexture(GL_TEXTURE_2D, ID); \
    glUniform1i(S, UNIT); \
}
