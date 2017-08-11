///////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2012-2017 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See https://www.gnu.org/copyleft/gpl.html for details.
///////////////////////////////////////////////////////////////////////////////

#ifndef __MAP_SAVE_POINT_HEADER__
#define __MAP_SAVE_POINT_HEADER__

#include "modes/map/map_objects/map_object.h"

namespace vt_map
{

namespace private_map
{

class ParticleObject;

/** ****************************************************************************
*** \brief Represents save point on the map
*** ***************************************************************************/
class SavePoint : public MapObject
{
public:
    SavePoint(float x, float y);
    virtual ~SavePoint() override
    {
    }

    //! \brief A C++ wrapper made to create a new object from scripting,
    //! without letting Lua handling the object life-cycle.
    //! \note We don't permit luabind to use constructors here as it can't currently
    //! give the object ownership at construction time.
    static SavePoint* Create(float x, float y);

    //! \brief Updates the object's current animation.
    //! \note the actual image resources is handled by the main map object.
    void Update();

    //! \brief Draws the object to the screen, if it is visible.
    //! \note the actual image resources is handled by the main map object.
    void Draw();

    //! \brief Tells whether a character is in or not, and setup the animation accordingly.
    void SetActive(bool active);

private:
    //! \brief A reference to the current map save animation.
    std::vector<vt_video::AnimatedImage>* _animations;

    //! \brief The corresponding particle object for active/inactive save points pointers
    // Note that those pointers are managed by the object supervisor. Don't delete them.
    ParticleObject* _active_particle_object;
    ParticleObject* _inactive_particle_object;

    //! \brief The sound played when activating the save point.
    std::string _activation_sound_filename;

    //! \brief Tells whether the point has become active
    bool _is_active;
};

} // namespace private_map

} // namespace vt_map

#endif // __MAP_SAVE_POINT_HEADER__
