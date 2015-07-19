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
*** \file   gui.cpp
*** \author Raj Sharma, roos@allacrost.org
*** \author Yohann Ferreira, yohann ferreira orange fr
*** \brief  Source file for GUI code
***
*** ***************************************************************************/

#include "utils/utils_pch.h"
#include "gui.h"

#include "engine/video/video.h"
#include "common/gui/menu_window.h"

#include "engine/script/script_read.h"

using namespace vt_utils;
using namespace vt_video;

namespace vt_gui
{

GUISystem *GUIManager = nullptr;

namespace private_gui
{

// *****************************************************************************
// ******************************* GUIElement **********************************
// *****************************************************************************

GUIElement::GUIElement() :
    _xalign(VIDEO_X_LEFT),
    _yalign(VIDEO_Y_TOP),
    _x_position(0.0f),
    _y_position(0.0f),
    _width(0.0f),
    _height(0.0f)
{}

void GUIElement::SetDimensions(float w, float h)
{
    if(w <= 0.0f) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "invalid width argument: " << w << std::endl;
        return;
    }

    if(h <= 0.0f) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "invalid height argument: " << h << std::endl;
        return;
    }

    _width = w;
    _height = h;
}

void GUIElement::SetAlignment(int32 xalign, int32 yalign)
{
    if(_xalign != VIDEO_X_LEFT && _xalign != VIDEO_X_CENTER && _xalign != VIDEO_X_RIGHT) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "invalid xalign value: " << xalign << std::endl;
        return;
    }

    if(_yalign != VIDEO_Y_TOP && _yalign != VIDEO_Y_CENTER && _yalign != VIDEO_Y_BOTTOM) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "invalid yalign value: " << yalign << std::endl;
        return;
    }

    _xalign = xalign;
    _yalign = yalign;
}

void GUIElement::CalculateAlignedRect(float &left, float &right, float &bottom, float &top)
{
    float width = right - left;
    float height = top - bottom;

    if(width < 0.0f)
        width = -width;

    if(height < 0.0f)
        height = -height;

    if(VideoManager->_current_context.coordinate_system.GetVerticalDirection() < 0.0f)
        top = -top;

    if(VideoManager->_current_context.coordinate_system.GetHorizontalDirection() < 0.0f)
        right = -right;

    float x_off, y_off;

    x_off = _x_position + ((VideoManager->_current_context.x_align + 1) * width)  * 0.5f *
            -VideoManager->_current_context.coordinate_system.GetHorizontalDirection();
    y_off = _y_position + ((VideoManager->_current_context.y_align + 1) * height) * 0.5f *
            -VideoManager->_current_context.coordinate_system.GetVerticalDirection();

    left   += x_off;
    right  += x_off;
    top    += y_off;
    bottom += y_off;
} // void GUIElement::CalculateAlignedRect(float &left, float &right, float &bottom, float &top)

void GUIElement::_DEBUG_DrawOutline()
{
    float left = 0.0f;
    float right = _width;
    float bottom = 0.0f;
    float top = _height;

    VideoManager->Move(0.0f, 0.0f);
    CalculateAlignedRect(left, right, bottom, top);
    VideoManager->DrawRectangleOutline(left, right, bottom, top, 3, alpha_black);
    VideoManager->DrawRectangleOutline(left, right, bottom, top, 1, alpha_white);
}

// *****************************************************************************
// ******************************* GUIControl **********************************
// *****************************************************************************

void GUIControl::CalculateAlignedRect(float &left, float &right, float &bottom, float &top)
{
    GUIElement::CalculateAlignedRect(left, right, bottom, top);

    // calculate the position offsets due to the owner window
    if(!_owner)
        return;

    // first, calculate the owner menu's rectangle
    float menu_left, menu_right, menu_bottom, menu_top;
    float menu_height, menu_width;

    _owner->GetDimensions(menu_width, menu_height);
    menu_left = 0.0f;
    menu_right = menu_width;
    menu_bottom = 0.0f;
    menu_top = menu_height;
    VideoManager->PushState();

    int32 xalign, yalign;
    _owner->GetAlignment(xalign, yalign);

    VideoManager->SetDrawFlags(xalign, yalign, 0);
    _owner->CalculateAlignedRect(menu_left, menu_right, menu_bottom, menu_top);
    VideoManager->PopState();

    // now, depending on the alignment of the control, add an offset
    if(menu_left < menu_right) {
        left += menu_left;
        right += menu_left;
    } else {
        left += menu_right;
        right += menu_right;
    }

    if(menu_top < menu_bottom) {
        top += menu_top;
        bottom += menu_top;
    } else {
        top += menu_bottom;
        bottom += menu_bottom;
    }
}

void GUIControl::_DEBUG_DrawOutline()
{
    float left = 0.0f;
    float right = _width;
    float bottom = 0.0f;
    float top = _height;

    VideoManager->Move(0.0f, 0.0f);
    CalculateAlignedRect(left, right, bottom, top);
    VideoManager->DrawRectangleOutline(left, right, bottom, top, 3, alpha_black);
    VideoManager->DrawRectangleOutline(left, right, bottom, top, 1, alpha_white);
}

} // namespace private_gui

using namespace private_gui;

// *****************************************************************************
// ****************************** GUISystem ********************************
// *****************************************************************************

GUISystem::GUISystem():
    _default_skin(nullptr)
{
    _DEBUG_draw_outlines = false;
}

GUISystem::~GUISystem()
{
    // Determine if any MenuWindows have not yet been deleted, and delete them if they exist
    if(!_menu_windows.empty()) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "there were undestroyed MenuWindows in GUISystem destructor" << std::endl;
        for(uint32 i = 0; i < _menu_windows.size(); ++i)
            _menu_windows[i]->Destroy();

        _menu_windows.clear();
    }

    // Delete all menu skins which are still active
    _menu_skins.clear();
}

bool GUISystem::LoadMenuSkin(const std::string& skin_id,
                             const std::string& skin_name, const std::string& cursor_file, const std::string& scroll_arrows_file,
                             const std::string& border_image, const std::string& background_image, bool make_default)
{
    // Check that the skin_id is not already used by another skin
    if(_menu_skins.find(skin_id) != _menu_skins.end()) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "the skin id " << skin_id << " is already used by another skin" << std::endl;
        return false;
    }

    _menu_skins.insert(std::make_pair(skin_id, MenuSkin()));
    MenuSkin &new_skin = _menu_skins[skin_id];

    if (!new_skin.menu_cursor.Load(cursor_file)) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "Couldn't load the GUI cursor file: '" << cursor_file << "'." << std::endl;
        _menu_skins.erase(skin_id);
        return false;
    }

    new_skin.skin_name = MakeUnicodeString(skin_name);

    // Load the MultiImage containing the borders of the skin.
    std::vector<StillImage> skin_borders;
    if(ImageDescriptor::LoadMultiImageFromElementGrid(skin_borders, border_image, 3, 6) == false) {
        _menu_skins.erase(skin_id);
        return false;
    }

    // Copy the borders over to the new MenuSkin and delete the unused images
    new_skin.borders[0][0] = skin_borders[0];
    new_skin.borders[0][1] = skin_borders[1];
    new_skin.borders[0][2] = skin_borders[2];
    new_skin.borders[1][0] = skin_borders[6];
    new_skin.borders[1][2] = skin_borders[8];
    new_skin.borders[2][0] = skin_borders[12];
    new_skin.borders[2][1] = skin_borders[13];
    new_skin.borders[2][2] = skin_borders[14];

    new_skin.connectors[0] = skin_borders[4];
    new_skin.connectors[1] = skin_borders[16];
    new_skin.connectors[2] = skin_borders[9];
    new_skin.connectors[3] = skin_borders[11];
    new_skin.connectors[4] = skin_borders[10];

    // The skin borders at indeces: 3, 5, 7, 15, and 17 are not used, and will be discarded when
    // they go out of scope (ie when this function returns)

    // Load the scroll arrow
    if(!ImageDescriptor::LoadMultiImageFromElementGrid(new_skin.scroll_arrows, scroll_arrows_file, 2, 4)) {
        _menu_skins.erase(skin_id);
        return false;
    }

    // Load the background image, if one has been specified
    if(!background_image.empty()) {
        if(new_skin.background.Load(background_image) == false) {
            IF_PRINT_WARNING(VIDEO_DEBUG) << "the background image file could not be loaded" << std::endl;
            _menu_skins.erase(skin_id);
            return false;
        }
    }

    // Determine if this new skin should be made the default skin
    if(make_default == true || _menu_skins.size() == 1) {
        _default_skin = &new_skin;
    }

    return true;
}

void GUISystem::SetUserMenuSkin(const std::string &skin_id)
{
    _user_menu_skin = skin_id;
}

std::string GUISystem::GetUserMenuSkinId()
{
    return _user_menu_skin;
}

bool GUISystem::SetDefaultMenuSkin(const std::string &skin_id)
{
    if(_menu_skins.find(skin_id) == _menu_skins.end()) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "the skin id " << skin_id << " was not registered." << std::endl;
        return false;
    }

    _default_skin = &_menu_skins[skin_id];
    return true;
}

void GUISystem::SetNextDefaultMenuSkin()
{
    if (_menu_skins.empty() || !_default_skin)
        return;

    std::vector<std::string> names;
    std::string current_name;

    std::map<std::string, private_gui::MenuSkin>::const_iterator it;
    for (it = _menu_skins.begin(); it != _menu_skins.end(); ++it) {
        // Find the default skin's name.
        if (&(it->second) == _default_skin)
            current_name = it->first;

        // Store the name.
        names.push_back(it->first);
    }

    // Find the default skin in the list of names.
    std::vector<std::string>::iterator default_skin = names.begin();
    for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
        if ((*it) == current_name) {
            default_skin = it;
            break;
        }
    }

    // Increment to the next skin.
    ++default_skin;

    // Wrap around case.
    if (default_skin == names.end())
        default_skin = names.begin();

    // Store the result.
    _default_skin = &_menu_skins[*default_skin];
}

void GUISystem::SetPreviousDefaultMenuSkin()
{
    if (_menu_skins.empty() || !_default_skin)
        return;

    std::vector<std::string> names;
    std::string current_name;

    std::map<std::string, private_gui::MenuSkin>::const_iterator it;
    for (it = _menu_skins.begin(); it != _menu_skins.end(); ++it) {
        // Find the default skin's name.
        if (&(it->second) == _default_skin)
            current_name = it->first;

        // Store the name.
        names.push_back(it->first);
    }

    // Find the default skin in the list of names.
    std::vector<std::string>::iterator default_skin = names.begin();
    for (std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
        if ((*it) == current_name) {
            default_skin = it;
            break;
        }
    }

    // Make sure we are not already at the beginning.
    if (default_skin == names.begin())
        default_skin = names.end();

    // Decrement to the previous skin.
    --default_skin;

    // Store the result.
    _default_skin = &_menu_skins[*default_skin];
}

std::string GUISystem::GetDefaultMenuSkinId()
{
    std::string result;

    if (!_default_skin)
        return result;

    // Iterate over the menu skins.
    std::map<std::string, private_gui::MenuSkin>::const_iterator it;
    for (it = _menu_skins.begin(); it != _menu_skins.end(); ++it) {
        // If we find the default skin...
        if (&(it->second) == _default_skin) {
            // Return its name.
            result = it->first;
            break;
        }
    }

    return result;
}

std::vector<vt_video::StillImage>* GUISystem::GetScrollArrows() const
{
    return &_default_skin->scroll_arrows;
}

vt_video::StillImage* GUISystem::GetCursor() const
{
    return &_default_skin->menu_cursor;
}

const ustring& GUISystem::GetDefaultMenuSkinName() const
{
    return _default_skin->skin_name;
}

void GUISystem::DeleteMenuSkin(const std::string &skin_id)
{
    if(_menu_skins.find(skin_id) == _menu_skins.end()) {
        IF_PRINT_WARNING(VIDEO_DEBUG) << "the skin id " << skin_id << " was not registered." << std::endl;
        return;
    }

    MenuSkin *dead_skin = &_menu_skins[skin_id];

    for (uint32 i = 0; i < _menu_windows.size(); ++i) {
        if(dead_skin == _menu_windows[i]->_skin) {
            IF_PRINT_WARNING(VIDEO_DEBUG) << "the MenuSkin \"" << skin_id << "\" was not deleted because a MenuWindow object was found to be using it" << std::endl;
            return;
        }
        ++i;
    }

    _menu_skins.erase(skin_id);
}

bool GUISystem::IsMenuSkinAvailable(const std::string &skin_id) const
{
    if(_menu_skins.find(skin_id) == _menu_skins.end())
        return false;
    else
        return true;
}

void GUISystem::ReloadSkinNames(const std::string& theme_script_filename)
{
    vt_script::ReadScriptDescriptor theme_script;

    if(!theme_script.OpenFile(theme_script_filename))
        return;

    if (!theme_script.OpenTable("themes")) {
        theme_script.CloseFile();
        return;
    }


    for (std::map<std::string, MenuSkin>::iterator it = _menu_skins.begin();
            it != _menu_skins.end(); ++it) {
        std::string theme_id = it->first;

        if (!theme_script.OpenTable(theme_id))
            continue;

        it->second.skin_name = MakeUnicodeString(theme_script.ReadString("name"));
        theme_script.CloseTable();
    }

    theme_script.CloseFile();
}

private_gui::MenuSkin *GUISystem::_GetMenuSkin(const std::string &skin_name)
{
    if(_menu_skins.find(skin_name) == _menu_skins.end())
        return nullptr;
    else
        return &(_menu_skins[skin_name]);
}



void GUISystem::_AddMenuWindow(MenuWindow *new_window)
{
    // Check first whether the window is already registered.
    std::vector<MenuWindow*>::iterator it = _menu_windows.begin();
    for (; it != _menu_windows.end(); ++it) {
        if ((*it) == new_window)
            return;
    }

    _menu_windows.push_back(new_window);
}



void GUISystem::_RemoveMenuWindow(MenuWindow *old_window)
{
    std::vector<MenuWindow*>::iterator it = _menu_windows.begin();
    for (; it != _menu_windows.end(); ++it) {
        if ((*it) == old_window) {
            _menu_windows.erase(it);
            return;
        }
    }

    IF_PRINT_WARNING(VIDEO_DEBUG) << "did not find a corresponding entry in the menu windows map" << std::endl;
}

} // namespace vt_gui
