#pragma once

#include <SDL3/SDL_keycode.h>

// Window defaults //
#define DEFAULT_WINDOW_TITLE "Program 5"
#define DEFAULT_WINDOW_WIDTH 512
#define DEFAULT_WINDOW_HEIGHT 512

// Input Binds //
#define MOVE_FORWARD SDLK_W
#define MOVE_BACKWARD SDLK_S
#define MOVE_LEFT SDLK_A
#define MOVE_RIGHT SDLK_D

#define KEY_QUIT SDLK_ESCAPE

#define TOGGLE_ROTATE SDLK_1
#define TOGGLE_SNAP SDLK_2

// Limits //
#define MAX_OBJS 32
#define MAX_LIGHTS 16
#define MAX_TEXTURES 512
#define MAX_PATH 32
// number of verticle vertices for ellipsoid
#define ELLIP_STACKS 20
// number of horizontal vertices for ellipsoid
#define ELLIP_SLICES 30

// Misc

// Expands input into string type
#define _STRINGIFY(X) #X
#define STRINGIFY(X) _STRINGIFY(X)
// "-name"
#define ARGIFY(NAME) STRINGIFY( -NAME  )

#define TEXTURE_DIR textures
#define TEXTURE(NAME) STRINGIFY(TEXTURE_DIR/NAME)

#define OBJ_VERT_SHADER "shader/vert.glsl"
#define OBJ_FRAG_SHADER "shader/frag.glsl"
#define BG_VERT_SHADER "shader/bg_vert.glsl"
#define BG_FRAG_SHADER "shader/bg_frag.glsl"
#define DEFAULT_BACKGROUND "textures/sky.jpg"

#ifdef DEBUG // saves me hella time and line numbers
    #define DEBUG_LOG(...) \
        SDL_Log(__VA_ARGS__)
#else // define empty version that gets ignored
    #define DEBUG_LOG(...) \
        ((void)0)
#endif

// starting values for camera
#define EYE_POS {0.5f, 0.5f, -0.5f}
#define EYE_TARGET {0.5f, 0.5f, 0.5f}
#define EYE_UP {0.0f, 1.0f, 0.0f}

#define EYE_DEFAULT_FOV 90.0f
#define EYE_DEFAULT_PITCH 0.0f
#define EYE_DEFAULT_YAW -90.0f

#define FLOAT_THRESH 0.0001f

#define DEFAULT_PLANE_POINT {0.0f, -1.0f, 0.0f}
#define DEFAULT_PLANE_NORM {0.0f, 1.0f, 0.0f}
#define PLANE_AMB {0.05f, 0.05f, 0.05f}
#define PLANE_DIFF {0.2f, 0.2f, 0.2f}
#define PLANE_SPEC {1.0, 1.0, 1.0}
#define PLANE_EXP 200.0f
#define PLANE_TEXTURE "textures/rocktile.jpg"
