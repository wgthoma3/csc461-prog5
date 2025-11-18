// System libs
#include <GLES2/gl2.h>
#include <SDL3/SDL_log.h>
#include <vector>
// Local libs
// Local includes
#include "shader.hh"

/**
 * Begin uniforms
 * TODO This is awful. NEEDS to gain class conciousness
 */
// Shader Uniforms
GLint   u_ambient,
        u_diffuse,
        u_specular,
        u_exponent,
        u_alpha,
        u_normal, // view normal
        // u_proj, // view projection matrix
        // u_view,
        u_model_view, // model transformation matrix
        u_mvp; // model-view projection matrix
        // Infinite plane
GLint   u_ivp, // inverse projection-view matrix
        u_plane_toggle, // toggle floor drawing in shader
        u_plane_point, // any point on the plane, used as an anchor
        u_plane_normal, // normal of the plane
        u_eye_pos; // camera position in world coordinates
        // Lights
GLint   u_light_cnt, // total number of lights loaded
        u_light_pos[MAX_LIGHTS],
        u_light_view_pos[MAX_LIGHTS], // light position in view space
        u_light_amb,
        u_light_diff[MAX_LIGHTS],
        u_light_spec[MAX_LIGHTS],
        u_light_state[MAX_LIGHTS]; // whether light is on/off
GLint u_bg_offset;
// Texture Uniforms
GLint   s_background; // background texture
GLint   s_object; // object texture
// Vertex Attributes
GLint   a_pos, // vertex position
        a_norm, // vertex normal
        a_uv; // uv coordinate
// BG Attributes //
GLint   a_bg_pos, // bg position attr
        a_bg_uv; // bg uv coord attr
/**
 * End Uniforms
 */

void print_gl_status(GLuint id) {
    GLint max_len = 0;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &max_len); // set length of log buffer to as-needed

    if(max_len > 0) {
        std::vector<char> log(max_len); // reserve log text region
        glGetProgramInfoLog(id, max_len, nullptr, log.data()); // get most recent log contents
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "GL Log: %s", log.data()); // display compile error
    } else
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "No GL log availible");
}

GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader_id = glCreateShader(type); // create GLSL shader
    if(shader_id == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "compiler_shader Failed to create shader object");
        return 0;
    }

    // read GLSL source code from file
    if(src != nullptr) {
        glShaderSource(shader_id, 1, &src, nullptr); // load source into OpenGL
        glCompileShader(shader_id); // compile shader
        GLint status = 0;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status); // get compilation status

        if(status == GL_FALSE) { // if GL_FALSE, compile failed
            GLint max_len = 0;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_len); // set length of log buffer to as-needed
            std::vector<char> log(max_len); // reserve log text region
            glGetShaderInfoLog(shader_id, max_len, &max_len, log.data()); // get most recent log contents
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "compiler_shader Compile error: %s", log.data());
            glDeleteShader(shader_id); // delete shader remove shader from memory | gets done after compile_shader is called in init
            shader_id = 0; // indicates failed shader creation
        }
    } else { // unknown program state
        shader_id = 0; // indicates failed shader creation
    }
    return shader_id; // return shader descriptor
}

GLint create_shader_program(const char* vert_src, const char* frag_src) {
    GLint shader = glCreateProgram(); // create shader program

    #ifdef DEBUG
        SDL_Log("Compiling vertex shader...");
    #endif
    
    // compile vertex shader
    int vert_id = compile_shader(GL_VERTEX_SHADER, vert_src);
    if(vert_id == 0) { // compile failed if id==0
        glDeleteProgram(shader);
        return 0; // fail
    }

    #ifdef DEBUG
        SDL_Log("Compiling fragment shader...");
    #endif
    
    int frag_id = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if(frag_id == 0) { // compile failed if id == 0
        glDeleteProgram(shader);
        glDeleteShader(vert_id);
        return 0; // fail
    }

    #ifdef DEBUG
        SDL_Log("Linking shaders...");
    #endif

    // attach shaders to program //
    glAttachShader(shader, vert_id); // attach vertex shader
    glAttachShader(shader, frag_id); // attach fragment shader
    glLinkProgram(shader); // link shader program with current GL context
    GLint lstatus;
    glGetProgramiv(shader, GL_LINK_STATUS, &lstatus); // get linker status
    if(lstatus == GL_FALSE) { // linking failed
        print_gl_status(shader); // print linker error
        // clean up
        glDeleteProgram(shader);
        glDeleteShader(vert_id);
        glDeleteShader(frag_id);
        return 0; // fail
    }
    // shaders are all loaded before runtime //
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);

    return shader;
}
