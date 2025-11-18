// System libs
#include <SDL3/SDL_log.h>
#include <GLES2/gl2.h>
#include <unordered_map>
#include <string>
// local libs
#define STB_IMAGE_IMPLEMENTATION // required for STB
#include "stb_image.h"
// local includes
#include "defs.hh"
#include "texture.hh"

static int texture_cnt;
static GLuint texture_dir[MAX_TEXTURES]; // texture buffer id container

// Associate texture name with GL id
// `static` so I can write to in main.cpp and obj.cpp
//    extremely evil, omw to programmer hell
std::unordered_map<std::string, GLuint> textures; // global texture container

int load_texture(const char* path, bool no_mirror) {
    GLuint id = 0;
    if(path) { // ignore null path
        // TODO apparently count() and contains() are new C++ standards
        // unordered_map::find is the classical way
        if(textures.count(path)) { // texture already loaded 
            id = textures[path]; // use existing id
            #ifdef DEBUG // Print status
                SDL_Log("%s already loaded (id=%d)", path, id);
            #endif
            
        } else { // need to load
            #ifdef USE_EMBEDDED_DATA // check if path corresponds with embedded data
            #endif

            int width, height, channels;

            // print status
            SDL_Log("\tLoading %s...", path);

            
            stbi_set_flip_vertically_on_load(true); // fix flipping issue
            unsigned char* data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha); // load image data from file
            if(data) {
                // Load into GL //
                glGenTextures(1, &id); // not great, TODO generate all at once
                glBindTexture(GL_TEXTURE_2D, id); // use current texture buffer
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                // set texture parameters
                if(no_mirror) {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // wrap u parameter
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // wrap v parameter
                } else {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // wrap u parameter
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); // wrap v parameter
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // use linear filtering
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glBindTexture(GL_TEXTURE_2D, 0); // unbind to prevent accidental modification
                stbi_image_free(data); // free image memory

                textures[path] = id; // mark texture as loaded

                #ifdef DEBUG // print status
                    SDL_Log("%s loaded with ID %d", path, id);
                #endif
                
            } else { // image failed to load
                SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to open texture %s", path);
            }
        }
    #ifdef DEBUG // log nullptr encounter
    } else {
        SDL_Log("load_texture was given NULL path!");

    #endif
    }

    return id;
}

int load_textures(int cnt, const char* path[]) {
    return 0; // TODO not implemented
}

void free_textures() {
    for(const auto& texture : textures) { // free all textures in map
        glDeleteTextures(1, &texture.second);
    }
    textures.clear(); // mark all as unloaded
}

Material::Material(
                   const std::array<float, 3>& amb, const std::array<float, 3>& diff,
                   const std::array<float, 3>& spec, const int n, float alpha, const char* texture) :
 ambient{amb[0], amb[1], amb[2]},
 diffuse{diff[0], diff[1], diff[2]},
 specular{spec[0], spec[1], spec[2]},
 exponent(n), alpha(alpha), texture(load_texture(texture)) {
     
}


