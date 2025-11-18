// System libs
#include <algorithm>
#include <fstream>
#include <cstdio> // sprintf

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <GLES2/gl2.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
// Local libs
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
// Local includes
#include "defs.hh"
#include "glm/matrix.hpp"
#include "obj.hh"
#include "shader.hh"
#include "texture.hh"
#include "game.hh"

/**
 * SDL Globals
 */
SDL_Window* window;
SDL_GLContext gl_context;
int window_height = DEFAULT_WINDOW_HEIGHT,
    window_width = DEFAULT_WINDOW_WIDTH;
float aspect_ratio = (float)window_width / (float)window_height;
/**
 * End SDL Globals
 */

/**
 * GL Globals
 */
// NOTE Most GL GLobals moved to `shader.hh` and `shader.cpp`
// TODO Finish OO Shader migration
GLuint  object_shader, // used for objects and lighting
        bg_shader; // used for background image
GLuint  bg_vbo, // background quad buffer
        bg_texture; // background texture id
GLuint  plane_vbo, // infinite plane quad buffer
        plane_texture; // inf plane texture
/**
 * End GL Globals
 */

int init() {
    DEBUG_LOG("init() starting SDL");
    /**
     * SDL3 Init
     */
    // Tell SDL to use ES 2.0
    // https://wiki.libsdl.org/SDL3/SDL_GLAttr
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Initialize SDL
    if( !SDL_Init(SDL_INIT_VIDEO) ) {
        // https://wiki.libsdl.org/SDL3/CategoryLog
        // Better than doing std::cout with all the messay conditional imports
        // Also has less performance overhead
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to initialize SDL: %s", SDL_GetError());
        return 1; // fail
    }
    window = SDL_CreateWindow(DEFAULT_WINDOW_TITLE, window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if( !window ) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to initialize window: %s", SDL_GetError());
        return 1; // fail
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if( !gl_context ) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create GL context: %s", SDL_GetError());
        exit(1); // fail
    }
    if(!SDL_GL_MakeCurrent(window, gl_context)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "Failed to lock context to thread");
        return 1; // fail
    }
    SDL_SetWindowRelativeMouseMode(window, true); // mouse reports delta rather than absolute pos
    /**
     * End SDL3 Init
     */
    DEBUG_LOG("init() starting gl");
    /**
     * Compile shaders
     */
    // GLint vert_id, frag_id;
    char* vert_src = nullptr, *frag_src = nullptr;

    // TODO Could combine a lot of this code into create_shader
    // malloc/free blocks common for both shaders

    // -- Background shader --
    {
        const char* vpath = BG_VERT_SHADER; // TODO replace with Shader class
        const char* fpath = BG_FRAG_SHADER;
        
        std::ifstream vert_file(vpath);
        get_src_from_file(vert_file, vert_src);
        std::ifstream frag_file(fpath);
        get_src_from_file(frag_file, frag_src);

        // DEBUG_LOG("VertSrc: %s\nFrag Src: %s", vert_src, frag_src);

        DEBUG_LOG("->background shader compile");
        bg_shader = create_shader_program(vert_src, frag_src);
        // free src blocks
        free(vert_src);
        free(frag_src);
        // check compile status
        if(bg_shader == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create background shader");
            return 1;
        }

        s_background = glGetUniformLocation(bg_shader, "s_texture"); // enable background texture
        u_bg_offset = glGetUniformLocation(bg_shader, "u_offset"); // enable parallax effect
        
        a_bg_pos = glGetAttribLocation(bg_shader, "a_pos"); // enable normal parameter
        a_bg_uv = glGetAttribLocation(bg_shader, "a_uv"); // enable uv parameter
    }

    // -- Object Shader --
    {
        const char* vpath = OBJ_VERT_SHADER; // TODO replace with Shader class
        const char* fpath = OBJ_FRAG_SHADER;
        
        std::ifstream vert_file(vpath);
        get_src_from_file(vert_file, vert_src);
        std::ifstream frag_file(fpath);
        get_src_from_file(frag_file, frag_src);
        
        DEBUG_LOG("->object shader compile");
        object_shader = create_shader_program(vert_src, frag_src); // create object shader
        // free src blocks
        free(vert_src);
        free(frag_src);
        // check compile status
        if(object_shader == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "Failed to create object shader");
            return 1; // fail
        }

        /**
         * Shader Parameters
         * TODO move to Shader class
         */
        // -- Object Shader --
        u_normal = glGetUniformLocation(object_shader, "u_normal");
        u_model_view = glGetUniformLocation(object_shader, "u_model_view");
        u_mvp = glGetUniformLocation(object_shader, "u_mvp");

        u_plane_toggle = glGetUniformLocation(object_shader, "u_plane_toggle");
        u_plane_point = glGetUniformLocation(object_shader, "u_plane_point");
        u_plane_normal = glGetUniformLocation(object_shader, "u_plane_normal");
        u_ivp = glGetUniformLocation(object_shader, "u_ivp");
        u_eye_pos = glGetUniformLocation(object_shader, "u_eye_pos");
        
        u_ambient = glGetUniformLocation(object_shader, "u_mat_amb"); // object ambient param
        u_diffuse = glGetUniformLocation(object_shader, "u_mat_dif"); // object diffuse param
        u_specular = glGetUniformLocation(object_shader, "u_mat_spec"); // object specular param
        u_exponent = glGetUniformLocation(object_shader, "u_mat_n"); // object exp param
        u_alpha = glGetUniformLocation(object_shader, "u_mat_alpha");
        s_object = glGetUniformLocation(object_shader, "s_texture"); // enable object texture

        u_light_amb = glGetUniformLocation(object_shader, "u_light_amb"); // global ambient is calulcated in load()
        u_light_cnt = glGetUniformLocation(object_shader, "u_light_cnt"); // light array parameters are enable in load()

        a_pos = glGetAttribLocation(object_shader, "a_pos"); // enable position parameter
        a_norm = glGetAttribLocation(object_shader, "a_norm"); // enable normal parameter
        a_uv = glGetAttribLocation(object_shader, "a_uv"); // enable uv parameter
    }
    /**
     * End Shader Parameters
     */

    // Other GL Init Things //
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // set clear color to black
    glClearDepthf(1.0f); // clear entire depth buffer
    glEnable(GL_DEPTH_TEST); // enable depth test
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, window_width, window_height); // match viewport to window

    return 0; // success
}

int load(std::vector<const char*>& json_paths, const char* background) {
    // global ambient from all lights
    float amb_0 = 0.0f;
    float amb_1 = 0.0f;
    float amb_2 = 0.0f;
    // define some format strings for getting light parameters
    // only need to define them once during loading
    static const char pos_fmt[] = "u_light_pos[%d]";
    static const char view_fmt[] = "u_light_view_pos[%d]";
    // const char amb_fmt[] = "u_light_amb[%d]";
    static const char dif_fmt[] = "u_light_diff[%d]";
    static const char spec_fmt[] = "u_light_spec[%d]";
    static const char state_fmt[] = "u_light_state[%d]";
    char ustr[32]; // temporary holder for formatted strings

    static const float _bg_quad[] = { // vertex data for background quad
        // X,     Y,     Z,     U,     V
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // v0 (Bottom-Left)
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // v1 (Bottom-Right)
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f, // v2 (Top-Left)
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f  // v3 (Top-Right)
    };
    static const float _plane_quad[] = { // vertex data for infinite plane
        // X,     Y,     Z,     U,     V
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, // v0 (Bottom-Left)
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f, // v1 (Bottom-Right)
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f, // v2 (Top-Left)
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f  // v3 (Top-Right)
    };

    DEBUG_LOG("Loading background...");
    // Background //
    if(background) { // load bg_texture texture
        bg_texture = load_texture(background);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "load() given NULL background! Using fallback " DEFAULT_BACKGROUND);
        bg_texture = load_texture(DEFAULT_BACKGROUND);
    }
    glUseProgram(bg_shader); // use BG shader parameters
    glGenBuffers(1, &bg_vbo); // bg_texture quad buffer
    glBindBuffer(GL_ARRAY_BUFFER, bg_vbo); // bind bg buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(_bg_quad), _bg_quad, GL_STATIC_DRAW); // bg vertices will never change
    glUniform1i(s_background, 0); // Texture Unit 0 for Background TODO move to Shader class

    DEBUG_LOG("Loading JSON...");
    // Load JSON //
    glUseProgram(object_shader); // other data goes to Object Shader TODO move to object loader or shader?
    load_json_files(json_paths.size(), json_paths.data()); // defined under `obj.cpp`
    glUniform1i(s_object, 1); // Texture Unit 1 for Objects TODO move to Shader class

    DEBUG_LOG("Loading plane...");
    plane_texture = load_texture(PLANE_TEXTURE);
    glGenBuffers(1, &plane_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_plane_quad), _plane_quad, GL_STATIC_DRAW); // plane vertices will never change

    DEBUG_LOG("Loading lights...");

    // Load Lights //
    // TODO Move to object constructor
    {
        int i = 0;
        for(const auto& light : lights) {
            // For each light object, create associated
            //   position, material and state parameters
            //   for the shader
            sprintf(ustr, pos_fmt, i);
            u_light_pos[i] = glGetUniformLocation(object_shader, ustr);
            sprintf(ustr, view_fmt, i);
            u_light_view_pos[i] = glGetUniformLocation(object_shader, ustr);
            // set light material
            // sprintf(ustr, amb_fmt, i);
            // u_light_amb[i] = glGetUniformLocation(object_shader, ustr);
            sprintf(ustr, dif_fmt, i);
            u_light_diff[i] = glGetUniformLocation(object_shader, ustr);
            sprintf(ustr, spec_fmt, i);
            u_light_spec[i] = glGetUniformLocation(object_shader, ustr);
            // set light count
            sprintf(ustr, state_fmt, i);
            u_light_state[i] = glGetUniformLocation(object_shader, ustr);

            glUniform3f(u_light_pos[i], light.pos.x, light.pos.y, light.pos.z); // light[i] positions
            // glUniform3f(u_light_amb[i], light.material.ambient[0], light.material.ambient[1], light.material.ambient[2]); // light[i] ambient
            glUniform3f(u_light_diff[i], light.material.diffuse[0], light.material.diffuse[1], light.material.diffuse[2]); // light[i] diffuse
            glUniform3f(u_light_spec[i], light.material.specular[0], light.material.specular[1], light.material.specular[2]); // light[i] specular
            glUniform1i(u_light_state[i], GL_TRUE); // light defaults to on

            #ifdef DEBUG
                SDL_Log("Added new Light(pos,amb,diff,spec)\
                        \n\t{<%.f, %.f, %.f>, <%.f, %.f, %.f>, <%.f, %.f, %.f>, <%.f, %.f, %.f>}",
                        light.pos.x, light.pos.y, light.pos.z,
                        light.material.ambient[0], light.material.ambient[1], light.material.ambient[2],
                        light.material.diffuse[0], light.material.diffuse[1], light.material.diffuse[2],
                        light.material.specular[0], light.material.specular[1], light.material.specular[2]
                );
            #endif

            // light_cnt += objs_size;
            amb_0 += light.material.ambient[0];
            amb_1 += light.material.ambient[1];
            amb_2 += light.material.ambient[2];
            i++; // next light uniform index
        }
        DEBUG_LOG("Got %d lights, global ambient <%f, %f, %f>",
                    i, amb_0 / static_cast<float>(lights.size()), amb_1 / static_cast<float>(lights.size()), amb_2 / static_cast<float>(lights.size()));

        glUniform1i(u_light_cnt, i); // set total number of lights
        glUniform3f(u_light_amb, amb_0 / static_cast<float>(lights.size()), amb_1 / static_cast<float>(lights.size()), amb_2 / static_cast<float>(lights.size()));
    }

    // Object GL processing moved to `obj.cpp`->load_objects_from_json()

    return 0; // success
};

void destroy() {
    // Destroy GL //
    // NOTE buffer cleanup moved to `obj.cpp`
    glDeleteBuffers(1, &bg_vbo); // clear bg buffer
    free_textures(); // from `texture.hh`
    glDeleteProgram(object_shader); // delete shader program
    // Destroy SDL //
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/**
 * Program State Values
 *
 * TODO Move to mainloop or `update.cpp`
 */
bool demo = false;
bool running = false;
bool depth_sort = false; // Painter's algorithm
bool snap_to_selected = false; // Move camera view on selection change
bool rotate_selected = true; // rotate selected object
bool painter_sort = true;
Eye camera(EYE_POS, EYE_TARGET, EYE_UP); // camera object
/**
 * End State Values
 */

int main(int argc, const char* argv[]) {
    Uint64 prev = SDL_GetTicks(); // holds ms from last frame
                                    // defining here to measure load time
    /**
     * Arg parse
     */
    const char* vert_override = nullptr,
              * frag_override = nullptr,
              * background_override = nullptr;
    std::vector<const char*> json_paths;

    for(int i = 1; i < argc; ++i) {
        // custom width arg
        if(strcmp(ARGIFY(width), argv[i]) == 0) {
            i++; // skip token
            window_width = atoi(argv[i]); // next arg should be width value
        }
        // custom height arg
        else if(strcmp(ARGIFY(height), argv[i]) == 0) {
            i++; // skip token
            window_width = atoi(argv[i]); // next arg should be height value
        }
        else if(strcmp(ARGIFY(vert-shader), argv[i]) == 0) {
            i++; // skip token
            vert_override = argv[i]; // next arg should be shader path
        }
        else if(strcmp(ARGIFY(frag-shader), argv[i]) == 0) {
            i++; // skip token
            frag_override = argv[i]; // next arg should be shader path
        }
        else if(strcmp(ARGIFY(background), argv[i]) == 0) {
            i++; // skip token
            background_override = argv[i]; // next arg should be background path
        }
        // Toggle Args
        else if(strcmp(ARGIFY(snap), argv[i]) == 0) {
            snap_to_selected = true;
        }
        else if(strcmp(ARGIFY(rotate), argv[i]) == 0) {
            rotate_selected = true;
        }
        else if(strcmp(ARGIFY(demo), argv[i]) == 0) {
            demo = true;
        }
        else { // add to json path //
            json_paths.push_back(argv[i]); // store json path and inc
        }
    }
    // report overrides
    // NOTE: shader overrides not used
    if(vert_override)
        SDL_Log("Got vertex shader override: %s", vert_override);
    if(frag_override)
        SDL_Log("Got fragment shader override: %s", frag_override);
    if(frag_override)
        SDL_Log("Got background override: %s", background_override);
    /**
     * End Arg parse
     */
    
    // Initialize SDL/OpenGL/shaders
    if(init()) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Initialization failed!");
        destroy(); // clean up before exiting
        exit(1);
    }

    #ifdef DEBUG
        SDL_Log("->load()");
    #endif
    
    // Load objects
    if(load(json_paths, background_override)) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Loading failed!");
        destroy(); // clean up before exiting
        exit(1);
    }

    { // tmp will be popped from stack outside this block
        Uint64 tmp = SDL_GetTicks();
        SDL_Log("Initalized! Took %zu ms", tmp - prev); // report loading time
        prev = tmp;
        running = true;
    }

    /**
     * Mainloop
     */
    SDL_Event e; // Event holder
    float shift_bg_x = 0.0f,
        shift_bg_y = 0.0f;
    const float shift_bg_scale = 0.005f;
    while(running) {
        // -- Delta Time --
        Uint64 now = SDL_GetTicks(); // get current ms
        Uint64 delta = now - prev; // get time btwn frames
        float delta_f = static_cast<float>(delta) / 1000.0f;
        prev = now; // will compare against this time next frame
        
        /**
         * Input
         */
        float mouse_x = 0.0f, // mouse x delta
              mouse_y = 0.0f; // mouse y delta
        glm::vec2 direction(0.0f);
        while(SDL_PollEvent(&e)) {
            
            if(e.type == SDL_EVENT_QUIT) { // if window X button was clicked
                running = false;
                break; // no need to process logic or render
                
            } if(e.type == SDL_EVENT_WINDOW_RESIZED) { // if resized by mouse
                SDL_GetWindowSize(window, &window_width, &window_height); // get new window size
                aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height); // recaculate aspect ratio
                glViewport(0, 0, window_width, window_height); // update gl viewport
                
            } if(e.type == SDL_EVENT_MOUSE_MOTION) {
                mouse_x = static_cast<float>(e.motion.xrel) * 0.1f; // scale down raw input
                mouse_y = static_cast<float>(e.motion.yrel) * 0.1f;

            } if(e.type == SDL_EVENT_KEY_DOWN) { // if keyboard was hit while focused on window
                if(KEY_QUIT == e.key.key) {
                    running = false; // equivalent to EVENT_QUIT
                    break;
                }
                // Movement keys
                if(MOVE_FORWARD == e.key.key) {
                    direction.y = 1.0f;
                }
                if(MOVE_BACKWARD == e.key.key) {
                    direction.y = -1.0f;
                }
                if(MOVE_LEFT == e.key.key) {
                    direction.x = -1.0f;
                }
                if(MOVE_RIGHT == e.key.key) {
                    direction.x = 1.0f;
                }
                // Other stuff
                if(TOGGLE_ROTATE == e.key.key) {
                    rotate_selected = !rotate_selected;
                    SDL_Log("Toggled Rotation: %d", rotate_selected);
                }
                if(TOGGLE_SNAP == e.key.key) {
                    snap_to_selected = !snap_to_selected;
                    SDL_Log("Toggled Object Snap: %d", snap_to_selected);
                }
            }
        }
        /**
         * End Input
         */

        /**
         * Updates
         */
        camera.mouse(mouse_x, mouse_y);
        if(glm::length(direction) > FLOAT_THRESH) {
            direction = glm::normalize(direction);
            float cpeed = 3.0f * delta_f;
            camera.move(direction, cpeed);
        }

        update(delta_f); // NOTE Game logic, defined in `game.cpp`

        glm::mat4 proj = camera.project(aspect_ratio); // get camera projetion matrix
        glm::mat4 view = camera.view(); // get camera lookat matrix
        glm::mat4 ivp = glm::inverse(proj * view);
        
        shift_bg_x += mouse_x * shift_bg_scale;
        shift_bg_y += mouse_y * shift_bg_scale;

        // Painter's sort (OLD)
        if(painter_sort) {
            // int id_selected = objects[selected].id;
            std::sort(objects.begin(), objects.end(),
                [](const Object &A, const Object &B) {
                    float dist_a = std::pow(A.pos.x - camera.pos.x, 2) + 
                            std::pow(A.pos.y - camera.pos.y, 2) + 
                            std::pow(A.pos.z - camera.pos.z, 2);
                    // Calculate squared distance for object B
                    float dist_b = std::pow(B.pos.x - camera.pos.x, 2) + 
                            std::pow(B.pos.y - camera.pos.y, 2) + 
                            std::pow(B.pos.z - camera.pos.z, 2);
                    return dist_a > dist_b;
                }
            );
            // update selection indexes
            // int i = 0;
            // for(const auto& obj : objects) {
            //     if(id_selected == obj.id) {
            //         selected = i;
            //         break;
            //     }
            //     i++;
            // }
        }
        /**
         * End Updates
         */

        /**
         * Render
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear GL buffer

        // -- Render Background --
        {
            USE_BG_SHADER; // macro from `shader.hh`
            glBindTexture(GL_TEXTURE_2D, bg_texture);
            glUniform1i(s_background, 0);
            glUniform2f(u_bg_offset, shift_bg_x, -shift_bg_y);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // dont need to use element array
        }
        // -- Render Objects --
        {
            USE_OBJ_SHADER;

            // Update light view positions
            int i = 0;
            for(const auto& light : lights) { // Calculate once per-frame rather than per-pixel in the frag shader
                glm::vec4 light_view = view * glm::vec4(light.pos, 1.0f);
                glUniform3f(u_light_view_pos[i], light_view.x, light_view.y, light_view.z);
                i++; // vector.size() is not int, so have to track my own idx
            }
            glUniform1i(u_plane_toggle, 0); // disable plane-drawing
        } // want to be able to use variable 'i' after this
        
        
        // iterate through objects
        for(auto& obj : objects) {
            // Use object's buffers
            glBindBuffer(GL_ARRAY_BUFFER, obj.vbo); // bind vertex buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ebo); // bind index buffer
            // set object's material
            glUniform3f(u_ambient, obj.material.ambient[0], obj.material.ambient[1], obj.material.ambient[2]); // set ambient
            glUniform3f(u_diffuse, obj.material.diffuse[0], obj.material.diffuse[1], obj.material.diffuse[2]); // set diffuse
            glUniform3f(u_specular, obj.material.specular[0], obj.material.specular[1], obj.material.specular[2]); // set specular
            glUniform1f(u_exponent, obj.material.exponent); // set shinniness exponent
            glUniform1f(u_alpha, obj.material.alpha);
            // bind object texture
            glBindTexture(GL_TEXTURE_2D, obj.material.texture);
            glUniform1i(s_object, 0);
            // use GLM's built in translation matrix builder
            glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.pos) * obj.rotation * obj.scale; // calculate object's model matrix
            glm::mat4 model_view = view * model;
            glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model_view)));
            glm::mat4 mvp = proj * model_view;
            // update shader uniforms
            glUniformMatrix4fv(u_model_view, 1, GL_FALSE, glm::value_ptr(model_view)); // pass object model-view matrix to shader
            glUniformMatrix3fv(u_normal, 1, GL_FALSE, glm::value_ptr(normal)); // pass view normal to shader
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp)); // pass object model-view matrix to shader
            // Update attribute pointers to current object buffer
            // pos.x pos.y pos.z | norm.x norm.y norm.z | uv.x uv.y
            glVertexAttribPointer(a_pos, 3, GL_FLOAT, GL_FALSE, BUFFER_STRIDE, static_cast<void*>(0)); // position location in buffer
            glVertexAttribPointer(a_norm, 3, GL_FLOAT, GL_FALSE, BUFFER_STRIDE, (void*)(sizeof(float) * 3)); // normal location in buffer
            glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, BUFFER_STRIDE, (void*)(sizeof(float) * 6)); // uv location in buffer

            glDrawElements(GL_TRIANGLES, obj.icnt, GL_UNSIGNED_SHORT, (void*)0); // draw object
        }

        {
            // -- Infinite Plane --
            static glm::vec3 plane_point = DEFAULT_PLANE_POINT; // TODO make configurable
            static glm::vec3 plane_norm = DEFAULT_PLANE_NORM;
            static glm::vec3 plane_amb = PLANE_AMB;
            static glm::vec3 plane_diff = PLANE_DIFF;
            static glm::vec3 plane_spec = PLANE_SPEC;
            static float plane_n = PLANE_EXP;
            static glm::mat4 plane_model(1.0f); // plane is always the same, so use identity
            glm::mat4 model_view = view * plane_model;
            glm::mat4 mvp = proj * model_view;

            glUniform1i(s_object, 0);
            glUniform1i(u_plane_toggle, 1); // enable plane-drawing
            // plane-specific uniforms
            glUniformMatrix3fv(u_plane_point, 1, GL_FALSE, glm::value_ptr(plane_point)); // arbitrary plane anchor
            glUniformMatrix3fv(u_plane_normal, 1, GL_FALSE, glm::value_ptr(plane_norm)); // plane surface normal
            glUniform3f(u_eye_pos, camera.pos.x, camera.pos.y, camera.pos.z); // update shader's camera pos value
            glUniformMatrix4fv(u_ivp, 1, GL_FALSE, glm::value_ptr(ivp)); // used to calculate ray-world position from clip-space
            glUniformMatrix4fv(u_model_view, 1, GL_FALSE, glm::value_ptr(model_view));
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            // Set plane materials
            glUniform3fv(u_ambient, 1, glm::value_ptr(plane_amb)); // set ambient
            glUniform3fv(u_diffuse, 1, glm::value_ptr(plane_diff)); // set diffuse
            glUniform3fv(u_specular, 1, glm::value_ptr(plane_spec)); // set specular
            glUniform1f(u_exponent, plane_n); // set shinniness exponent
            glUniform1f(u_alpha, 1.0f); // set transparency
            // Bind buffers
            glBindTexture(GL_TEXTURE_2D, plane_texture);
            glUniform1i(s_object, 0);
            glBindBuffer(GL_ARRAY_BUFFER, plane_vbo);
            // Set attribute pts
            glVertexAttribPointer(a_pos, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, static_cast<void*>(0)); // position location in buffer
            glVertexAttribPointer(a_norm, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, static_cast<void*>(0)); // normals are unused, so a_norm = a_pos
            glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3)); // uv location in buffer

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // dont need to use element array
        }
        
        SDL_GL_SwapWindow(window); // swap to second buffer for double-buffering
        /**
         * End Render
         */
    }
    /**
     * End Mainloop
     */

    SDL_Log("Exiting program...");
    destroy(); // clean
    return 0; // success
}
