////////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2012-2015 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
////////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    gl_sprite.h
*** \author  Authenticate, James Lammlein
*** \brief   Header file for buffers for a sprite.
*** ***************************************************************************/

#ifndef __SPRITE_HEADER__
#define __SPRITE_HEADER__

namespace vt_video
{
namespace gl
{

//! \brief A class for drawing a sprite.
class Sprite
{
public:
    Sprite();
    ~Sprite();

    //! \brief Draws a sprite.
    void Draw();

    //! \brief Draws a sprite.
    void Draw(float* vertex_positions,
              float* vertex_texture_coordinates,
              float* vertex_colors);

private:
    //
    // The copy constructor and assignment operator are hidden by design
    // to cause compilation errors when attempting to copy or assign this class.
    //

    Sprite(const Sprite& sprite);
    Sprite& operator=(const Sprite& sprite);

    GLuint _vao;
    GLuint _vertex_position_buffer;
    GLuint _vertex_texture_coordinate_buffer;
    GLuint _vertex_color_buffer;
    GLuint _index_buffer;
};

} // namespace gl

} // namespace vt_video

#endif
