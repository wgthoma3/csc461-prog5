// System libs
#include <fstream>
#include <SDL3/SDL_log.h>
// Local libs
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include "nlohmann/json.hpp"
// Local includes
#include "defs.hh"
#include "obj.hh"
#include "shader.hh"
#include "texture.hh"

using json = nlohmann::json;

// CONTAINERS //
std::vector<Object> lights;
std::vector<Object> objects;

// HELPER: gets type based on value heuristics
static inline ObjType find_type_of_json(const json& data) {
    if(data.contains("vertices")) // if vertices entry exist, must be a triangle object
        return ObjType::TRIANGLE;
    else if(data.contains("a")) // if a,b,c entries exist, must be an ellipsoid
        return ObjType::ELLIPSOID;
    else
        return ObjType::LIGHT;
}

// triangle constructor
Object::Object(const std::vector<std::array<float, 3>>& vertices, const std::vector<std::array<float, 3>>& normals, const std::vector<std::array<float, 2>>& uvs, const std::vector<std::array<unsigned short, 3>>& triangles, const std::array<float, 3>& amb, const std::array<float, 3>& diff, const std::array<float, 3>& spec, const int n, const float alpha, const char* texture) :
    type(ObjType::TRIANGLE),
    icnt(triangles.size() * 3), // 3 indices per triangle
    vcnt(vertices.size()),
    material(amb, diff, spec, n, alpha, texture),
    rotation(glm::mat4(1.0f)), // initialize with identity matrix
    scale(glm::mat4(1.0f))
{
    std::vector<float> _data(vertices.size() * 8); // temporary vertex storage

    // -- Object Center --
    glm::vec3 min_coords(std::numeric_limits<float>::max());
    glm::vec3 max_coords(std::numeric_limits<float>::lowest());
    int vcnt = vertices.size(); // number of vertices in object
    // find bounding box center
    for(int i = 0; i < vcnt; ++i) {
        // update minimum and maximum coords (new and based)
        // need to find the smallest axis-aligned box that can enclose all verts
        min_coords.x = glm::min(min_coords.x, vertices[i][0]);
        min_coords.y = glm::min(min_coords.y, vertices[i][1]);
        min_coords.z = glm::min(min_coords.z, vertices[i][2]);
        max_coords.x = glm::max(max_coords.x, vertices[i][0]);
        max_coords.y = glm::max(max_coords.y, vertices[i][1]);
        max_coords.z = glm::max(max_coords.z, vertices[i][2]);
    }
    pos = (min_coords + max_coords) / 2.0f; // center will be the average of the local minimum and maximum coords
    for(int i = 0; i < vcnt; ++i) {
        // DATA FLATTENING //
        // pack values together
        int offset = i * 8;
        // positions offset by calculated center in view space
        _data[offset] = vertices[i][0] - pos.x;
        _data[offset + 1] = vertices[i][1] - pos.y;
        _data[offset + 2] = vertices[i][2] - pos.z;
        
        _data[offset + 3] = normals[i][0];
        _data[offset + 4] = normals[i][1];
        _data[offset + 5] = normals[i][2];

        if(uvs.empty()) { // set all coords to 0,0
            _data[offset + 6] = 0.0f;
            _data[offset + 7] = 0.0f;
        } else { // assign uvs
            _data[offset + 6] = uvs[i][0];
            _data[offset + 7] = uvs[i][1];
        }
     }
    // This is fucking awful, there has to be a better way
    // There is! (kinda). We don't have to carry around this block of memory anymore, just load GL values here!
    // TODO could also just insert all of the data as seperate blocks like b4. should integrate custom stride value

    // -- Buffer Data --
    // create object's buffers
    // TODO what is penalty of generating buffers 1by1 vs. mass allocation?
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    // bind new buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    // vertex data is contiguous
    glBufferData(GL_ARRAY_BUFFER, _data.size() * sizeof(float), _data.data(), GL_DYNAMIC_DRAW); // copy vertices, normals, uvs
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, icnt * sizeof(short), triangles.data(), GL_DYNAMIC_DRAW); // copy triangle idxs
    // BUFFER: attrs stored in sequential blocks
    // <pos.x pos.y pos.z | norm.x norm.y norm.z | uv.x uv.y>

    #ifdef DEBUG
        const char* typestr;
        switch(type) {
            case ObjType::TRIANGLE:
                typestr = "TRIANGLE";
                break;
            case ObjType::ELLIPSOID:
                typestr = "ELLIPSOID";
                break;
            default:
                typestr = "UNKNOWN";
                break;
        }
        SDL_Log("Added new Object(type,id,tri_cnt,texture_id,pos,amb,diff,spec,n)\
                \n\t{%s, %d, %d, %d, \
                    \n\t<%.f, %.f, %.f>, <%.f, %.f, %.f>, <%.f, %.f, %.f>, <%.f, %.f, %.f>, %d}",
                typestr, id, icnt, material.texture,
                pos.x, pos.y, pos.z,
                material.ambient[0], material.ambient[1], material.ambient[2],
                material.diffuse[0], material.diffuse[1], material.diffuse[2],
                material.specular[0], material.specular[1], material.specular[2], material.exponent
        );
    #endif
}
// ellipsoid constructor
Object::Object(float x, float y, float z, float a, float b, float c, const std::array<float, 3>& amb, const std::array<float, 3>& diff, const std::array<float, 3>& spec, const int n, const float alpha,  const char* texture) :
    type(ObjType::ELLIPSOID),
    material(amb, diff, spec, n, alpha, texture),
    pos(x, y, z)
{
}
// light constructor
Object::Object(const std::array<float, 3>& amb, const std::array<float, 3>& diff, const std::array<float, 3>& spec, float x, float y, float z) :
    type(ObjType::LIGHT),
    material(amb, diff, spec, 1, 1.0, nullptr), // light json provides no exponent
    pos(x, y, z)
{
}

Object::~Object() {
}

/*

FUNCTION DEFINITIONS

*/

void load_json_files(int cnt, const char* paths[]) {
    for(int i = 0; i < cnt; ++i) {
        load_objects_from_json(paths[i]);
    }
}

void load_objects_from_json(const char *path, bool text) {
    json contents;

    if(text) { // parse path as json string
        contents = json::parse(path);
    } else { // open file "path" and parse
        std::ifstream fp(path);
        contents = json::parse(fp);
    }

    ObjType type = find_type_of_json(contents[0]); // find type using first object found

    #ifdef DEBUG
        const char* typestr;
        switch(type) {
            case TRIANGLE:
                typestr = "Triangle";
                break;
            case ELLIPSOID:
                typestr = "Ellipsoid";
                break;
            case LIGHT:
                typestr = "Light";
                break;
            default:
                typestr = "Unknown";
        }
        
        SDL_Log("JSON %s is type %s", path, typestr);
    #endif

    if(ObjType::TRIANGLE == type) {

        for(const auto& jobj : contents) {
            const auto& vertices = jobj["vertices"].get<std::vector<std::array<float, 3>>>(); // load vertices
            const auto& normals = jobj["normals"].get<std::vector<std::array<float, 3>>>(); // load normals
            const auto& triangles = jobj["triangles"].get<std::vector<std::array<unsigned short, 3>>>(); // load indicesi
            // Material //
            const auto& mat = jobj["material"]; // metrial sub-object
            const auto& amb = mat["ambient"].get<std::array<float, 3>>();
            const auto& diff = mat["diffuse"].get<std::array<float, 3>>();
            const auto& spec = mat["specular"].get<std::array<float, 3>>();
            int n = mat["n"].get<int>();
            float alpha = 1.0f;
            if(mat.contains("alpha"))
                alpha = mat["alpha"].get<float>();

            // empty uv array (only need to define once)
            static const std::vector<std::array<float, 2>> empty_uv;

            if(mat.contains("texture") && mat["texture"].is_string()) { // create object with texture
                // std::string texture(STRINGIFY(TEXTURE_DIR/));
                const std::string& name = mat["texture"].get<std::string>(); // prefix arg with texture directory
                char texture[MAX_PATH];
                sprintf(texture, "%s/%s", STRINGIFY(TEXTURE_DIR), name.c_str());
                // texture and coordinates
                // i cant believe this works lmao
                const auto& uvs = jobj.contains("uvs") ? jobj["uvs"].get<std::vector<std::array<float, 2>>>() : empty_uv; // load uv coordiantes, fill with 0s if none given
                // append new triangle instance with uvs
                objects.emplace_back(vertices, normals, uvs, triangles, amb, diff, spec, n, alpha, texture);
            } else { // create object with no texture
                objects.emplace_back(vertices, normals, empty_uv, triangles, amb, diff, spec, n, alpha, nullptr);
            }
        }
    }
    else if(ObjType::ELLIPSOID == type) {
        for(const auto& jobj : contents) {
            // Dimensions //
            float x = jobj["x"].get<float>();
            float y = jobj["y"].get<float>();
            float z = jobj["z"].get<float>();
            float a = jobj["a"].get<float>(); // "width"
            float b = jobj["b"].get<float>(); // "height"
            float c = jobj["c"].get<float>(); // "depth"
            // Material //
            const auto& amb = jobj["ambient"].get<std::array<float, 3>>();
            const auto& diff = jobj["diffuse"].get<std::array<float, 3>>();
            const auto& spec = jobj["specular"].get<std::array<float, 3>>();
            int n = jobj["n"].get<int>();
            float alpha = 1.0f;
            if(jobj.contains("alpha"))
                alpha = jobj["alpha"].get<float>();
            // append new ellipsoid instance
            objects.emplace_back(x, y, z, a, b, c, amb, diff, spec, n, alpha, nullptr);
            // next_obj_id++; // set next id
        }
    }
    else if(ObjType::LIGHT == type) {
        for(const auto& jobj : contents) {
            // Dimensions //
            float x = jobj["x"].get<float>();
            float y = jobj["y"].get<float>();
            float z = jobj["z"].get<float>();
            // Material //
            const auto& amb = jobj["ambient"].get<std::array<float, 3>>();
            const auto& diff = jobj["diffuse"].get<std::array<float, 3>>();
            const auto& spec = jobj["specular"].get<std::array<float, 3>>();
            // append new light instance
            lights.emplace_back(amb, diff, spec, x, y, z);
        }
    }
    else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "load_json_file Encountered unkown type %d", type);
    }
}

Eye::Eye(const glm::vec3& pos, const glm::vec3& lookat, const glm::vec3& up, float fov) : fov(fov), pos(pos), _forward(glm::normalize(lookat - pos)), _up(up) {
    // (OLD) Gram-Schmidt process
    // _direction = glm::normalize(pos - lookat);
    // _right = glm::normalize(glm::cross(up, _direction));
    // _up = glm::cross(_direction, _right);
    pitch = glm::degrees(asin(_forward.y)); // proj onto X/Z plane -> sqrt(x^2 + z^2)
    yaw = glm::degrees(atan2(_forward.z, _forward.x)); // proj onto X/Z plane

    update();
}

glm::mat4 Eye::view() {
    return glm::lookAt(pos, pos + _forward, _up);
}

glm::mat4 Eye::project(float aspect_ratio) {
    return glm::perspective(glm::radians(fov), aspect_ratio, near_z, far_z);
}

void Eye::mouse(float x, float y) {
    yaw += x;
    pitch -= y;
    if(pitch > 89.0f) // clamp maximum pitch to prevent control issues
        pitch = 89.0f;
    if(pitch < -89.0f) // clamp minimum
        pitch = -89.0f;
    update(); // update camera directions
}

void Eye::update() {
    // glm uses radians natively
    float pitch_r = glm::radians(pitch);
    float yaw_r = glm::radians(yaw);
    // recalculate forward direction
    _forward = glm::normalize(
        glm::vec3(
            glm::cos(yaw_r) * glm::cos(pitch_r),
            glm::sin(pitch_r),
            glm::sin(yaw_r) * glm::cos(pitch_r)
        )
    );
    // right direction = forward x GlobalUp
    _right = glm::normalize(glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    _up = glm::normalize(glm::cross(_right, _forward)); // ensure all vectors are orthogonal
}

void Eye::set_target(const glm::vec3& tpos) {
    _forward = glm::normalize(tpos - pos); // get new forward direction
    // recalculate other directions
    _right = glm::normalize(glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    _up = glm::normalize(glm::cross(_right, _forward)); // ensure all vectors are orthogonal
    // calculate new yaw/pitch (same as constructor)
    pitch = glm::degrees(asin(_forward.y));
    yaw = glm::degrees(atan2(_forward.z, _forward.x));
}

void Eye::move(const glm::vec2& normal, float speed) {
    pos += speed * normal.x * _right; // horizontal movement
    pos += speed * normal.y * _forward; // forward/backward
}
