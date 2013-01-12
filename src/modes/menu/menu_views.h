///////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2004-2010 by The Allacrost Project
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
///////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    menu_views.h
*** \author  Daniel Steuernol steu@allacrost.org
*** \author  Andy Gardner chopperdave@allacrost.org
*** \author  Nik Nadig (IkarusDowned) nihonnik@gmail.com
*** \brief   Header file for various menu views.
***
*** This code handles the different menu windows that the user will see while the
*** is in menu mode. These windows are used for displaying inventory lists,
*** character statistics, and various other pieces of information.
*** ***************************************************************************/

#ifndef __MENU_VIEWS__
#define __MENU_VIEWS__

#include "common/global/global.h"
#include "common/gui/textbox.h"

#include "common/gui/menu_window.h"
#include "common/gui/option.h"

namespace hoa_menu
{

namespace private_menu
{

//! \brief The different item categories
enum ITEM_CATEGORY {
    ITEM_ALL = 0,
    ITEM_ITEM = 1,
    ITEM_WEAPONS = 2,
    ITEM_HEAD_ARMOR = 3,
    ITEM_TORSO_ARMOR = 4,
    ITEM_ARM_ARMOR = 5,
    ITEM_LEG_ARMOR = 6,
    ITEM_KEY = 7,
    ITEM_CATEGORY_SIZE = 8
};

//! \brief The different skill types
enum SKILL_CATEGORY {
    SKILL_ALL = 0,
    SKILL_FIELD = 1,
    SKILL_BATTLE = 2,
    SKILL_CATEGORY_SIZE = 3
};

//! \brief The different equipment categories
enum EQUIP_CATEGORY {
    EQUIP_WEAPON = 0,
    EQUIP_HEADGEAR = 1,
    EQUIP_BODYARMOR = 2,
    EQUIP_OFFHAND = 3,
    EQUIP_LEGGINGS = 4,
    EQUIP_CATEGORY_SIZE = 5
};

//! \brief The different option boxes that can be active for items
enum ITEM_ACTIVE_OPTION {
    ITEM_ACTIVE_NONE = 0,
    ITEM_ACTIVE_CATEGORY = 1,
    ITEM_ACTIVE_LIST = 2,
    ITEM_ACTIVE_CHAR = 3,
    ITEM_ACTIVE_SIZE = 4
};

//! \brief The different option boxes that can be active for skills
enum SKILL_ACTIVE_OPTION {
    SKILL_ACTIVE_NONE = 0,
    SKILL_ACTIVE_CHAR = 1,
    SKILL_ACTIVE_CATEGORY = 2,
    SKILL_ACTIVE_LIST = 3,
    SKILL_ACTIVE_CHAR_APPLY = 4,
    SKILL_ACTIVE_SIZE = 5
};

//! \brief The different option boxes that can be active for equipment
enum EQUIP_ACTIVE_OPTION {
    EQUIP_ACTIVE_NONE = 0,
    EQUIP_ACTIVE_CHAR = 1,
    EQUIP_ACTIVE_SELECT = 2,
    EQUIP_ACTIVE_LIST = 3,
    EQUIP_ACTIVE_SIZE = 4
};

//! \brief The different option boxes that can be active for party formation
enum FORM_ACTIVE_OPTION {
    FORM_ACTIVE_NONE = 0,
    FORM_ACTIVE_CHAR = 1,
    FORM_ACTIVE_SECOND = 2,
    FORM_ACTIVE_SIZE = 3
};

//! \brief Possible values from the confirm window
enum CONFIRM_RESULT {
    CONFIRM_RESULT_YES = 0,
    CONFIRM_RESULT_NO = 1,
    CONFIRM_RESULT_NOTHING = 2,
    CONFIRM_RESULT_CANCEL = 3,
};



/** ****************************************************************************
*** \brief Represents an individual character window
***
*** There should be one of these windows for each character in the game.
*** It will contain all the information of the character and handle its draw
*** placement.
*** ***************************************************************************/
class CharacterWindow : public hoa_gui::MenuWindow
{
private:
    //! The name of the character that this window corresponds) to
    uint32 _char_id;

    //! The image of the character
    hoa_video::StillImage _portrait;

public:
    CharacterWindow();

    ~CharacterWindow()
    {}

    /** \brief Set the character for this window
    *** \param character the character to associate with this window
    **/
    void SetCharacter(hoa_global::GlobalCharacter *character);

    /** \brief render this window to the screen
    *** \return success/failure
    **/
    void Draw();
}; // class CharacterWindow : public hoa_video::MenuWindow



/** ****************************************************************************
*** \brief Represents the inventory window to browse the party's inventory
***
*** This handles item use.  You can also view all items by category.
*** ***************************************************************************/
class InventoryWindow : public hoa_gui::MenuWindow
{
    friend class hoa_menu::MenuMode;
    friend class InventoryState;

public:
    InventoryWindow();

    ~InventoryWindow()
    {}

    /** \brief Toggles the inventory window being in the active context for the player
    *** \param new_status Activates the inventory window when true, de-activates it when false
    **/
    void Activate(bool new_status);

    /** \brief Indicates whether the inventory window is in the active context
    *** \return True if the inventory window is in the active context
    **/
    bool IsActive() {
        return _active_box;
    }

    //! If the inventory window is ready to cancel out, or cancel out a sub-window
    //bool CanCancel();

    /*!
    * \brief Updates the inventory window.  Handles key presses, switches window context, etc.
    */
    void Update();

    /*!
    * \brief Draw the inventory window
    * \return success/failure
    */
    void Draw();

private:
    //! Used for char portraits in bottom menu
    std::vector<hoa_video::StillImage> _portraits;

    //! Used for the current dungeon
    hoa_video::StillImage _location_graphic;

    //! Flag to specify the active option box
    uint32 _active_box;

    //! OptionBox to display all of the items
    hoa_gui::OptionBox _inventory_items;

    //! OptionBox to choose character
    hoa_gui::OptionBox _char_select;

    //! OptionBox to choose item category
    hoa_gui::OptionBox _item_categories;

    //! TextBox that holds the selected object's description
    hoa_gui::TextBox _description;

    //! Vector of GlobalObjects that corresponds to _inventory_items
    std::vector< hoa_global::GlobalObject * > _item_objects;

    //! holds previous category. we were looking at
    ITEM_CATEGORY _previous_category;

    /*!
    * \brief Updates the item text in the inventory items
    */
    void _UpdateItemText();

    /*!
    * \brief Initializes inventory items option box
    */
    void _InitInventoryItems();

    /*!
    * \brief Initializes char select option box
    */
    void _InitCharSelect();

    /*!
    * \brief Initializes item category select option box
    */
    void _InitCategory();

    template <class T> std::vector<hoa_global::GlobalObject *> _GetItemVector(std::vector<T *> *inv);

}; // class InventoryWindow : public hoa_video::MenuWindow



/** ****************************************************************************
*** \brief Represents the Party window, displaying all the information about the character.
***
*** This window display all the attributes of the character.
*** You can scroll through them all as well, to view all the different characters.
*** You can also reorder the position of characters
*** ***************************************************************************/
class PartyWindow : public hoa_gui::MenuWindow
{
    friend class hoa_menu::MenuMode;
private:

    //! char portraits
    std::vector<hoa_video::StillImage> _full_portraits;

    //! if the window is active or not
    uint32 _char_select_active;

    //! character selection option box
    hoa_gui::OptionBox _char_select;

    //! The character select option box once first character has been selected
    hoa_gui::OptionBox _second_char_select;

    /*!
    * \brief initialize character selection option box
    */
    void _InitCharSelect();

public:

    PartyWindow();

    ~PartyWindow()
    {}

    /*!
    * \brief render this window to the screen
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief update function handles input to the window
    */
    void Update();

    /*!
    * \brief Get status window active state
    * \return the char select value when active, or zero if inactive
    */
    inline uint32 GetActiveState() {
        return _char_select_active;
    }

    /*!
    * \brief Active this window
    * \param new_value true to activate window, false to deactivate window
    */
    void Activate(bool new_value);

}; // class PartyWindow : public hoa_video::MenuWindow



/** ****************************************************************************
*** \brief Represents the Skills window, displaying all the skills for the character.
***
*** This window display all the skills for a particular character.
*** You can scroll through them all, filter by category, choose one, and apply it
*** to a character.
*** ***************************************************************************/
class SkillsWindow : public hoa_gui::MenuWindow
{
    friend class hoa_menu::MenuMode;
    friend class SkillsState;
public:
    SkillsWindow();

    ~SkillsWindow()
    {}

    /*!
    * \brief Updates key presses and window states
    */
    void Update();

    /*!
    * \brief Draws the windows and option boxes
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief Activates the window
    * \param new_value true to activate window, false to deactivate window
    */
    void Activate(bool new_status);

    /*!
    * \brief Checks to see if the skills window is active
    * \return true if the window is active, false if it's not
    */
    bool IsActive() {
        return _active_box;
    }

private:
    //! Flag to specify the active option box
    uint32 _active_box;

    //! The character select option box
    hoa_gui::OptionBox _char_select;

    //! The skills categories option box
    hoa_gui::OptionBox _skills_categories;

    //! The skills list option box
    hoa_gui::OptionBox _skills_list;

    //! The skill SP cost option box
    hoa_gui::OptionBox _skill_cost_list;

    //! TextBox that holds the selected skill's description
    hoa_gui::TextBox _description;

    //! Track which character's skillset was chosen
    int32 _char_skillset;

    /*!
    * \brief Initializes the skills category chooser
    */
    void _InitSkillsCategories();

    /*!
    * \brief Initializes the skills chooser
    */
    void _InitSkillsList();

    /*!
    * \brief Initializes the character selector
    */
    void _InitCharSelect();

    //! \brief Returns the currently selected skill
    hoa_global::GlobalSkill *_GetCurrentSkill();

    /*!
    * \brief Sets up the skills that comprise the different categories
    */
    void _UpdateSkillList();

    hoa_utils::ustring _BuildSkillListText(const hoa_global::GlobalSkill *skill);

    //! \brief parses the 3 skill lists of the global character and sorts them according to use (menu/battle)
    void _BuildMenuBattleSkillLists(std::vector<hoa_global::GlobalSkill *> *skill_list,
                                    std::vector<hoa_global::GlobalSkill *> *field, std::vector<hoa_global::GlobalSkill *> *battle,
                                    std::vector<hoa_global::GlobalSkill *> *all);

}; //class SkillsWindow : public hoa_video::MenuWindow


/** ****************************************************************************
*** \brief Represents the Equipment window, allowing the player to change equipment.
***
*** This window changes a character's equipment.
*** You can choose a piece of equipment and replace with an item from the given list.
*** ***************************************************************************/
class EquipWindow : public hoa_gui::MenuWindow
{
    friend class hoa_menu::MenuMode;
    friend class InventoryState;
    friend class EquipState;
public:
    EquipWindow();
    ~EquipWindow();

    /*!
    * \brief Draws window
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief Performs updates
    */
    void Update();

    /*!
    * \brief Checks to see if the equipment window is active
    * \return true if the window is active, false if it's not
    */
    bool IsActive() {
        return _active_box;
    }

    /*!
    * \brief Activates the window
    * \param new_value true to activate window, false to deactivate window
    * \param equip Tells Whether the window should be in equip or unequip mode.
    */
    void Activate(bool new_status, bool equip);

private:
    //! \brief Tells whether the window is in equip or unequip mode.
    bool _equip;

    //! Character selector
    hoa_gui::OptionBox _char_select;

    //! Equipment selector
    hoa_gui::OptionBox _equip_select;

    //! Replacement selector
    hoa_gui::OptionBox _equip_list;

    //! \brief the items actual index in the replacor list
    //! Since not all the items are displayed in this list.
    std::vector<uint32> _equip_list_inv_index;

    //! Flag to specify the active option box
    uint32 _active_box;

    //! equipment images
    std::vector<hoa_video::StillImage> _equip_images;

    /*!
    * \brief Set up char selector
    */
    void _InitCharSelect();

    /*!
    * \brief Set up equipment selector
    */
    void _InitEquipmentSelect();

    /*!
    * \brief Set up replacement selector
    */
    void _InitEquipmentList();

    /*!
    * \brief Updates the equipment list
    */
    void _UpdateEquipList();

}; // class EquipWindow : public hoa_video::MenuWindow

/**
*** \brief Represents the quest log list window on the left side
*** this holds the options box "list" that players can cycle through to look at
*** their quests (in Quest Window)
**/

class QuestListWindow : public hoa_gui::MenuWindow {
    friend class hoa_menu::MenuMode;
    friend class QuestState;
    friend class QuestWindow;
public:
    QuestListWindow();
    ~QuestListWindow(){};

    /*!
    * \brief Draws window
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief Performs updates
    */
    void Update();

    /*!
    * \brief Result of whether or not this window is active
    * \return true if this window is active
    */
    bool IsActive()
    {
        return _active_box;
    }

    /*!
    * \brief switch the active state of this window, and do any associated work
    * \param activate or deactivate
    */
    void Activate(bool new_state);


private:

    //! \brief the selectable list of quests
    hoa_gui::OptionBox _quests_list;

    //! The currently active quest log entries.
    std::vector<hoa_global::QuestLogEntry*> _quest_entries;

    //! \brief indicates whether _quests_list is active or not
    bool _active_box;

    //! Setup the quests log list
    void _SetupQuestsList();

    //! \brief updates the side window quest list based on the current quest log entries
    void _UpdateQuestList();
};
/**
*** \brief Represents the quest log main window
*** players can view their active quests as well as completed quests when this window is viewing
**/

class QuestWindow : public hoa_gui::MenuWindow {

    friend class hoa_menu::MenuMode;
    friend class QuestState;

public:

    QuestWindow();
    ~QuestWindow(){}
    /*!
    * \brief Draws window
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief Performs updates
    */
    void Update();

    /*!
    * \brief sets the viewing quest id information for the quest. we use this to query the text description
    */
    void SetViewingQuestId(const std::string &quest_id)
    {
        _viewing_quest_id = quest_id;
    }

private:
    //! \brief the currently viewing quest id. this is set by the Quest List Window through the
    //! SetViewingQuestId() function
    std::string _viewing_quest_id;

    //! \brief sets the display text to be rendered, based on their quest key that is set
    hoa_gui::TextBox _quest_description;

};

/**
*** \brief handles showing the currently set world map
*** upon selection, based on the key press we cycle thru locations that are
*** set as "revealed" on the map
***
*** \note WorldMap has no left window. This means that the entire screen rendering takes place here
*** based on the Wold Map selection here, we update the WorldMapState such that for the
*** bottom window render, we have all the information needed to show
***
**/

class WorldMapWindow : public hoa_gui::MenuWindow
{
    friend class hoa_menu::MenuMode;
    friend class WorldMapState;

    enum WORLDMAP_NAVIGATION {
        WORLDMAP_NOPRESS,   //no key press.
        WORLDMAP_CANCEL,   //a cancel press to exit from viewing the window
        WORLDMAP_LEFT,      //a left press to move "up" the list of locations
        WORLDMAP_RIGHT      //a right press to move "down" the list of locations
    };
public:
    WorldMapWindow();

    ~WorldMapWindow()
    {
        _location_marker.Clear();
        _location_pointer.Clear();
    }

    /*!
    * \brief Draws window
    * \return success/failure
    */
    void Draw();

    /*!
    * \brief Performs updates
    */
    void Update();

     /*!
    * \brief Result of whether or not this window is active
    * \return true if this window is active
    */
    bool IsActive()
    {
        return _active;
    }

    /*!
    * \brief switch the active state of this window, and do any associated work
    * \param activate or deactivate
    */
    void Activate(bool new_state);

    /*!
    * \brief gets the WorldMapLocation pointer to the currently pointing
    * location, or NULL if it deson't exist
    * \return Pointer to the currently indexes WorldMapLocation
    */
    hoa_global::WorldMapLocation *GetCurrentViewingLocation()
    {
        const std::vector<std::string> &current_location_ids = hoa_global::GlobalManager->GetViewableLocationIds();
        const uint32 N = current_location_ids.size();
        if( N == 0 || _location_pointer_index > N)
            return NULL;
        return hoa_global::GlobalManager->GetWorldLocation(current_location_ids[_location_pointer_index]);
    }

private:

    //! \brief based on the worldmap selection, sets the pointer on the
    //! current map
    void _SetSelectedLocation(WORLDMAP_NAVIGATION worldmap_goto);

    //! \brief draws the locations and the pointer based on
    //! the currently active location ids and what we have selected
    //! \param window_position_x The X position of the window
    //! \param window_position_y The Y position of the window
    void _DrawViewableLocations(float window_position_x, float window_position_y);

    //! \brief pointer to the currently loaded world map image
    hoa_video::StillImage *_current_world_map;

    //! \brief the location marker. this is loaded in the ctor
    hoa_video::StillImage _location_marker;

    //! \brief the location pointer. this is loaded in the ctor
    hoa_video::StillImage _location_pointer;

    //! \brief offsets for the current image to view in the center of the window
    float _current_image_x_offset;
    float _current_image_y_offset;

    //! \brief the current index to the location the pointer should be on
    uint32 _location_pointer_index;

    //! \brief indicates whether this window is active or not
    bool _active;

};

/*!
* \brief Converts a vector of GlobalItem*, etc. to a vector of GlobalObjects*
* \return the same vector, with elements of type GlobalObject*
*/
template <class T> std::vector<hoa_global::GlobalObject *> InventoryWindow::_GetItemVector(std::vector<T *>* inv)
{
    std::vector<hoa_global::GlobalObject *> obj_vector;

    for(typename std::vector<T *>::iterator i = inv->begin(); i != inv->end(); i++) {
        obj_vector.push_back(*i);
    }

    return obj_vector;
}

} // namespace private_menu

/** **************************************************************************
*** \brief A window to display a message to the player
*** Displays a message to the user in the center of the screen
*** This class is not private because it's a handy message box and
*** it could be used else where.
*** **************************************************************************/
class MessageWindow : public hoa_gui::MenuWindow
{
public:
    MessageWindow(const hoa_utils::ustring &message, float w, float h);
    ~MessageWindow();

    //! \brief Set the text to display in the window
    void SetText(const hoa_utils::ustring &message) {
        _message = message;
        _textbox.SetDisplayText(message);
    }

    //! \brief Standard Window Functions
    //@{
    void Draw();
    //@}

private:
    //! \brief the message to display
    hoa_utils::ustring _message;

    //! \brief used to display the message
    hoa_gui::TextBox _textbox;
}; // class MessageWindow

} // namespace hoa_menu

#endif
