#pragma once

// System includes
#include <vector>
// Local libraries
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
// local includes
#include "texture.hh"

enum ObjType {
    LIGHT,
    TRIANGLE,
    ELLIPSOID
};

struct Object {
    ObjType type;
    int id; // order of obj creation
    int icnt = 0; // number of triangles
    int vcnt = 0; // number of unique vertices defining object
    unsigned vbo = 0, // obj's vertex buffer
             ebo = 0; // obj's element buffer
    glm::vec3 pos; // object position
    glm::mat4 rotation, // rotation matrix
              scale; // scaling matrix
    Material material; // obj's material

    // Hacky way of differentiating constructors
    // C++ can't have similar starting parameters
    //   for function overloads

    // triangle constructor
    Object(const std::vector<std::array<float, 3>>& vertices,
           const std::vector<std::array<float, 3>>& normals,
           const std::vector<std::array<float, 2>>& uvs,
           const std::vector<std::array<unsigned short, 3>>& triangles,
           const std::array<float, 3>& amb,
           const std::array<float, 3>& diff,
           const std::array<float, 3>& spec,
           const int n,
           const float alpha,
           const char* texture
       );
    // ellipsoid constructor
    Object(float x, float y, float z, float a, float b, float c,
           const std::array<float, 3>& amb,
           const std::array<float, 3>& diff,
           const std::array<float, 3>& spec,
           const int n,
           const float alpha,
           const char* texture
       );
    // light constructor
    Object(const std::array<float, 3>& amb,
           const std::array<float, 3>& diff,
           const std::array<float, 3>& spec,
           float x, float y, float z
       );
    ~Object(); // frees GL resources
};

//.Similar to object with less values
struct Eye {
    float fov = 90.0f,
          far_z = 100.0f,
          near_z = 0.1f;
    float pitch,
          yaw;
    
    glm::vec3 pos;
    glm::vec3 _up;
    glm::vec3 _forward;
    glm::vec3 _right;
    // glm::mat4 view; // view matrix

    Eye(const glm::vec3& pos, const glm::vec3& lookat, const glm::vec3& up, float fov = 90.0f);
    glm::mat4 view();
    glm::mat4 project(float aspect_ratio);
    void update(); // recalculate direction based on pitch/yaw/lookat
    void mouse(float x, float y);
    void move(const glm::vec2& normal, float speed);
    void set_target(const glm::vec3& tpos);
};

// CONTAINERS //
extern std::vector<Object> lights;
extern std::vector<Object> objects;

/**
 * Load Object Data from JSON files
 */

void load_objects_from_json(const char* path, bool text = false);


void load_json_files(int cnt, const char* paths[]);

// Helper macros for indexing buffers during render
// TODO should swap this out for a more dynamic solution
#define BUFFER_STRIDE sizeof(float) * 6 + sizeof(float) * 2
