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
*** \file   gui.h
*** \author Raj Sharma, roos@allacrost.org
*** \author Yohann Ferreira, yohann ferreira orange fr
*** \brief  Header file for GUI code
***
*** This code implements the base structures of the video engine's GUI system.
*** ***************************************************************************/

#ifndef __GUI_HEADER__
#define __GUI_HEADER__

#include "engine/video/color.h"

#include "utils/singleton.h"
#include "utils/ustring.h"

namespace vt_video
{
class StillImage;
class VideoEngine;
}

namespace vt_gui
{

class GUISystem;
class MenuWindow;

//! \brief The singleton pointer for the GUI manager
extern GUISystem *GUIManager;

namespace private_gui
{

class MenuSkin;

//! \brief 50% alpha colors used for debug drawing of GUI element outlines
//@{
const vt_video::Color alpha_black(0.0f, 0.0f, 0.0f, 0.5f);
const vt_video::Color alpha_white(1.0f, 1.0f, 1.0f, 0.5f);
//@}

//! \brief Constants used as indeces to access the GUISystem#_scroll_arrows vector
//@{
const uint32 SCROLL_UP           = 0;
const uint32 SCROLL_DOWN         = 1;
const uint32 SCROLL_LEFT         = 2;
const uint32 SCROLL_RIGHT        = 3;
const uint32 SCROLL_UP_GREY      = 4;
const uint32 SCROLL_DOWN_GREY    = 5;
const uint32 SCROLL_LEFT_GREY    = 6;
const uint32 SCROLL_RIGHT_GREY   = 7;
//@}


/** ****************************************************************************
*** \brief An abstract base class for all GUI elements (windows + controls).
*** This class contains basic functions such as Draw(), Update(), etc.
*** ***************************************************************************/
class GUIElement
{
public:
    GUIElement();

    //! \note The destructor must be re-implemented in all children of this class.
    virtual ~GUIElement()
    {}

    //! \brief Draws the GUI element to the screen.
    virtual void Draw() = 0;

    /** \brief Updates the state of the element.
    *** \param frame_time The time that has elapsed since the last frame was drawn, in milliseconds
    **/
    virtual void Update(uint32 frame_time) = 0;

    /** \brief Sets the width and height of the element
    *** \param w The width to set for the element
    *** \param h The height to set for the element
    *** If either the width or height arguments are negative (or zero) no change will take place. This method only sets the
    *** _width and _height members. Deriving classes may need to override this function to take into consideration
    *** changes that must take place when the element is re-sized.
    **/
    virtual void SetDimensions(float w, float h);

    /** \brief Sets the position of the object.
    *** \param x A reference to store the x coordinate of the object.
    *** \param y A reference to store the y coordinate of the object.
    *** \note X and y are in terms of a 1024x768 coordinate system
    **/
    void SetPosition(float x, float y) {
        _x_position = x;
        _y_position = y;
    }

    /** \brief Sets the alignment of the element.
    *** \param xalign Valid values include VIDEO_X_LEFT, VIDEO_X_CENTER, or VIDEO_X_RIGHT.
    *** \param yalign Valid values include VIDEO_Y_TOP, VIDEO_Y_CENTER, or VIDEO_Y_BOTTOM.
    **/
    void SetAlignment(int32 xalign, int32 yalign);

    /** \brief Returns the width and height of the GUI element
    *** \param w Reference to a variable to hold the width
    *** \param h Reference to a variable to hold the height
    **/
    void GetDimensions(float& w, float& h) const {
        w = _width;
        h = _height;
    }

    //! \brief Returns the width of the GUI element
    inline float GetWidth() const {
        return _width;
    }

    //! \brief Returns the height of the GUI element
    inline float GetHeight() const {
        return _height;
    }

    /** \brief Gets the position of the object.
    *** \param x A reference to store the x coordinate of the object.
    *** \param y A reference to store the y coordinate of the object.
    *** \note X and y are in terms of a 1024x768 coordinate system
    **/
    void GetPosition(float& x, float& y) const {
        x = _x_position;
        y = _y_position;
    }

    //! \brief Returns the position of the GUI element on the x axis.
    inline float GetXPosition() const {
        return _x_position;
    }

    //! \brief Returns the position of the GUI element on the y axis.
    inline float GetYPosition() const {
        return _y_position;
    }

    /** \brief Gets the x and y alignment of the element.
    *** \param xalign - x alignment of the object
    *** \param yalign - y alignment of the object
    **/
    void GetAlignment(int32 &xalign, int32 &yalign) const {
        xalign = _xalign;
        yalign = _yalign;
    }

    /** \brief Calculates and returns the four edges for an aligned rectangle
    *** \param left A reference where to store the coordinates of the rectangle's left edge.
    *** \param right A reference where to store the coordinates of the rectangle's right edge.
    *** \param bottom A reference where to store the coordinates of the rectangle's bttom edge.
    *** \param top A reference where to store the coordinates of the rectangle's top edge.
    ***
    *** Given a rectangle specified in VIDEO_X_LEFT and VIDEO_Y_BOTTOM orientation, this function
    *** transforms the rectangle based on the video engine's alignment flags.
    *** \todo I think this function needs to be renamed. It seems to only be used to compute the
    *** four edges of the GUI element. It should be called "CalculateEdges" or somthing more
    *** specific if it is only used in this manner.
    **/
    virtual void CalculateAlignedRect(float &left, float &right, float &bottom, float &top);

protected:
    //! \brief Members for determining the element's draw alignment.
    int32 _xalign, _yalign;

    //! \brief The x and y position of the gui element.
    float _x_position, _y_position;

    //! \brief The dimensions of the GUI element in pixels.
    float _width, _height;

    //! \brief Draws an outline of the element boundaries
    virtual void _DEBUG_DrawOutline();
}; // class GUIElement


/** ****************************************************************************
*** \brief GUIControl is a type of GUI element, specifically for controls.
*** This is for functions that controls have, but menu windows don't have, such
*** as the SetOwner() function.
*** ***************************************************************************/
class GUIControl : public GUIElement
{
public:
    GUIControl() {
        _owner = nullptr;
    }

    virtual ~GUIControl()
    {}

    /** \brief Calculates and returns the four edges for an aligned rectangle
    *** \param left A reference where to store the coordinates of the rectangle's left edge.
    *** \param right A reference where to store the coordinates of the rectangle's right edge.
    *** \param bottom A reference where to store the coordinates of the rectangle's bttom edge.
    *** \param top A reference where to store the coordinates of the rectangle's top edge.
    *** \note The difference between this function and the one for GUI elements is that
    *** controls must take their owner window into account.
    **/
    virtual void CalculateAlignedRect(float &left, float &right, float &bottom, float &top);

    /** \brief Sets the menu window which "owns" this control.
    *** \param owner_window A pointer to the menu that owns the control.
    *** \note If the control is not owned by any menu window, then set the owner to nullptr.
    *** When a control is owned by a menu, it means that it obeys the menu's scissoring
    *** rectangle so that the control won't be drawn outside of the bounds of the menu.
    *** It also means that the position of the control is relative to the position of the
    *** window. (i.e. control.position += menu.position).
    **/
    virtual void SetOwner(MenuWindow *owner_window) {
        _owner = owner_window;
    }

protected:
    /** \brief A pointer to the menu which owns this control.
    *** When the owner is set to nullptr, the control can draw to any part of the screen
    *** (so scissoring is ignored) and drawing coordinates are not modified.
    **/
    MenuWindow *_owner;

    /** \brief Draws an outline of the control boundaries
    *** \note This implementation uses the
    ***
    **/
    virtual void _DEBUG_DrawOutline();
}; // class GUIControl : public GUIElement

} // namespace private_gui


/** ****************************************************************************
*** \brief A helper class to the video engine to manage all of the GUI functionality.
***
*** There is exactly one instance of this class, which is both created and destroyed
*** by the VideoEngine class. This class is essentially an extension of the GameVideo
*** class which manages the GUI system. It also handles the drawing of the
*** average frames per second (FPS) on the screen.
*** ***************************************************************************/
class GUISystem : public vt_utils::Singleton<GUISystem>
{
    friend class vt_utils::Singleton<GUISystem>;
    friend class vt_video::VideoEngine;
    friend class MenuWindow;
    friend class TextBox;
    friend class OptionBox;
public:
    GUISystem();

    ~GUISystem();

    bool SingletonInitialize()
    { return true; }

    /** \name Methods for loading of menu skins
    ***
    *** These methods all attempt to load a menu skin. The differences between these implementations are
    *** whether the skin includes a background image, cursor image, single background color, multiple background colors,
    *** or some combination thereof. Only the skin_name and border_image arguments are mandatory for all
    *** versions of this function to have.
    ***
    *** \param skin_name The name that will be used to refer to the skin after it is successfully loaded
    *** \param cursor_file The filename for the image that contains the menu's cursor image.
    *** \param border_image The filename for the multi-image that contains the menu's border images
    *** \param background_image The filename for the skin's background image
    *** \param make_default If this skin should be the default menu skin to be used, set this argument to true
    *** \return True if the skin was loaded successfully, or false in case of an error
    ***
    *** A few notes about this function:
    *** - If no other menu skins are loaded when this function is called, the default skin will automatically be set to this skin,
    ***   regardless of the value of the make_default parameter.
    **/
    bool LoadMenuSkin(const std::string& skin_id,
                      const std::string& skin_name, const std::string& cursor_file, const std::string& scroll_arrows_file,
                      const std::string& border_image, const std::string& background_image, bool make_default = false);

    /** \brief Stores the id of the user menu skin.
    *** \param skin_id The id of the user menu skin.
    ***
    *** This function stores the name of the user menu skin.  It does not change
    *** the default menu skin directly.
    **/
    void SetUserMenuSkin(const std::string& skin_id);

    //! \brief Returns the id of the user menu skin.
    std::string GetUserMenuSkinId();

    /** \brief Deletes a menu skin that has been loaded
    *** \param skin_id The id of the loaded menu skin that should be removed
    ***
    *** This function could fail on one of two circumstances. First, if there is no MenuSkin loaded for
    *** the key skin_name, the function will do nothing. Second, if any MenuWindow objects are still
    *** referencing the skin that is trying to be deleted, the function will print a warning message
    *** and not delete the skin. Therefore, <b>before you call this function, you must delete any and all
    *** MenuWindow objects which make use of this skin, or change the skin used by those objects</b>.
    **/
    void DeleteMenuSkin(const std::string &skin_id);

    //! \brief Returns true if there is a menu skin available corresponding to the argument name
    bool IsMenuSkinAvailable(const std::string &skin_id) const;

    /** \brief Sets the default menu skin to use from the set of pre-loaded skins
    *** \param skin_id The name of the already loaded menu skin that should be made the default skin
    ***
    *** If the skin_id does not refer to a valid skin, a warning message will be printed and no change
    *** will occur.
    *** \return Whether the skin could be loaded.
    *** \note This method will <b>not</b> change the skins of any active menu windows.
    **/
    bool SetDefaultMenuSkin(const std::string& skin_id);

    /** \brief Sets the next default menu skin to use from the set of pre-loaded skins
    ***
    *** \note This method will <b>not</b> change the skins of any active menu windows.
    **/
    void SetNextDefaultMenuSkin();

    /** \brief Sets the default menu skin to use from the set of pre-loaded skins
    ***
    *** \note This method will <b>not</b> change the skins of any active menu windows.
    **/
    void SetPreviousDefaultMenuSkin();

    //! \brief Returns the id of the default menu skin.  Returns the empty string if there is no default menu skin.
    std::string GetDefaultMenuSkinId();

    //! \brief Returns the translated name of the user menu skin.
    const vt_utils::ustring& GetDefaultMenuSkinName() const;

    //! \brief Reloads the translated theme names when changing the language.
    void ReloadSkinNames(const std::string& theme_filename);

    /** \brief Returns a pointer to a vector of scroll arrow images.
    ***
    *** The size of this vector is eight. The first four images are the standard arrows and the last
    *** four are greyed out arrows (used to indicate the end of scrolling). The first four arrow
    *** images represent up, down, left, right in that order, and the last four arrows follow this
    *** format as well.
    **/
    std::vector<vt_video::StillImage>* GetScrollArrows() const;

    //! \brief Returns a pointer to current skin cursor image.
    vt_video::StillImage* GetCursor() const;

    //! \brief Returns true if GUI elements should have outlines drawn over their boundaries
    bool DEBUG_DrawOutlines() const {
        return _DEBUG_draw_outlines;
    }

    /** \brief Debug functioning for enabling/disabling the drawing of GUI element boundaries
    *** \param enable Set to true to enable outlines, false to disable
    **/
    void DEBUG_EnableGUIOutlines(bool enable) {
        _DEBUG_draw_outlines = enable;
    }

private:
    /** \brief A map containing all of the menu skins which have been loaded
    *** The string argument is the reference id of the menu, which is defined
    *** by the user when they load a new skin.
    ***
    **/
    std::map<std::string, private_gui::MenuSkin> _menu_skins;

    /** \brief A vector containing all of the actively created MenuWindow objects
    *** The primary purpose of this member is to coordinate menu windows
    *** with menu skins. A menu skin can not be deleted when a menu window is still using that skin, and menu windows
    *** must be re-drawn when the properties of a menu skin that it uses changes.
    **/
    std::vector <MenuWindow *> _menu_windows;

    //! \brief The id of the user menu skin.
    std::string _user_menu_skin;

    /** \brief A pointer to the default menu skin that GUI objects will use if a skin is not explicitly declared
    *** If no menu skins exist, this member will be nullptr. It will never be nullptr as long as one menu skin is loaded.
    *** If the default menu skin is deleted by the user, an alternative default skin will automatically be set.
    **/
    vt_gui::private_gui::MenuSkin* _default_skin;

    /** \brief Draws an outline of the boundary for all GUI elements drawn to the screen when true
    *** The VideoEngine class contains the method that modifies this variable.
    **/
    bool _DEBUG_draw_outlines;

    // ---------- Private methods

    /** \brief Returns a pointer to the MenuSkin of a corresponding skin name
    *** \param skin_id The id of the menu skin to grab
    *** \return A pointer to the MenuSkin, or nullptr if the skin name was not found
    **/
    private_gui::MenuSkin *_GetMenuSkin(const std::string &skin_id);

    //! \brief Returns a pointer to the default menu skin
    private_gui::MenuSkin *_GetDefaultMenuSkin() const {
        return _default_skin;
    }

    /** \brief Adds a newly created MenuWindow into the map of existing windows
    *** \param new_window A pointer to the newly created MenuWindow
    *** Don't call this method anywhere else but from MenuWindow::Create(), or you may cause problems.
    **/
    void _AddMenuWindow(MenuWindow *new_window);

    /** \brief Removes an existing MenuWindow from the map of existing windows
    *** \param old_window A pointer to the MenuWindow to be removed
    *** Don't call this method anywhere else but from MenuWindow::Destroy(), or you may cause problems.
    **/
    void _RemoveMenuWindow(MenuWindow *old_window);
}; // class GUISystem : public vt_utils::Singleton<GUISystem>

} // namespace vt_gui

#endif // __GUI_HEADER__
