////////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2012-2015 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
////////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    gl_shader_programs.h
*** \author  Authenticate, James Lammlein
*** \brief   Header file for shader program definitions.
*** ***************************************************************************/

#ifndef __SHADER_PROGRAMS_HEADER__
#define __SHADER_PROGRAMS_HEADER__

namespace vt_video
{
namespace gl
{
namespace shader_programs
{

enum ShaderPrograms
{
    Solid = 0,
    SolidGrayscale,
    Sprite,
    SpriteGrayscale,
    Count
};

} // namespace shader_programs

} // namespace gl

} // namespace vt_video

#endif
