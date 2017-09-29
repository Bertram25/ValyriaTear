////////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2017 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
////////////////////////////////////////////////////////////////////////////////

#include "skill_node.h"

namespace vt_global {

SkillNode::SkillNode(uint32_t id,
                     float x_location,
                     float y_location,
                     const std::string& icon_animation_filename,
                     uint32_t experience_points_needed,
                     int32_t skill_id_learned) :
    _id(id),
    _position(x_location, y_location),
    _experience_points_needed(experience_points_needed),
    _skill_id_learned(skill_id_learned)
{
    if(!_icon_image.LoadFromAnimationScript(icon_animation_filename))
        PRINT_WARNING << "Couldn't load animation from script: " << icon_animation_filename << std::endl;
}

void SkillNode::AddNeededItem(uint32_t item_id, uint32_t item_number) {
    if (item_number == 0) {
        PRINT_WARNING << "Couldn't add 0 item number for item id: " << item_id << std::endl;
        return;
    }
    _items_needed.push_back(std::pair<uint32_t, uint32_t>(item_id, item_number));
}

void SkillNode::AddStatUpgrade(uint32_t stat, uint32_t upgrade) {
    if (upgrade == 0) {
        PRINT_WARNING << "Couldn't add 0 upgrade for stat id: " << stat << std::endl;
        return;
    }
    _stats_upgrades.push_back(std::pair<uint32_t, uint32_t>(stat, upgrade));
}

void SkillNode::AddChildNodeLink(uint32_t node_id) {
    // Prevent from double insertion
    for(uint32_t cur_node_id : _children_nodes_links) {
        if (cur_node_id == node_id) {
            PRINT_WARNING << "Node link id: (" << node_id
                          << ") already added: " << std::endl;
            return;
        }
    }
    _children_nodes_links.push_back(node_id);
}

void SkillNode::AddParentNodeLink(uint32_t node_id) {
    // Prevent from double insertion
    for(uint32_t cur_node_id : _parent_nodes_links) {
        if (cur_node_id == node_id) {
            PRINT_WARNING << "Parent node link id: (" << node_id
                          << ") already added: " << std::endl;
            return;
        }
    }
    _parent_nodes_links.push_back(node_id);
}

} // namespace vt_global
