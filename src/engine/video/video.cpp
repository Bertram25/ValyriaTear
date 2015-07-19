///////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2004-2011 by The Allacrost Project
//            Copyright (C) 2012-2015 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
///////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    video.cpp
*** \author  Raj Sharma, roos@allacrost.org
*** \author  Yohann Ferreira, yohann ferreira orange fr
*** \brief   Source file for video engine interface.
*** ***************************************************************************/

#include "utils/utils_pch.h"
#include "engine/video/video.h"

#include "engine/mode_manager.h"
#include "engine/script/script_read.h"
#include "engine/system.h"
#include "engine/video/gl/gl_particle_system.h"
#include "engine/video/gl/gl_shader.h"
#include "engine/video/gl/gl_shader_definitions.h"
#include "engine/video/gl/gl_shader_program.h"
#include "engine/video/gl/gl_shader_programs.h"
#include "engine/video/gl/gl_shaders.h"
#include "engine/video/gl/gl_sprite.h"

#include "utils/utils_strings.h"

using namespace vt_utils;
using namespace vt_video::private_video;

namespace vt_video
{

VideoEngine *VideoManager = nullptr;
bool VIDEO_DEBUG = false;

//-----------------------------------------------------------------------------
// Static variable for the Color class
//-----------------------------------------------------------------------------

const Color Color::clear(0.0f, 0.0f, 0.0f, 0.0f);
const Color Color::white(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::gray(0.5f, 0.5f, 0.5f, 1.0f);
const Color Color::black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::orange(1.0f, 0.4f, 0.0f, 1.0f);
const Color Color::yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::aqua(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::violet(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::brown(0.6f, 0.3f, 0.1f, 1.0f);

void RotatePoint(float &x, float &y, float angle)
{
    float original_x = x;
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);

    x = x * cos_angle - y * sin_angle;
    y = y * cos_angle + original_x * sin_angle;
}

//-----------------------------------------------------------------------------
// VideoEngine class
//-----------------------------------------------------------------------------

VideoEngine::VideoEngine():
    _sdl_window(nullptr),
    _fps_display(false),
    _fps_sum(0),
    _current_sample(0),
    _number_samples(0),
    _FPS_textimage(nullptr),
    _gl_error_code(GL_NO_ERROR),
    _gl_blend_is_active(false),
    _gl_texture_2d_is_active(false),
    _gl_stencil_test_is_active(false),
    _gl_scissor_test_is_active(false),
    _viewport_x_offset(0),
    _viewport_y_offset(0),
    _viewport_width(0),
    _viewport_height(0),
    _screen_width(0),
    _screen_height(0),
    _fullscreen(false),
    _x_cursor(0),
    _y_cursor(0),
    _debug_info(false),
    _x_shake(0),
    _y_shake(0),
    _brightness_value(1.0f),
    _temp_fullscreen(false),
    _temp_width(0),
    _temp_height(0),
    _vsync_mode(0),
    _game_update_mode(false),
    _sprite(nullptr),
    _particle_system(nullptr),
    _initialized(false)
{
    _current_context.blend = 0;
    _current_context.x_align = -1;
    _current_context.y_align = -1;
    _current_context.x_flip = 0;
    _current_context.y_flip = 0;
    _current_context.coordinate_system = CoordSys(0.0f, VIDEO_STANDARD_RES_WIDTH,
                                         0.0f, VIDEO_STANDARD_RES_HEIGHT);
    _current_context.viewport = ScreenRect(0, 0, VIDEO_STANDARD_RES_WIDTH, VIDEO_STANDARD_RES_HEIGHT);
    _current_context.scissor_rectangle = ScreenRect(0, 0, VIDEO_STANDARD_RES_WIDTH,
                                         VIDEO_STANDARD_RES_HEIGHT);
    _current_context.scissoring_enabled = false;

    _transform_stack.push(gl::Transform());

    for(uint32 sample = 0; sample < FPS_SAMPLES; sample++)
        _fps_samples[sample] = 0;
}

void VideoEngine::_UpdateFPS()
{
    if(!_fps_display)
        return;

    // We only create the text image when needed, to permit getting the text style correctly.
    if (!_FPS_textimage)
        _FPS_textimage = new TextImage("FPS: ", TextStyle("text20", Color::white));

    //! \brief Maximum milliseconds that the current frame time and our averaged frame time must vary
    //! before we begin trying to catch up
    const uint32 MAX_FTIME_DIFF = 5;

    //! \brief The number of samples to take if we need to play catchup with the current FPS
    const uint32 FPS_CATCHUP = 20;

    uint32 frame_time = vt_system::SystemManager->GetUpdateTime();

    // Calculate the FPS for the current frame
    uint32 current_fps = 1000;
    if(frame_time)
        current_fps /= frame_time;

    // The number of times to insert the current FPS sample into the fps_samples array
    uint32 number_insertions;

    if(_number_samples == 0) {
        // If the FPS display is uninitialized, set the entire FPS array to the current FPS
        _number_samples = FPS_SAMPLES;
        number_insertions = FPS_SAMPLES;
    } else if(current_fps >= 500) {
        // If the game is going at 500 fps or faster, 1 insertion is enough
        number_insertions = 1;
    } else {
        // Find if there's a discrepancy between the current frame time and the averaged one.
        // If there's a large difference, add extra samples so the FPS display "catches up" more quickly.
        float avg_frame_time = 1000.0f * FPS_SAMPLES / _fps_sum;
        int32 time_difference = static_cast<int32>(avg_frame_time) - static_cast<int32>(frame_time);

        if(time_difference < 0)
            time_difference = -time_difference;

        if(time_difference <= static_cast<int32>(MAX_FTIME_DIFF))
            number_insertions = 1;
        else
            number_insertions = FPS_CATCHUP; // Take more samples to catch up to the current FPS
    }

    // Insert the current_fps samples into the fps_samples array for the number of times specified
    for(uint32 j = 0; j < number_insertions; j++) {
        _fps_sum -= _fps_samples[_current_sample];
        _fps_sum += current_fps;
        _fps_samples[_current_sample] = current_fps;
        _current_sample = (_current_sample + 1) % FPS_SAMPLES;
    }

    uint32 avg_fps = _fps_sum / FPS_SAMPLES;

    // The text to display to the screen
    _FPS_textimage->SetText("FPS: " + NumberToString(avg_fps));
}

void VideoEngine::_DrawFPS()
{
    if(!_fps_display || !_FPS_textimage)
        return;

    PushState();
    SetStandardCoordSys();
    SetDrawFlags(VIDEO_X_LEFT, VIDEO_Y_BOTTOM, VIDEO_X_NOFLIP, VIDEO_Y_NOFLIP, VIDEO_BLEND, 0);
    Move(930.0f, 40.0f); // Upper right hand corner of the screen
    _FPS_textimage->Draw();
    PopState();
}

VideoEngine::~VideoEngine()
{
    // Clean up the sprite.
    if (_sprite) {
        delete _sprite;
        _sprite = nullptr;
    }

    // Clean up the particle system.
    if (_particle_system) {
        delete _particle_system;
        _particle_system = nullptr;
    }

    // Clean up the shaders and shader programs.
    glUseProgram(0);

    for (std::map<gl::shader_programs::ShaderPrograms, gl::ShaderProgram*>::iterator i = _programs.begin(); i != _programs.end(); ++i)
    {
        delete i->second;
        i->second = nullptr;
    }
    _programs.clear();

    for (std::map<gl::shaders::Shaders, gl::Shader*>::iterator i = _shaders.begin(); i != _shaders.end(); ++i)
    {
        delete i->second;
        i->second = nullptr;
    }
    _shaders.clear();

    TextManager->SingletonDestroy();

    _rectangle_image.Clear();
    delete _FPS_textimage;

    TextureManager->SingletonDestroy();
}

bool VideoEngine::FinalizeInitialization()
{
    // Load GLEW. Unneeded on OSX.
#ifndef __APPLE__
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        PRINT_ERROR << "Unable to initialize GLEW." << std::endl;
        return false;
    }
#endif

    // Create the sprite.
    _sprite = new gl::Sprite();

    // Create the particle system.
    _particle_system = new gl::ParticleSystem();

    //
    // Create the programmable pipeline.
    //

    // Create the shaders.
    gl::Shader* default_vertex                              = new gl::Shader(GL_VERTEX_SHADER, gl::shader_definitions::DEFAULT_VERTEX);
    gl::Shader* solid_color_fragment                        = new gl::Shader(GL_FRAGMENT_SHADER, gl::shader_definitions::SOLID_FRAGMENT);
    gl::Shader* solid_color_grayscale_fragment              = new gl::Shader(GL_FRAGMENT_SHADER, gl::shader_definitions::SOLID_GRAYSCALE_FRAGMENT);
    gl::Shader* sprite_fragment                             = new gl::Shader(GL_FRAGMENT_SHADER, gl::shader_definitions::SPRITE_FRAGMENT);
    gl::Shader* sprite_grayscale_fragment                   = new gl::Shader(GL_FRAGMENT_SHADER, gl::shader_definitions::SPRITE_GRAYSCALE_FRAGMENT);

    // Store the shaders.
    _shaders[gl::shaders::VertexDefault] = default_vertex;
    _shaders[gl::shaders::FragmentSolid] = solid_color_fragment;
    _shaders[gl::shaders::FragmentSolidGrayscale] = solid_color_grayscale_fragment;
    _shaders[gl::shaders::FragmentSprite] = sprite_fragment;
    _shaders[gl::shaders::FragmentSpriteGrayscale] = sprite_grayscale_fragment;

    //
    // Create the shader programs.
    //

    std::vector<std::string> attributes;
    attributes.push_back("in_Vertex");
    attributes.push_back("in_TexCoords");
    attributes.push_back("in_Color");

    gl::ShaderProgram* solid_program = new gl::ShaderProgram(_shaders[gl::shaders::VertexDefault],
                                                             _shaders[gl::shaders::FragmentSolid],
                                                             attributes);

    gl::ShaderProgram* solid_grayscale_program = new gl::ShaderProgram(_shaders[gl::shaders::VertexDefault],
                                                                       _shaders[gl::shaders::FragmentSolidGrayscale],
                                                                       attributes);

    gl::ShaderProgram* sprite_program = new gl::ShaderProgram(_shaders[gl::shaders::VertexDefault],
                                                              _shaders[gl::shaders::FragmentSprite],
                                                              attributes);

    gl::ShaderProgram* sprite_grayscale_program = new gl::ShaderProgram(_shaders[gl::shaders::VertexDefault],
                                                                        _shaders[gl::shaders::FragmentSpriteGrayscale],
                                                                        attributes);

    //
    // Store the shader programs.
    //

    _programs[gl::shader_programs::Solid] = solid_program;
    _programs[gl::shader_programs::SolidGrayscale] = solid_grayscale_program;
    _programs[gl::shader_programs::Sprite] = sprite_program;
    _programs[gl::shader_programs::SpriteGrayscale] = sprite_grayscale_program;

    // Create instances of the various sub-systems
    TextureManager = TextureController::SingletonCreate();
    TextManager = TextSupervisor::SingletonCreate();

    // Initialize all sub-systems.
    if (TextureManager->SingletonInitialize() == false) {
        PRINT_ERROR << "could not initialize texture manager" << std::endl;
        return false;
    }

    if (TextManager->SingletonInitialize() == false) {
        PRINT_ERROR << "could not initialize text manager" << std::endl;
        return false;
    }

    // Prepare the screen for rendering.
    Clear();

    // Empty image used to draw colored rectangles.
    if (_rectangle_image.Load("") == false) {
        PRINT_ERROR << "_rectangle_image could not be created" << std::endl;
        return false;
    }

    _initialized = true;
    return true;
}

//-----------------------------------------------------------------------------
// VideoEngine class - General methods
//-----------------------------------------------------------------------------

void VideoEngine::SetDrawFlags(int32 first_flag, ...)
{
    int32 flag = first_flag;
    va_list args;

    va_start(args, first_flag);
    while(flag != 0) {
        switch(flag) {
        case VIDEO_X_LEFT:
            _current_context.x_align = -1;
            break;
        case VIDEO_X_CENTER:
            _current_context.x_align = 0;
            break;
        case VIDEO_X_RIGHT:
            _current_context.x_align = 1;
            break;

        case VIDEO_Y_TOP:
            _current_context.y_align = 1;
            break;
        case VIDEO_Y_CENTER:
            _current_context.y_align = 0;
            break;
        case VIDEO_Y_BOTTOM:
            _current_context.y_align = -1;
            break;

        case VIDEO_X_NOFLIP:
            _current_context.x_flip = 0;
            break;
        case VIDEO_X_FLIP:
            _current_context.x_flip = 1;
            break;

        case VIDEO_Y_NOFLIP:
            _current_context.y_flip = 0;
            break;
        case VIDEO_Y_FLIP:
            _current_context.y_flip = 1;
            break;

        case VIDEO_NO_BLEND:
            _current_context.blend = 0;
            break;
        case VIDEO_BLEND:
            _current_context.blend = 1;
            break;
        case VIDEO_BLEND_ADD:
            _current_context.blend = 2;
            break;

        default:
            IF_PRINT_WARNING(VIDEO_DEBUG) << "Unknown flag in argument list: " << flag << std::endl;
            break;
        }
        flag = va_arg(args, int32);
    }
    va_end(args);
}

void VideoEngine::Clear()
{
    //! \todo glClearColor is a state change operation. It should only be called when the clear color changes
    Clear(Color::black);
}

void VideoEngine::Clear(const Color& c)
{
    _current_context.viewport = ScreenRect(_viewport_x_offset, _viewport_y_offset, _viewport_width, _viewport_height);
    glViewport(_viewport_x_offset, _viewport_y_offset, _viewport_width, _viewport_height);
    glClearColor(c[0], c[1], c[2], c[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    TextureManager->_debug_num_tex_switches = 0;
}

void VideoEngine::Update()
{
    uint32 frame_time = vt_system::SystemManager->GetUpdateTime();

    _screen_fader.Update(frame_time);

    if (_fps_display)
        _UpdateFPS();
}

void VideoEngine::DrawDebugInfo()
{
    if (TextureManager->_debug_current_sheet >= 0)
        TextureManager->DEBUG_ShowTexSheet();

    if (_fps_display)
        _DrawFPS();
}

bool VideoEngine::CheckGLError() {
    if(!VIDEO_DEBUG)
        return false;

    _gl_error_code = glGetError();
    return (_gl_error_code != GL_NO_ERROR);
}

const std::string VideoEngine::CreateGLErrorString()
{
    const GLubyte *error_string = gluErrorString(_gl_error_code);

    if(error_string == nullptr)
        return ("Unknown GL error code: " + NumberToString(_gl_error_code));
    else
        return (char *)error_string;
}

//-----------------------------------------------------------------------------
// VideoEngine class - Screen size and resolution methods
//-----------------------------------------------------------------------------

void VideoEngine::GetPixelSize(float &x, float &y)
{
    x = fabs(_current_context.coordinate_system.GetRight() - _current_context.coordinate_system.GetLeft()) / _viewport_width;
    y = fabs(_current_context.coordinate_system.GetTop() - _current_context.coordinate_system.GetBottom()) / _viewport_height;
}

bool VideoEngine::ApplySettings()
{
    if (!_sdl_window) {
        PRINT_WARNING << "Invalid SDL_Window instance. Can't apply video settings." << std::endl;
        return false;
    }

    // Potentially losing GL context, so unload images first
    // TODO: Still needed?
    if(!TextureManager || !TextureManager->UnloadTextures())
        IF_PRINT_WARNING(VIDEO_DEBUG) << "failed to delete OpenGL textures during a context change" << std::endl;

    // Clear GL state, for OSX compatibility
    // TODO: Is that still true?
    DisableBlending();
    DisableTexture2D();
    DisableStencilTest();
    DisableScissoring();

    // Turn off writing to the depth buffer
    glDepthMask(GL_FALSE);

    if (_temp_fullscreen && !_fullscreen) {
        // We want to go in fullscreen mode
        // Get desktop resolution and adapt the current resolution
        int32 display_index = SDL_GetWindowDisplayIndex(_sdl_window);
        if (display_index < 0) {
            if(TextureManager)
                TextureManager->ReloadTextures();
            return false;
        }
        SDL_DisplayMode dsp_mode;
        if (SDL_GetDesktopDisplayMode(display_index, &dsp_mode) < 0) {
            if(TextureManager)
                TextureManager->ReloadTextures();
            return false;
        }

        // Try to apply the fullscreen mode.
        if (SDL_SetWindowFullscreen(_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
            if(TextureManager)
                TextureManager->ReloadTextures();
            return false;
        }
        // Set the resolution to the current desktop one.
        _temp_width = dsp_mode.w;
        _temp_height = dsp_mode.h;
    }
    else if (!_temp_fullscreen && _fullscreen) {
        // We want to go in windowed mode
        if (SDL_SetWindowFullscreen(_sdl_window, 0) < 0) {
            if(TextureManager)
                TextureManager->ReloadTextures();
            return false;
        }
        // Go back to windowed mode. Let's not apply a too high resolution
        // in this case to permit the player to still see the menus.
        if (_temp_width > 1024) {
            _temp_width = 1024;
            _temp_height = 768;
        }
        SDL_SetWindowSize(_sdl_window, _temp_width, _temp_height);
    }
    else if (_temp_height != _screen_height || _temp_width != _screen_width) {
        // We simply want to change the current resolution.
        SDL_SetWindowSize(_sdl_window, _temp_width, _temp_height);
    }

    // Now that the new settings worked, apply them on the config (and the viewport)
    _screen_width = _temp_width;
    _screen_height = _temp_height;
    _fullscreen = _temp_fullscreen;

    _UpdateViewportMetrics();

    // Try to apply the VSync mode
    if (_vsync_mode > 2)
        _vsync_mode = 0;

    // Try Swap tearing
    if (_vsync_mode == 2 && SDL_GL_SetSwapInterval(-1) != 0) {
        // Swap tearing failed, attempt VSync.
        _vsync_mode = 1;
    }
    // Try VSync
    if (_vsync_mode == 1 && SDL_GL_SetSwapInterval(1) != 0) {
        // VSync failed, fall-back to none.
        _vsync_mode = 0;
    }
    // No VSync
    if (_vsync_mode == 0)
        SDL_GL_SetSwapInterval(0);

    if(TextureManager)
        TextureManager->ReloadTextures();

    return true;
}

void VideoEngine::_UpdateViewportMetrics()
{
    // Test the desired resolution and adds the necessary offsets if it's not a 4:3 one
    float width = _screen_width;
    float height = _screen_height;
    float scr_ratio = height > 0.2f ? width / height : 1.33f;
    if (vt_utils::IsFloatEqual(scr_ratio, 1.33f, 0.2f)) { // 1.33f == 4:3
        // 4:3: No offsets
        _viewport_x_offset = 0;
        _viewport_y_offset = 0;
        _viewport_width = _screen_width;
        _viewport_height = _screen_height;
        return;
    }

    // Handle non 4:3 cases
    if (width >= height) {
        float ideal_width = height / 3.0f * 4.0f;
        _viewport_width = ideal_width;
        _viewport_height = _screen_height;
        _viewport_x_offset = (int32)((width - ideal_width) / 2.0f);
        _viewport_y_offset = 0;
    }
    else {
        float ideal_height = width / 3.0f * 4.0f;
        _viewport_height = ideal_height;
        _viewport_width = _screen_width;
        _viewport_x_offset = 0;
        _viewport_y_offset = (int32)((height - ideal_height) / 2.0f);
    }
}

//-----------------------------------------------------------------------------
// VideoEngine class - Coordinate system and viewport methods
//-----------------------------------------------------------------------------

void VideoEngine::SetCoordSys(const CoordSys &coordinate_system)
{
    _current_context.coordinate_system = coordinate_system;

    float left = _current_context.coordinate_system.GetLeft();
    float right = _current_context.coordinate_system.GetRight();
    float bottom = _current_context.coordinate_system.GetBottom();
    float top = _current_context.coordinate_system.GetTop();
    float near_z = -1.0f;
    float far_z = 1.0f;

    // Calculate the orthographic projection.
    float m00 = 2.0f / (right - left);
    float m11 = 2.0f / (top - bottom);
    float m22 = -2.0f / (far_z - near_z);

    float m03 = -(right + left) / (right - left);
    float m13 = -(top + bottom) / (top - bottom);
    float m23 = -(far_z + near_z) / (far_z - near_z);

    // Store the orthographic projection.
    _projection = gl::Transform(m00, 0.0f, 0.0f, m03,
                                0.0f, m11, 0.0f, m13,
                                0.0f, 0.0f, m22, m23,
                                0.0f, 0.0f, 0.0f, 1.0f);
}

void VideoEngine::GetCurrentViewport(float &x, float &y, float &width, float &height)
{
    GLint viewport_dimensions[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_VIEWPORT, viewport_dimensions);

    x = (float) viewport_dimensions[0];
    y = (float) viewport_dimensions[1];
    width = (float) viewport_dimensions[2];
    height = (float) viewport_dimensions[3];
}

void VideoEngine::SetViewport(float x, float y, float width, float height)
{
    if(width <= 0 || height <= 0)
    {
        PRINT_WARNING << "attempted to set an invalid viewport size: " << x << "," << y
            << " at " << width << ":" << height << std::endl;
        return;
    }

    _viewport_x_offset = x;
    _viewport_y_offset = y;
    _viewport_width = width;
    _viewport_height = height;
    glViewport(_viewport_x_offset, _viewport_y_offset, _viewport_width, _viewport_height);
}

void VideoEngine::EnableBlending()
{
    if(!_gl_blend_is_active) {
        glEnable(GL_BLEND);
        _gl_blend_is_active = true;
    }
}

void VideoEngine::DisableBlending()
{
    if(_gl_blend_is_active) {
        glDisable(GL_BLEND);
        _gl_blend_is_active = false;
    }
}

void VideoEngine::EnableStencilTest()
{
    if(!_gl_stencil_test_is_active) {
        glEnable(GL_STENCIL_TEST);
        _gl_stencil_test_is_active = true;
    }
}

void VideoEngine::DisableStencilTest()
{
    if(_gl_stencil_test_is_active) {
        glDisable(GL_STENCIL_TEST);
        _gl_stencil_test_is_active = false;
    }
}

void VideoEngine::EnableTexture2D()
{
    if(!_gl_texture_2d_is_active) {
        glEnable(GL_TEXTURE_2D);
        _gl_texture_2d_is_active = true;
    }
}

void VideoEngine::DisableTexture2D()
{
    if(_gl_texture_2d_is_active) {
        glDisable(GL_TEXTURE_2D);
        _gl_texture_2d_is_active = false;
    }
}

gl::ShaderProgram* VideoEngine::LoadShaderProgram(const gl::shader_programs::ShaderPrograms& shader_program)
{
    gl::ShaderProgram* result = nullptr;

    assert(_programs.find(shader_program) != _programs.end());
    if (_programs.find(shader_program) != _programs.end()) {
        result = _programs.at(shader_program);
        result->Load();
    }

    return result;
}

void VideoEngine::UnloadShaderProgram()
{
    glUseProgram(0);
}

void VideoEngine::DrawParticleSystem(gl::ShaderProgram* shader_program,
                                     float* vertex_positions,
                                     float* vertex_texture_coordinates,
                                     float* vertex_colors,
                                     unsigned number_of_vertices)
{
    assert(_particle_system != nullptr);
    assert(shader_program != nullptr);
    assert(vertex_positions != nullptr);
    assert(vertex_texture_coordinates != nullptr);
    assert(vertex_colors != nullptr);
    assert(number_of_vertices % 4 == 0);

    // Load the shader uniforms common to all programs.
    float buffer[16] = { 0 };
    _transform_stack.top().Apply(buffer);
    shader_program->UpdateUniform("u_Model", buffer, 16);

    gl::Transform identity;
    identity.Apply(buffer);
    shader_program->UpdateUniform("u_View", buffer, 16);

    _projection.Apply(buffer);
    shader_program->UpdateUniform("u_Projection", buffer, 16);

    shader_program->UpdateUniform("u_Color", reinterpret_cast<const float*>(&::vt_video::Color::white), 4);

    // Draw the particle system.
    _particle_system->Draw(vertex_positions, vertex_texture_coordinates, vertex_colors, number_of_vertices);
}

void VideoEngine::DrawSprite(gl::ShaderProgram* shader_program,
                             float* vertex_positions,
                             float* vertex_texture_coordinates,
                             float* vertex_colors,
                             const Color& color)
{
    assert(_sprite != nullptr);
    assert(shader_program != nullptr);
    assert(vertex_positions != nullptr);
    assert(vertex_texture_coordinates != nullptr);
    assert(vertex_colors != nullptr);

    // Load the shader uniforms common to all programs.
    float buffer[16] = { 0 };
    _transform_stack.top().Apply(buffer);
    shader_program->UpdateUniform("u_Model", buffer, 16);

    gl::Transform identity;
    identity.Apply(buffer);
    shader_program->UpdateUniform("u_View", buffer, 16);

    _projection.Apply(buffer);
    shader_program->UpdateUniform("u_Projection", buffer, 16);

    shader_program->UpdateUniform("u_Color", color.GetColors(), 4);

    // Draw the sprite.
    _sprite->Draw(vertex_positions, vertex_texture_coordinates, vertex_colors);
}

void VideoEngine::EnableScissoring()
{
    _current_context.scissoring_enabled = true;
    if (!_gl_scissor_test_is_active) {
        glEnable(GL_SCISSOR_TEST);
        _gl_scissor_test_is_active = true;
    }
}

void VideoEngine::DisableScissoring()
{
    _current_context.scissoring_enabled = false;
    if (_gl_scissor_test_is_active) {
        glDisable(GL_SCISSOR_TEST);
        _gl_scissor_test_is_active = false;
    }
}

void VideoEngine::SetScissorRect(unsigned x, unsigned y, unsigned width, unsigned height)
{
    SetScissorRect(ScreenRect(x, y, width, height));
}

void VideoEngine::SetScissorRect(const ScreenRect& screen_rectangle)
{
    _current_context.scissor_rectangle = screen_rectangle;

    glScissor(static_cast<GLint>(_current_context.scissor_rectangle.left),
              static_cast<GLint>(_current_context.scissor_rectangle.top),
              static_cast<GLsizei>(_current_context.scissor_rectangle.width),
              static_cast<GLsizei>(_current_context.scissor_rectangle.height));
}

//-----------------------------------------------------------------------------
// VideoEngine class - Transformation methods
//-----------------------------------------------------------------------------

void VideoEngine::Move(float x, float y)
{
    _transform_stack.top().Reset();
    _transform_stack.top().Translate(x, y);

    _x_cursor = x;
    _y_cursor = y;
}

void VideoEngine::MoveRelative(float x, float y)
{
    _transform_stack.top().Translate(x, y);

    _x_cursor += x;
    _y_cursor += y;
}

void VideoEngine::PushMatrix()
{
    _transform_stack.push(_transform_stack.top());
}

void VideoEngine::PopMatrix()
{
    // Sanity.
    if (!_transform_stack.empty()) {
        _transform_stack.pop();
    }

    // Sanity.
    if (_transform_stack.empty()) {
        _transform_stack.push(gl::Transform());
    }
}

void VideoEngine::PushState()
{
    PushMatrix();

    _context_stack.push(_current_context);
}

void VideoEngine::PopState()
{
    // Restore the most recent context information and pop it from stack
    if(_context_stack.empty()) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "no video states were saved on the stack" << std::endl;
        return;
    }

    _current_context = _context_stack.top();
    _context_stack.pop();

    PopMatrix();

    glViewport(_current_context.viewport.left, _current_context.viewport.top, _current_context.viewport.width, _current_context.viewport.height);

    if (_current_context.scissoring_enabled) {
        EnableScissoring();
        SetScissorRect(_current_context.scissor_rectangle);
    } else {
        DisableScissoring();
    }
}

void VideoEngine::Rotate(float angle)
{
    _transform_stack.top().Rotate(angle);
}

void VideoEngine::Scale(float x, float y)
{
    _transform_stack.top().Scale(x, y);
}

void VideoEngine::DrawFadeEffect()
{
    _screen_fader.Draw();
}

void VideoEngine::DisableFadeEffect()
{
    // Disable potential game fades as it is just another light effect.
    // Transitional effects are done by the mode manager and shouldn't
    // be interrupted.
    if(IsFading() && !IsLastFadeTransitional())
        FadeIn(0);
}

StillImage VideoEngine::CaptureScreen() throw(Exception)
{
    // Static variable used to make sure the capture has a unique name in the texture image map
    static uint32 capture_id = 0;

    StillImage screen_image;

    // Retrieve width/height of the viewport. viewport_dimensions[2] is the width, [3] is the height
    GLint viewport_dimensions[4];
    glGetIntegerv(GL_VIEWPORT, viewport_dimensions);
    screen_image.SetDimensions((float)viewport_dimensions[2], (float)viewport_dimensions[3]);

    // Set up the screen rectangle to copy
    ScreenRect screen_rect(viewport_dimensions[0], viewport_dimensions[1], viewport_dimensions[2], viewport_dimensions[3]);

    // Create a new ImageTexture with a unique filename for this newly captured screen
    ImageTexture *new_image = new ImageTexture("capture_screen" + NumberToString(capture_id), "<T>", viewport_dimensions[2], viewport_dimensions[3]);
    new_image->AddReference();

    // Create a texture sheet of an appropriate size that can retain the capture
    TexSheet *temp_sheet = TextureManager->_CreateTexSheet(RoundUpPow2(viewport_dimensions[2]), RoundUpPow2(viewport_dimensions[3]), VIDEO_TEXSHEET_ANY, false);
    VariableTexSheet *sheet = dynamic_cast<VariableTexSheet *>(temp_sheet);

    // Ensure that texture sheet creation succeeded, insert the texture image into the sheet, and copy the screen into the sheet
    if(sheet == nullptr) {
        delete new_image;
        throw Exception("could not create texture sheet to store captured screen", __FILE__, __LINE__, __FUNCTION__);
    }
    if(sheet->InsertTexture(new_image) == false) {
        TextureManager->_RemoveSheet(sheet);
        delete new_image;
        throw Exception("could not insert captured screen image into texture sheet", __FILE__, __LINE__, __FUNCTION__);
    }
    if(sheet->CopyScreenRect(0, 0, screen_rect) == false) {
        TextureManager->_RemoveSheet(sheet);
        delete new_image;
        throw Exception("call to TexSheet::CopyScreenRect() failed", __FILE__, __LINE__, __FUNCTION__);
    }

    // Store the image element to the saved image (with a flipped y axis)
    screen_image._image_texture = new_image;
    screen_image._texture = new_image;

    // Vertically flip the texture image by swapping the v coordinates, since OpenGL returns the image upside down in the CopyScreenRect call
    float temp = new_image->v1;
    new_image->v1 = new_image->v2;
    new_image->v2 = temp;

    ++capture_id;
    return screen_image;
}

StillImage VideoEngine::CreateImage(ImageMemory *raw_image, const std::string &image_name, bool delete_on_exist) throw(Exception)
{
    //the returning image
    StillImage still_image;

    //check if the raw_image pointer is valid
    if(!raw_image)
    {
        throw Exception("raw_image is nullptr, cannot create a StillImage", __FILE__, __LINE__, __FUNCTION__);
    }

    still_image.SetDimensions(raw_image->width, raw_image->height);

    //Check to see if the image_name exists
    if(TextureManager->_IsImageTextureRegistered(image_name))
    {
        //if we are allowed to delete, then we remove the texture
        if(delete_on_exist)
        {
            ImageTexture* old = TextureManager->_GetImageTexture(image_name);
            TextureManager->_UnregisterImageTexture(old);
            if(old->RemoveReference())
                delete old;
        }
        else
        {
            throw Exception("image already exists in texture manager", __FILE__, __LINE__, __FUNCTION__);
        }
    }

    //create a new texture image. the next few steps are similar to CaptureImage, so in the future
    // we may want to do a code-cleanup
    ImageTexture *new_image = new ImageTexture(image_name, "<T>", raw_image->width, raw_image->height);
    new_image->AddReference();
    // Create a texture sheet of an appropriate size that can retain the capture
    TexSheet *temp_sheet = TextureManager->_CreateTexSheet(RoundUpPow2(raw_image->width), RoundUpPow2(raw_image->height), VIDEO_TEXSHEET_ANY, false);
    VariableTexSheet *sheet = dynamic_cast<VariableTexSheet *>(temp_sheet);

    // Ensure that texture sheet creation succeeded, insert the texture image into the sheet, and copy the screen into the sheet
    if(sheet == nullptr) {
        delete new_image;
        throw Exception("could not create texture sheet to store still image", __FILE__, __LINE__, __FUNCTION__);
    }

    if(sheet->InsertTexture(new_image) == false)
    {
        TextureManager->_RemoveSheet(sheet);
        delete new_image;
        throw Exception("could not insert raw image into texture sheet", __FILE__, __LINE__, __FUNCTION__);
    }

    if(sheet->CopyRect(0, 0, *raw_image) == false)
    {
        TextureManager->_RemoveSheet(sheet);
        delete new_image;
        throw Exception("call to TexSheet::CopyRect() failed", __FILE__, __LINE__, __FUNCTION__);
    }

    // Store the image element to the saved image (with a flipped y axis)
    still_image._image_texture = new_image;
    still_image._texture = new_image;
    return still_image;
}

bool VideoEngine::IsScreenShaking()
{
    vt_mode_manager::GameMode *gm = vt_mode_manager::ModeManager->GetTop();

    if (!gm)
        return false;

    vt_mode_manager::EffectSupervisor &effects = gm->GetEffectSupervisor();
    if (!effects.IsScreenShaking())
        return false;

    // update the shaking offsets before returning
    effects.GetShakingOffsets(_x_shake, _y_shake);
    return true;
}

void VideoEngine::SetBrightness(float value)
{
    _brightness_value = value;

    // Limit min/max brightness
    if(_brightness_value > 2.0f) {
        _brightness_value = 2.0f;
    } else if(_brightness_value < 0.0f) {
        _brightness_value = 0.0f;
    }

    SDL_SetWindowBrightness(_sdl_window, _brightness_value);
}

void VideoEngine::MakeScreenshot(const std::string &filename)
{
    private_video::ImageMemory buffer;

    // Retrieve the width and height of the viewport.
    GLint viewport_dimensions[4]; // viewport_dimensions[2] is the width, [3] is the height
    glGetIntegerv(GL_VIEWPORT, viewport_dimensions);

    // Buffer to store the image before it is flipped
    buffer.width = viewport_dimensions[2];
    buffer.height = viewport_dimensions[3];
    buffer.pixels = malloc(buffer.width * buffer.height * 3);
    buffer.rgb_format = true;

    // Read the viewport pixel data
    glReadPixels(viewport_dimensions[0], viewport_dimensions[1],
                 buffer.width, buffer.height, GL_RGB, GL_UNSIGNED_BYTE, buffer.pixels);

    if(CheckGLError() == true) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "an OpenGL error occured: " << CreateGLErrorString() << std::endl;

        free(buffer.pixels);
        buffer.pixels = nullptr;
        return;
    }

    // Vertically flip the image, then swap the flipped and original images
    void *buffer_temp = malloc(buffer.width * buffer.height * 3);
    for(uint32 i = 0; i < buffer.height; ++i) {
        memcpy((uint8 *)buffer_temp + i * buffer.width * 3,
               (uint8 *)buffer.pixels + (buffer.height - i - 1) * buffer.width * 3, buffer.width * 3);
    }
    void *temp = buffer.pixels;
    buffer.pixels = buffer_temp;
    buffer_temp = temp;

    buffer.SaveImage(filename);

    free(buffer_temp);
    free(buffer.pixels);
    buffer.pixels = nullptr;
}

int32 VideoEngine::_ConvertYAlign(int32 y_align)
{
    switch(y_align) {
    case VIDEO_Y_BOTTOM:
        return -1;
    case VIDEO_Y_CENTER:
        return 0;
    case VIDEO_Y_TOP:
        return 1;
    default:
        IF_PRINT_WARNING(VIDEO_DEBUG) << "unknown value for argument flag: " << y_align << std::endl;
        return 0;
    }
}

int32 VideoEngine::_ConvertXAlign(int32 x_align)
{
    switch(x_align) {
    case VIDEO_X_LEFT:
        return -1;
    case VIDEO_X_CENTER:
        return 0;
    case VIDEO_X_RIGHT:
        return 1;
    default:
        IF_PRINT_WARNING(VIDEO_DEBUG) << "unknown value for argument flag: " << x_align << std::endl;
        return 0;
    }
}

void VideoEngine::DrawLine(float x1, float y1, unsigned width1, float x2, float y2, unsigned width2, const Color &color)
{
    //
    // Compute the line's vertex positions.
    //

    // This is the equation for drawing a line with different starting and ending widths.
    float angle = atan2(static_cast<float>(y2 - y1), static_cast<float>(x2 - x1));
    float w2sina1 = static_cast<float>(width1) / 2.0f * sin(angle);
    float w2cosa1 = static_cast<float>(width1) / 2.0f * cos(angle);
    float w2sina2 = static_cast<float>(width2) / 2.0f * sin(angle);
    float w2cosa2 = static_cast<float>(width2) / 2.0f * cos(angle);

    float vertex_positions[] =
    {
        x1 + w2sina1, y1 - w2cosa1, 0.0f, // Vertex One.
        x2 + w2sina2, y2 - w2cosa2, 0.0f, // Vertex Two.
        x2 - w2sina2, y2 + w2cosa2, 0.0f, // Vertex Three.
        x1 - w2sina1, y1 + w2cosa1, 0.0f  // Vertex Four.
    };

    // The vertex texture coordinates.
    // These will be ignored in this case.
    float vertex_texture_coordinates[] =
    {
        0.0f, 0.0f, // Vertex One.
        0.0f, 0.0f, // Vertex Two.
        0.0f, 0.0f, // Vertex Three.
        0.0f, 0.0f  // Vertex Four.
    };

    // The vertex colors.
    // These will be ignored in this case.
    float vertex_colors[] =
    {
        1.0f, 1.0f, 1.0f, 1.0f, // Vertex One.
        1.0f, 1.0f, 1.0f, 1.0f, // Vertex Two.
        1.0f, 1.0f, 1.0f, 1.0f, // Vertex Three.
        1.0f, 1.0f, 1.0f, 1.0f  // Vertex Four.
    };

    EnableBlending();
    DisableTexture2D();

    // Normal blending.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load the solid shader program.
    gl::ShaderProgram* shader_program = VideoManager->LoadShaderProgram(gl::shader_programs::Solid);
    assert(shader_program != nullptr);

    // Draw the line.
    DrawSprite(shader_program, vertex_positions, vertex_texture_coordinates, vertex_colors, color);

    // Unload the shader program.
    UnloadShaderProgram();
}

void VideoEngine::DrawGrid(float left, float top, float right, float bottom, float width_cell_horizontal, float width_cell_vertical, unsigned width_line, const Color& color)
{
    assert(right > left);
    assert(bottom > top);
    assert(width_cell_horizontal > 0.0f);
    assert(width_cell_vertical > 0.0f);
    assert(width_line > 0);

    // Draw the grid's vertical lines.
    for (float i = left; i <= right; i += width_cell_horizontal)
    {
        DrawLine(i, top, width_line, i, bottom, width_line, color);
    }

    // Draw the grid's horizontal lines.
    for (float j = top; j <= bottom; j += width_cell_vertical)
    {
        DrawLine(left, j, width_line, right, j, width_line, color);
    }
}

void VideoEngine::DrawRectangle(float width, float height, const Color &color)
{
    _rectangle_image._width = width;
    _rectangle_image._height = height;
    _rectangle_image._color[0] = color;

    _rectangle_image.Draw(color);
}

void VideoEngine::DrawRectangleOutline(float left, float right, float bottom, float top, unsigned width, const Color &color)
{
    DrawLine(left, bottom, width, right, bottom, width, color);
    DrawLine(left, top, width, right, top, width, color);
    DrawLine(left, bottom, width, left, top, width, color);
    DrawLine(right, bottom, width, right, top, width, color);
}

void VideoEngine::DrawHalo(const ImageDescriptor &id, const Color &color)
{
    char old_blend_mode = _current_context.blend;
    _current_context.blend = VIDEO_BLEND_ADD;
    id.Draw(color);
    _current_context.blend = old_blend_mode;
}

}  // namespace vt_video
