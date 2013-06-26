////////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2004-2011 by The Allacrost Project
//            Copyright (C) 2012-2013 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
////////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    global_actors.cpp
*** \author  Tyler Olsen, roots@allacrost.org
*** \author  Yohann Ferreira, yohann ferreira orange fr
*** \brief   Source file for global game actors
*** ***************************************************************************/

#include "global_actors.h"

#include "engine/script/script_read.h"
#include "global_objects.h"
#include "global_effects.h"
#include "global_skills.h"

#include "common/global/global.h"

#include "utils/utils_files.h"
#include "utils/utils_random.h"

#include <iostream>

using namespace vt_utils;
using namespace vt_video;
using namespace vt_script;

namespace vt_global
{

extern bool GLOBAL_DEBUG;

////////////////////////////////////////////////////////////////////////////////
// GlobalAttackPoint class
////////////////////////////////////////////////////////////////////////////////

GlobalAttackPoint::GlobalAttackPoint(GlobalActor *owner) :
    _actor_owner(owner),
    _x_position(0),
    _y_position(0),
    _fortitude_modifier(0.0f),
    _protection_modifier(0.0f),
    _evade_modifier(0.0f),
    _total_physical_defense(0),
    _total_evade_rating(0)
{
    for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i)
        _total_magical_defense[i] = 0;
}

bool GlobalAttackPoint::LoadData(ReadScriptDescriptor &script)
{
    if(script.IsFileOpen() == false) {
        return false;
    }

    _name = MakeUnicodeString(script.ReadString("name"));
    _x_position = script.ReadInt("x_position");
    _y_position = script.ReadInt("y_position");
    _fortitude_modifier = script.ReadFloat("fortitude_modifier");
    _protection_modifier = script.ReadFloat("protection_modifier");
    _evade_modifier = script.ReadFloat("evade_modifier");

    // Status effect data is optional so check if a status_effect table exists first
    if(script.DoesTableExist("status_effects") == true) {
        script.OpenTable("status_effects");

        std::vector<int32> table_keys;
        script.ReadTableKeys(table_keys);
        for(uint32 i = 0; i < table_keys.size(); i++) {
            float probability = script.ReadFloat(table_keys[i]);
            _status_effects.push_back(std::make_pair(static_cast<GLOBAL_STATUS>(table_keys[i]), probability));
        }

        script.CloseTable();
    }


    if(script.IsErrorDetected()) {
        if(GLOBAL_DEBUG) {
            PRINT_WARNING << "one or more errors occurred while reading the save game file - they are listed below"
                          << std::endl
                          << script.GetErrorMessages() << std::endl;
        }
        return false;
    }

    return true;
}



void GlobalAttackPoint::CalculateTotalDefense(const GlobalArmor *equipped_armor)
{
    if(_actor_owner == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "attack point has no owning actor" << std::endl;
        return;
    }

    // Calculate defense ratings from owning actor's base stat properties and the attack point modifiers
    if(_fortitude_modifier <= -1.0f)  // If the modifier is less than or equal to -100%, set the total defense to zero
        _total_physical_defense = 0;
    else
        _total_physical_defense = _actor_owner->GetFortitude() + static_cast<int32>(_actor_owner->GetFortitude() * _fortitude_modifier);

    // If the modifier is less than or equal to -100%, set the total defense to zero
    uint32 magical_base = 0;
    if(_protection_modifier > -1.0f)
        magical_base = _actor_owner->GetProtection() + static_cast<int32>(_actor_owner->GetProtection() * _protection_modifier);

    // If present, add defense ratings from the armor equipped
    if(equipped_armor) {
        _total_physical_defense += equipped_armor->GetPhysicalDefense();

        for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i) {
            _total_magical_defense[i] = (magical_base + equipped_armor->GetMagicalDefense())
                                        * _actor_owner->GetElementalModifier((GLOBAL_ELEMENTAL) i);
        }
    }
    else {
        for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i)
            _total_magical_defense[i] = magical_base * _actor_owner->GetElementalModifier((GLOBAL_ELEMENTAL) i);
    }
}



void GlobalAttackPoint::CalculateTotalEvade()
{
    if(_actor_owner == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "attack point has no owning actor" << std::endl;
        return;
    }

    // Calculate evade ratings from owning actor's base evade stat and the evade modifier
    if(_evade_modifier <= -1.0f)  // If the modifier is less than or equal to -100%, set the total evade to zero
        _total_evade_rating = 0.0f;
    else
        _total_evade_rating = _actor_owner->GetEvade() + (_actor_owner->GetEvade() * _evade_modifier);
}

////////////////////////////////////////////////////////////////////////////////
// GlobalActor class
////////////////////////////////////////////////////////////////////////////////

GlobalActor::GlobalActor() :
    _id(0),
    _experience_points(0),
    _hit_points(0),
    _max_hit_points(0),
    _skill_points(0),
    _max_skill_points(0),
    _total_physical_attack(0)
{
    // Init the elemental strength intensity container
    _elemental_modifier.resize(GLOBAL_ELEMENTAL_TOTAL, 1.0f);

    for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i)
        _total_magical_attack[i] = 0;
}


GlobalActor::~GlobalActor()
{
    // Delete all attack points
    for(uint32 i = 0; i < _attack_points.size(); i++) {
        delete _attack_points[i];
    }
    _attack_points.clear();

    // Delete all skills
    for(uint32 i = 0; i < _skills.size(); ++i)
        delete _skills[i];
    //_skills.clear();
    //_skill_ids.clear();
}



GlobalActor::GlobalActor(const GlobalActor &copy)
{
    _id = copy._id;
    _name = copy._name;
    _map_sprite_name = copy._map_sprite_name;
    _portrait = copy._portrait;
    _full_portrait = copy._full_portrait;
    _stamina_icon = copy._stamina_icon;
    _experience_points = copy._experience_points;
    _hit_points = copy._hit_points;
    _max_hit_points = copy._max_hit_points;
    _skill_points = copy._skill_points;
    _max_skill_points = copy._max_skill_points;

    _strength.SetBase(copy._strength.GetBase());
    _strength.SetModifier(copy._strength.GetModifier());
    _vigor.SetBase(copy._vigor.GetBase());
    _vigor.SetModifier(copy._vigor.GetModifier());
    _fortitude.SetBase(copy._fortitude.GetBase());
    _fortitude.SetModifier(copy._fortitude.GetModifier());
    _protection.SetBase(copy._protection.GetBase());
    _protection.SetModifier(copy._protection.GetModifier());
    _agility.SetBase(copy._agility.GetBase());
    _agility.SetModifier(copy._agility.GetModifier());
    _evade.SetBase(copy._evade.GetBase());
    _evade.SetModifier(copy._evade.GetModifier());

    _total_physical_attack = copy._total_physical_attack;
    // init the elemental modifier size to avoid a segfault
    _elemental_modifier.resize(GLOBAL_ELEMENTAL_TOTAL, 1.0f);
    for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i) {
        _total_magical_attack[i] = copy._total_magical_attack[i];
        _elemental_modifier[i] = copy._elemental_modifier[i];
    }

    // Copy all attack points
    for(uint32 i = 0; i < copy._attack_points.size(); i++) {
        _attack_points.push_back(new GlobalAttackPoint(*copy._attack_points[i]));
        _attack_points[i]->SetActorOwner(this);
    }

    // Copy all skills
    for (uint32 i = 0; i < copy._skills.size(); ++i) {
        // Create a new instance as the skill is deleted on an actor object basis.
        _skills.push_back(new GlobalSkill(*copy._skills[i]));
        _skills_id.push_back(copy._skills_id[i]);
    }
}

GlobalActor &GlobalActor::operator=(const GlobalActor &copy)
{
    if(this == &copy)  // Handle self-assignment case
        return *this;

    _id = copy._id;
    _name = copy._name;
    _map_sprite_name = copy._map_sprite_name;
    _portrait = copy._portrait;
    _full_portrait = copy._full_portrait;
    _stamina_icon = copy._stamina_icon;
    _experience_points = copy._experience_points;
    _hit_points = copy._hit_points;
    _max_hit_points = copy._max_hit_points;
    _skill_points = copy._skill_points;
    _max_skill_points = copy._max_skill_points;

    _strength.SetBase(copy._strength.GetBase());
    _strength.SetModifier(copy._strength.GetModifier());
    _vigor.SetBase(copy._vigor.GetBase());
    _vigor.SetModifier(copy._vigor.GetModifier());
    _fortitude.SetBase(copy._fortitude.GetBase());
    _fortitude.SetModifier(copy._fortitude.GetModifier());
    _protection.SetBase(copy._protection.GetBase());
    _protection.SetModifier(copy._protection.GetModifier());
    _agility.SetBase(copy._agility.GetBase());
    _agility.SetModifier(copy._agility.GetModifier());
    _evade.SetBase(copy._evade.GetBase());
    _evade.SetModifier(copy._evade.GetModifier());

    _total_physical_attack = copy._total_physical_attack;
    _elemental_modifier.resize(GLOBAL_ELEMENTAL_TOTAL, 1.0f);
    for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i) {
        _total_magical_attack[i] = copy._total_magical_attack[i];
        _elemental_modifier[i] = copy._elemental_modifier[i];
    }

    // Copy all attack points
    for(uint32 i = 0; i < copy._attack_points.size(); i++) {
        _attack_points.push_back(new GlobalAttackPoint(*_attack_points[i]));
        _attack_points[i]->SetActorOwner(this);
    }

    // Copy all skills
    for (uint32 i = 0; i < copy._skills.size(); ++i) {
        // Create a new instance as the skill is deleted on an actor object basis.
        _skills.push_back(new GlobalSkill(*copy._skills[i]));
        _skills_id.push_back(copy._skills_id[i]);
    }
    return *this;
}

bool GlobalActor::HasSkill(uint32 skill_id)
{
    for (uint32 i = 0; i < _skills_id.size(); ++i) {
        if (_skills_id.at(i) == skill_id)
            return true;
    }
    return false;
}

uint32 GlobalActor::GetTotalPhysicalDefense(uint32 index) const
{
    if(index >= _attack_points.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of attack points: " << index << std::endl;
        return 0;
    }

    return _attack_points[index]->GetTotalPhysicalDefense();
}

uint32 GlobalActor::GetTotalMagicalDefense(uint32 index, GLOBAL_ELEMENTAL element) const
{
    if(index >= _attack_points.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of attack points: " << index << std::endl;
        return 0;
    }

    if (element <= GLOBAL_ELEMENTAL_INVALID || element >= GLOBAL_ELEMENTAL_TOTAL)
        element = GLOBAL_ELEMENTAL_NEUTRAL;

    return _attack_points[index]->GetTotalMagicalDefense(element);
}

float GlobalActor::GetTotalEvadeRating(uint32 index) const
{
    if(index >= _attack_points.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of attack points: " << index << std::endl;
        return 0.0f;
    }

    return _attack_points[index]->GetTotalEvadeRating();
}

uint32 GlobalActor::GetAverageDefense()
{
    if (_attack_points.empty())
        return 0;

    uint32 phys_defense = 0;

    for(uint32 i = 0; i < _attack_points.size(); i++)
        phys_defense += _attack_points[i]->GetTotalPhysicalDefense();
    phys_defense /= _attack_points.size();

    return phys_defense;
}

uint32 GlobalActor::GetAverageMagicalDefense(GLOBAL_ELEMENTAL element)
{
    if (_attack_points.empty())
        return 0;

    uint32 mag_defense = 0;

    for(uint32 i = 0; i < _attack_points.size(); i++)
        mag_defense += _attack_points[i]->GetTotalMagicalDefense(element);
    mag_defense /= _attack_points.size();
    return mag_defense;
}

float GlobalActor::GetAverageEvadeRating()
{
    if (_attack_points.empty())
        return 0;

    float evade = 0.0f;

    for(uint32 i = 0; i < _attack_points.size(); i++)
        evade += _attack_points[i]->GetTotalEvadeRating();
    evade /= static_cast<float>(_attack_points.size());

    return evade;
}

GlobalAttackPoint *GlobalActor::GetAttackPoint(uint32 index) const
{
    if(index >= _attack_points.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of attack points: " << index << std::endl;
        return NULL;
    }

    return _attack_points[index];
}

void GlobalActor::AddHitPoints(uint32 amount)
{
    if((0xFFFFFFFF - amount) < _hit_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "integer overflow condition detected: " << amount << std::endl;
        _hit_points = 0xFFFFFFFF;
    } else {
        _hit_points += amount;
    }

    if(_hit_points > _max_hit_points)
        _hit_points = _max_hit_points;
}



void GlobalActor::SubtractHitPoints(uint32 amount)
{
    if(amount >= _hit_points)
        _hit_points = 0;
    else
        _hit_points -= amount;
}



void GlobalActor::AddMaxHitPoints(uint32 amount)
{
    if((0xFFFFFFFF - amount) < _max_hit_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "integer overflow condition detected: " << amount << std::endl;
        _max_hit_points = 0xFFFFFFFF;
    } else {
        _max_hit_points += amount;
    }
}



void GlobalActor::SubtractMaxHitPoints(uint32 amount)
{
    if(amount > _max_hit_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "argument value will cause max hit points to decrease to zero: " << amount << std::endl;
        _max_hit_points = 0;
        _hit_points = 0;
    } else {
        _max_hit_points -= amount;
        if(_hit_points > _max_hit_points)
            _hit_points = _max_hit_points;
    }
}



void GlobalActor::AddSkillPoints(uint32 amount)
{
    if((0xFFFFFFFF - amount) < _skill_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "integer overflow condition detected: " << amount << std::endl;
        _skill_points = 0xFFFFFFFF;
    } else {
        _skill_points += amount;
    }

    if(_skill_points > _max_skill_points)
        _skill_points = _max_skill_points;
}



void GlobalActor::SubtractSkillPoints(uint32 amount)
{
    if(amount >= _skill_points)
        _skill_points = 0;
    else
        _skill_points -= amount;
}



void GlobalActor::AddMaxSkillPoints(uint32 amount)
{
    if((0xFFFFFFFF - amount) < _max_skill_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "integer overflow condition detected: " << amount << std::endl;
        _max_skill_points = 0xFFFFFFFF;
    } else {
        _max_skill_points += amount;
    }
}



void GlobalActor::SubtractMaxSkillPoints(uint32 amount)
{
    if(amount > _max_skill_points) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "argument value will cause max skill points to decrease to zero: " << amount << std::endl;
        _max_skill_points = 0;
        _skill_points = 0;
    } else {
        _max_skill_points -= amount;
        if(_skill_points > _max_skill_points)
            _skill_points = _max_skill_points;
    }
}



void GlobalActor::AddStrength(uint32 amount)
{
    _strength.SetBase(_strength.GetBase() + (float)amount);
    _CalculateAttackRatings();
}



void GlobalActor::SubtractStrength(uint32 amount)
{
    float new_base = _strength.GetBase() - (float)amount;
    _strength.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateAttackRatings();
}



void GlobalActor::AddVigor(uint32 amount)
{
    _vigor.SetBase(_vigor.GetBase() + (float)amount);
    _CalculateAttackRatings();
}



void GlobalActor::SubtractVigor(uint32 amount)
{
    float new_base = _vigor.GetBase() - (float)amount;
    _vigor.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateAttackRatings();
}



void GlobalActor::AddFortitude(uint32 amount)
{
    _fortitude.SetBase(_fortitude.GetBase() + (float)amount);
    _CalculateDefenseRatings();
}



void GlobalActor::SubtractFortitude(uint32 amount)
{
    float new_base = _fortitude.GetBase() - (float)amount;
    _fortitude.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateDefenseRatings();
}



void GlobalActor::AddProtection(uint32 amount)
{
    _protection.SetBase(_protection.GetBase() + (float)amount);
    _CalculateDefenseRatings();
}



void GlobalActor::SubtractProtection(uint32 amount)
{
    float new_base = _protection.GetBase() - (float)amount;
    _protection.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateDefenseRatings();
}



void GlobalActor::AddAgility(uint32 amount)
{
    _agility.SetBase(_agility.GetBase() + (float)amount);
}



void GlobalActor::SubtractAgility(uint32 amount)
{
    float new_base = _agility.GetBase() - (float)amount;
    _agility.SetBase(new_base < 0.0f ? 0.0f : new_base);
}



void GlobalActor::AddEvade(float amount)
{
    float new_base = _evade.GetBase() + amount;
    _evade.SetBase(new_base > 1.0f ? 1.0f : new_base);
    _CalculateEvadeRatings();
}



void GlobalActor::SubtractEvade(float amount)
{
    float new_base = _evade.GetBase() - amount;
    _evade.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateEvadeRatings();
}



void GlobalActor::_CalculateAttackRatings()
{
    _total_physical_attack = _strength.GetValue();
    for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i)
        _total_magical_attack[i] = _vigor.GetValue() * _elemental_modifier[i];
}



void GlobalActor::_CalculateDefenseRatings()
{
    // Re-calculate the defense ratings for all attack points
    for(uint32 i = 0; i < _attack_points.size(); i++)
        _attack_points[i]->CalculateTotalDefense(NULL);
}



void GlobalActor::_CalculateEvadeRatings()
{
    // Re-calculate the evade ratings for all attack points
    for(uint32 i = 0; i < _attack_points.size(); i++) {
        _attack_points[i]->CalculateTotalEvade();
    }
}

////////////////////////////////////////////////////////////////////////////////
// GlobalCharacter class
////////////////////////////////////////////////////////////////////////////////

GlobalCharacter::GlobalCharacter(uint32 id, bool initial) :
    _experience_level(0),
    _enabled(true),
    _weapon_equipped(NULL),
    _experience_for_next_level(0),
    _hit_points_growth(0),
    _skill_points_growth(0),
    _strength_growth(0),
    _vigor_growth(0),
    _fortitude_growth(0),
    _protection_growth(0),
    _agility_growth(0),
    _evade_growth(0.0f)
{
    _id = id;

    // Open the characters script file
    std::string filename = "dat/actors/characters.lua";
    ReadScriptDescriptor char_script;
    if(!char_script.OpenFile(filename)) {
        PRINT_ERROR << "failed to open character data file: "
                    << filename << std::endl;
        return;
    }

    // Retrieve their basic character property data
    char_script.OpenTable("characters");
    char_script.OpenTable(_id);
    _name = MakeUnicodeString(char_script.ReadString("name"));

    // Load all the graphic data
    std::string portrait_filename = char_script.ReadString("portrait");
    if(DoesFileExist(portrait_filename)) {
        _portrait.Load(portrait_filename);
    }
    else if(!portrait_filename.empty()) {
        PRINT_WARNING << "Unavailable portrait image: " << portrait_filename
                      << " for character: " << MakeStandardString(_name) << std::endl;
    }

    std::string full_portrait_filename = char_script.ReadString("full_portrait");
    if(DoesFileExist(full_portrait_filename)) {
        _full_portrait.Load(full_portrait_filename);
    }
    else if(!full_portrait_filename.empty()) {
        PRINT_WARNING << "Unavailable full portrait image: " << full_portrait_filename
                      << " for character: " << MakeStandardString(_name) << std::endl;
    }

    std::string stamina_icon_filename = char_script.ReadString("stamina_icon");
    bool stamina_icon_loaded = false;
    if(DoesFileExist(stamina_icon_filename)) {
        if(_stamina_icon.Load(stamina_icon_filename, 45.0f, 45.0f))
            stamina_icon_loaded = true;
    } else {
        // Don't complain if no icon was provided on purpose
        if(!stamina_icon_filename.empty()) {
            PRINT_WARNING << "Unavailable stamina icon image: " << stamina_icon_filename
                          << " for character: " << MakeStandardString(_name) << ". Loading default one." << std::endl;
        }
    }

    // Load default in case of failure
    if(!stamina_icon_loaded)
        _stamina_icon.Load("img/icons/actors/default_stamina_icon.png", 45.0f, 45.0f);

    // Load the character's battle portraits from a multi image
    _battle_portraits.assign(5, StillImage());
    for(uint32 i = 0; i < _battle_portraits.size(); i++) {
        _battle_portraits[i].SetDimensions(100.0f, 100.0f);
    }
    std::string battle_portraits_filename = char_script.ReadString("battle_portraits");
    if(battle_portraits_filename.empty() ||
            !ImageDescriptor::LoadMultiImageFromElementGrid(_battle_portraits,
                    battle_portraits_filename, 1, 5)) {
        // Load empty portraits when they don't exist.
        for(uint32 i = 0; i < _battle_portraits.size(); ++i) {
            _battle_portraits[i].Clear();
            _battle_portraits[i].Load("", 1.0f, 1.0f);
        }
    }

    // Set up the map sprite name (untranslated) used as a string id to later link it with a map sprite.
    _map_sprite_name = char_script.ReadString("map_sprite_name");

    // Load the special skills category name and icon
    _special_category_name = MakeUnicodeString(char_script.ReadString("special_skill_category_name"));
    _special_category_icon = char_script.ReadString("special_skill_category_icon");

    // Load the bare hand skills available
    if (char_script.DoesTableExist("bare_hands_skills")) {
        std::vector<uint32> bare_skills;
        char_script.ReadUIntVector("bare_hands_skills", bare_skills);
        for (uint32 i = 0; i < bare_skills.size(); ++i)
            AddSkill(bare_skills[i]);
    }

    // Read each battle_animations table keys and store the corresponding animation in memory.
    std::vector<std::string> keys_vect;
    char_script.ReadTableKeys("battle_animations", keys_vect);
    char_script.OpenTable("battle_animations");
    for(uint32 i = 0; i < keys_vect.size(); ++i) {
        AnimatedImage animation;
        animation.LoadFromAnimationScript(char_script.ReadString(keys_vect[i]));
        _battle_animation[keys_vect[i]] = animation;
    }
    char_script.CloseTable();

    // Construct the character from the initial stats if necessary
    if(initial) {
        char_script.OpenTable("initial_stats");
        _experience_level = char_script.ReadUInt("experience_level");
        _experience_points = char_script.ReadUInt("experience_points");
        _max_hit_points = char_script.ReadUInt("max_hit_points");
        _hit_points = _max_hit_points;
        _max_skill_points = char_script.ReadUInt("max_skill_points");
        _skill_points = _max_skill_points;
        _strength.SetBase(char_script.ReadUInt("strength"));
        _vigor.SetBase(char_script.ReadUInt("vigor"));
        _fortitude.SetBase(char_script.ReadUInt("fortitude"));
        _protection.SetBase(char_script.ReadUInt("protection"));
        _agility.SetBase(char_script.ReadUInt("agility"));
        _evade.SetBase(char_script.ReadFloat("evade"));

        // Add the character's initial equipment. If any equipment ids are zero, that indicates nothing is to be equipped.
        uint32 equipment_id = 0;
        equipment_id = char_script.ReadUInt("weapon");
        if(equipment_id != 0)
            _weapon_equipped = new GlobalWeapon(equipment_id);
        else
            _weapon_equipped = NULL;

        equipment_id = char_script.ReadUInt("head_armor");
        if(equipment_id != 0)
            _armor_equipped.push_back(new GlobalArmor(equipment_id));
        else
            _armor_equipped.push_back(NULL);

        equipment_id = char_script.ReadUInt("torso_armor");
        if(equipment_id != 0)
            _armor_equipped.push_back(new GlobalArmor(equipment_id));
        else
            _armor_equipped.push_back(NULL);

        equipment_id = char_script.ReadUInt("arm_armor");
        if(equipment_id != 0)
            _armor_equipped.push_back(new GlobalArmor(equipment_id));
        else
            _armor_equipped.push_back(NULL);

        equipment_id = char_script.ReadUInt("leg_armor");
        if(equipment_id != 0)
            _armor_equipped.push_back(new GlobalArmor(equipment_id));
        else
            _armor_equipped.push_back(NULL);

        char_script.CloseTable();
        if(char_script.IsErrorDetected()) {
            if(GLOBAL_DEBUG) {
                PRINT_WARNING << "one or more errors occurred while reading initial data - they are listed below"
                              << std::endl
                              << char_script.GetErrorMessages() << std::endl;
            }
        }
    } // if (initial == true)
    else {
        // Make sure the _armor_equipped vector is sized appropriately. Armor should be equipped on the character
        // externally to this constructor.
        _armor_equipped.resize(4, NULL);
    }

    // Setup the character's attack points
    char_script.OpenTable("attack_points");
    for(uint32 i = GLOBAL_POSITION_HEAD; i <= GLOBAL_POSITION_LEGS; i++) {
        _attack_points.push_back(new GlobalAttackPoint(this));
        char_script.OpenTable(i);
        if(_attack_points[i]->LoadData(char_script) == false) {
            IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to succesfully load data for attack point: " << i << std::endl;
        }
        char_script.CloseTable();
    }
    char_script.CloseTable();

    if(char_script.IsErrorDetected()) {
        if(GLOBAL_DEBUG) {
            PRINT_WARNING << "one or more errors occurred while reading attack point data - they are listed below"
                          << std::endl << char_script.GetErrorMessages() << std::endl;
        }
    }

    // Construct the character's initial skill set if necessary
    if(initial) {
        // The skills table contains key/value pairs. The key indicate the level required to learn the skill and the value is the skill's id
        std::vector<uint32> skill_levels;
        char_script.OpenTable("skills");
        char_script.ReadTableKeys(skill_levels);

        // We want to add the skills beginning with the first learned to the last. ReadTableKeys does not guarantee returing the keys in a sorted order,
        // so sort the skills by level before checking each one.
        std::sort(skill_levels.begin(), skill_levels.end());

        // Only add the skills for which the experience level requirements are met
        for(uint32 i = 0; i < skill_levels.size(); i++) {
            if(skill_levels[i] <= _experience_level) {
                AddSkill(char_script.ReadUInt(skill_levels[i]));
            }
            // Because skill_levels is sorted, all remaining skills will not have their level requirements met
            else {
                break;
            }
        }

        char_script.CloseTable(); // skills
        if(char_script.IsErrorDetected()) {
            if(GLOBAL_DEBUG) {
                PRINT_WARNING << "one or more errors occurred while reading skill data - they are listed below"
                              << std::endl << char_script.GetErrorMessages() << std::endl;
            }
        }

        // If initial, determine the character's XP for next level.
        std::vector<int32> xp_per_levels;
        char_script.OpenTable("growth");
        char_script.ReadIntVector("experience_for_next_level", xp_per_levels);
        if (_experience_level <= xp_per_levels.size()) {
            _experience_for_next_level = xp_per_levels[_experience_level - 1];
        }
        else {
            PRINT_ERROR << "No Xp for next level found for character id " << _id
                << " at level " << _experience_level << std::endl;
            // Bad default
            _experience_for_next_level = 100000;
        }
        char_script.CloseTable(); // growth
    } // if (initial)

    // Reloads available skill according to equipment
    _UpdatesAvailableSkills();

    char_script.CloseTable(); // "characters[id]"
    char_script.CloseTable(); // "characters"

    // Close the script file and calculate all rating totals
    if(char_script.IsErrorDetected()) {
        if(GLOBAL_DEBUG) {
            PRINT_WARNING << "one or more errors occurred while reading final data - they are listed below"
                          << std::endl << char_script.GetErrorMessages() << std::endl;
        }
    }
    char_script.CloseFile();

    // Init and updates the status effects according to current equipment.
    _equipment_status_effects.resize(GLOBAL_STATUS_TOTAL, GLOBAL_INTENSITY_NEUTRAL);
    _UpdateEquipmentStatusEffects();

    _CalculateAttackRatings();
    _CalculateDefenseRatings();
    _CalculateEvadeRatings();
} // GlobalCharacter::GlobalCharacter(uint32 id, bool initial)

GlobalCharacter::~GlobalCharacter()
{
    // Delete all equipment
    if(_weapon_equipped != NULL)
        delete _weapon_equipped;
    for(uint32 i = 0; i < _armor_equipped.size(); i++) {
        if(_armor_equipped[i] != NULL)
            delete _armor_equipped[i];
    }
    _armor_equipped.clear();
}


bool GlobalCharacter::AddExperiencePoints(uint32 xp)
{
    _experience_points += xp;
    _experience_for_next_level -= xp;
    return ReachedNewExperienceLevel();
}

void GlobalCharacter::AddStrength(uint32 amount)
{
    _strength.SetBase(_strength.GetBase() + (float)amount);
    _CalculateAttackRatings();
}

void GlobalCharacter::SubtractStrength(uint32 amount)
{
    float new_base = _strength.GetBase() - (float)amount;
    _strength.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateAttackRatings();
}

void GlobalCharacter::AddVigor(uint32 amount)
{
    _vigor.SetBase(_vigor.GetBase() + (float)amount);
    _CalculateAttackRatings();
}

void GlobalCharacter::SubtractVigor(uint32 amount)
{
    float new_base = _vigor.GetBase() - (float)amount;
    _vigor.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateAttackRatings();
}

void GlobalCharacter::AddFortitude(uint32 amount)
{
    _fortitude.SetBase(_fortitude.GetBase() + (float)amount);
    _CalculateDefenseRatings();
}

void GlobalCharacter::SubtractFortitude(uint32 amount)
{
    float new_base = _fortitude.GetBase() - (float)amount;
    _fortitude.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateDefenseRatings();
}

void GlobalCharacter::AddProtection(uint32 amount)
{
    _protection.SetBase(_protection.GetBase() + (float)amount);
    _CalculateDefenseRatings();
}

void GlobalCharacter::SubtractProtection(uint32 amount)
{
    float new_base = _protection.GetBase() - (float)amount;
    _protection.SetBase(new_base < 0.0f ? 0.0f : new_base);
    _CalculateDefenseRatings();
}

GlobalWeapon *GlobalCharacter::EquipWeapon(GlobalWeapon *weapon)
{
    GlobalWeapon *old_weapon = _weapon_equipped;
    _weapon_equipped = weapon;

    // Updates the equipment status effects first
    _UpdateEquipmentStatusEffects();

    _CalculateAttackRatings();
    _UpdatesAvailableSkills();

    return old_weapon;
}

GlobalArmor *GlobalCharacter::_EquipArmor(GlobalArmor *armor, uint32 index)
{
    if(index >= _armor_equipped.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of pieces of armor equipped: " << index << std::endl;
        return armor;
    }

    GlobalArmor *old_armor = _armor_equipped[index];
    _armor_equipped[index] = armor;

    if(old_armor != NULL && armor != NULL) {
        if(old_armor->GetObjectType() != armor->GetObjectType()) {
            IF_PRINT_WARNING(GLOBAL_DEBUG) << "old armor was replaced with a different type of armor" << std::endl;
        }
    }

    // Updates the equipment status effect first
    _UpdateEquipmentStatusEffects();
    // This is a subset of _CalculateDefenseRatings(), but just for the given armor.
    _attack_points[index]->CalculateTotalDefense(_armor_equipped[index]);

    // Reloads available skill according to equipment
    _UpdatesAvailableSkills();

    return old_armor;
}

GlobalArmor *GlobalCharacter::GetArmorEquipped(uint32 index) const
{
    if(index >= _armor_equipped.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded number of pieces of armor equipped: " << index << std::endl;
        return NULL;
    }

    return _armor_equipped[index];
}

bool GlobalCharacter::HasEquipment() const
{
    if (_weapon_equipped)
        return true;

    for (uint32 i = 0; i < _armor_equipped.size(); ++i) {
        if (_armor_equipped.at(i) != NULL)
            return true;
    }
    return false;
}

void GlobalCharacter::_UpdateEquipmentStatusEffects()
{
    // Reset the status effect intensities
    for (uint32 i = 0; i < _equipment_status_effects.size(); ++i)
        _equipment_status_effects[i] = GLOBAL_INTENSITY_NEUTRAL;

    // For each piece of equipment, we add the intensity of the given status
    // effect on the status effect cache
    if (_weapon_equipped) {
        const std::vector<std::pair<GLOBAL_STATUS, GLOBAL_INTENSITY> >& _effects = _weapon_equipped->GetStatusEffects();

        for (uint32 i = 0; i < _effects.size(); ++i) {

            GLOBAL_STATUS effect = _effects[i].first;
            GLOBAL_INTENSITY intensity = _effects[i].second;

            // Check bounds and update the intensity
            if (_equipment_status_effects[effect] + intensity > GLOBAL_INTENSITY_POS_EXTREME)
                _equipment_status_effects[effect] = GLOBAL_INTENSITY_POS_EXTREME;
            else if (_equipment_status_effects[effect] + intensity < GLOBAL_INTENSITY_NEG_EXTREME)
                _equipment_status_effects[effect] = GLOBAL_INTENSITY_NEG_EXTREME;
            else
                _equipment_status_effects[effect] = (GLOBAL_INTENSITY)(_equipment_status_effects[effect] + intensity);
        }
    }

    // armors
    for (uint32 i = 0; i < _armor_equipped.size(); ++i) {
        if (!_armor_equipped[i])
            continue;

        const std::vector<std::pair<GLOBAL_STATUS, GLOBAL_INTENSITY> >& _effects = _armor_equipped[i]->GetStatusEffects();

        for (uint32 j = 0; j < _effects.size(); ++j) {

            GLOBAL_STATUS effect = _effects[j].first;
            GLOBAL_INTENSITY intensity = _effects[j].second;

            // Check bounds and update the intensity
            if (_equipment_status_effects[effect] + intensity > GLOBAL_INTENSITY_POS_EXTREME)
                _equipment_status_effects[effect] = GLOBAL_INTENSITY_POS_EXTREME;
            else if (_equipment_status_effects[effect] + intensity < GLOBAL_INTENSITY_NEG_EXTREME)
                _equipment_status_effects[effect] = GLOBAL_INTENSITY_NEG_EXTREME;
            else
                _equipment_status_effects[effect] = (GLOBAL_INTENSITY)(_equipment_status_effects[effect] + intensity);
        }
    }

    // Actually apply the effects on the character now
    ReadScriptDescriptor &script_file = vt_global::GlobalManager->GetStatusEffectsScript();
    for (uint32 i = 0; i < _equipment_status_effects.size(); ++i) {
        GLOBAL_INTENSITY intensity = _equipment_status_effects[i];

        if (!script_file.OpenTable(i)) {
            PRINT_WARNING << "No status effect defined for this status value: " << i << std::endl;
            continue;
        }

        if (intensity == GLOBAL_INTENSITY_NEUTRAL) {

            // Call RemovePassive(global_actor)
            if(!script_file.DoesFunctionExist("RemovePassive")) {
                PRINT_WARNING << "No RemovePassive() function found in Lua definition file for status: " << i << std::endl;
                script_file.CloseTable(); // effect id
                continue;
            }

            ScriptObject remove_passive_function = script_file.ReadFunctionPointer("RemovePassive");
            script_file.CloseTable(); // effect id

            if (!remove_passive_function.is_valid()) {
                PRINT_WARNING << "Invalid RemovePassive() function found in Lua definition file for status: " << i << std::endl;
                continue;
            }

            try {
                ScriptCallFunction<void>(remove_passive_function, this);
            } catch(const luabind::error &e) {
                PRINT_ERROR << "Error while loading status effect RemovePassive() function" << std::endl;
                ScriptManager->HandleLuaError(e);
            } catch(const luabind::cast_failed &e) {
                PRINT_ERROR << "Error while loading status effect RemovePassive() function" << std::endl;
                ScriptManager->HandleCastError(e);
            }
        }
        else {
            // Call ApplyPassive(global_actor, intensity)
            if(!script_file.DoesFunctionExist("ApplyPassive")) {
                PRINT_WARNING << "No ApplyPassive() function found in Lua definition file for status: " << i << std::endl;
                script_file.CloseTable(); // effect id
                continue;
            }

            ScriptObject apply_passive_function = script_file.ReadFunctionPointer("ApplyPassive");
            script_file.CloseTable(); // effect id

            if (!apply_passive_function.is_valid()) {
                PRINT_WARNING << "Invalid ApplyPassive() function found in Lua definition file for status: " << i << std::endl;
                continue;
            }

            try {
                ScriptCallFunction<void>(apply_passive_function, this, intensity);
            } catch(const luabind::error &e) {
                PRINT_ERROR << "Error while loading status effect ApplyPassive() function" << std::endl;
                ScriptManager->HandleLuaError(e);
            } catch(const luabind::cast_failed &e) {
                PRINT_ERROR << "Error while loading status effect ApplyPassive() function" << std::endl;
                ScriptManager->HandleCastError(e);
            }
        } // Call function depending on intensity
    } // For each equipment status effect
}

bool GlobalCharacter::AddSkill(uint32 skill_id, bool permanently)
{
    if(skill_id == 0) {
        PRINT_WARNING << "function received an invalid skill_id argument: " << skill_id << std::endl;
        return false;
    }

    GlobalSkill *skill = new GlobalSkill(skill_id);
    if(!skill->IsValid()) {
        PRINT_WARNING << "the skill to add failed to load: " << skill_id << std::endl;
        delete skill;
        return false;
    }

    if(HasSkill(skill_id)) {
        //Test whether the skill should become permanent
        if (permanently) {
            bool found = false;
            for (uint32 i = 0; i < _permanent_skills.size(); ++i) {
                if (_permanent_skills[i] == skill_id) {
                    found = true;
                    i = _permanent_skills.size();
                }
            }

            // if the skill wasn't permanent, it will then become one.
            if (!found)
                _permanent_skills.push_back(skill->GetID());
        }

        // The character already knew the skill but that doesn't really matter.
        delete skill;
        return true;
    }

    // Insert the pointer to the new skill inside of the global skills map and the skill type vector
    switch(skill->GetType()) {
    case GLOBAL_SKILL_WEAPON:
        _weapon_skills.push_back(skill);
        break;
    case GLOBAL_SKILL_MAGIC:
        _magic_skills.push_back(skill);
        break;
    case GLOBAL_SKILL_SPECIAL:
        _special_skills.push_back(skill);
        break;
    case GLOBAL_SKILL_BARE_HANDS:
        _bare_hands_skills.push_back(skill);
        break;
    default:
        PRINT_WARNING << "loaded a new skill with an unknown skill type: " << skill->GetType() << std::endl;
        return false;
        break;
    }

    _skills_id.push_back(skill_id);
    _skills.push_back(skill);
    if (permanently)
        _permanent_skills.push_back(skill->GetID());

    return true;
}

bool GlobalCharacter::AddNewSkillLearned(uint32 skill_id)
{
    if(skill_id == 0) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function received an invalid skill_id argument: " << skill_id << std::endl;
        return false;
    }

    // Make sure we don't add a skill more than once
    for(std::vector<GlobalSkill*>::iterator it = _new_skills_learned.begin(); it != _new_skills_learned.end(); ++it) {
        if(skill_id == (*it)->GetID()) {
            IF_PRINT_WARNING(GLOBAL_DEBUG) << "the skill to add was already present in the list of newly learned skills: "
                                           << skill_id << std::endl;
            return false;
        }
    }

    if (!AddSkill(skill_id))
        PRINT_WARNING << "Failed because the new skill was not added successfully: " << skill_id << std::endl;
    else
        _new_skills_learned.push_back(_skills.back());

    return true;
}

void GlobalCharacter::_UpdatesAvailableSkills()
{
    // Clears out the skills <and parse the current equipment to tells which ones are available.
    for (uint32 i = 0; i < _skills.size(); ++i) {
        delete _skills[i];
    }
    _skills.clear();
    _skills_id.clear();

    _bare_hands_skills.clear();
    _weapon_skills.clear();
    _magic_skills.clear();
    _special_skills.clear();

    // First readd the permanent ones
    for (uint32 i = 0; i < _permanent_skills.size(); ++i) {
        // As the skill is already permanent, don't readd it as one.
        AddSkill(_permanent_skills[i], false);
    }

    // Now, add skill obtained through current equipment.
    if (_weapon_equipped) {
        const std::vector<uint32>& wpn_skills = _weapon_equipped->GetEquipmentSkills();

        for (uint32 i = 0; i < wpn_skills.size(); ++i)
            AddSkill(wpn_skills[i], false);
    }

    for (uint32 i = 0; i < _armor_equipped.size(); ++i) {
        if (!_armor_equipped[i])
            continue;

        const std::vector<uint32>& armor_skills = _armor_equipped[i]->GetEquipmentSkills();

        for (uint32 j = 0; j < armor_skills.size(); ++j)
            AddSkill(armor_skills[j], false);
    }
}

vt_video::AnimatedImage *GlobalCharacter::RetrieveBattleAnimation(const std::string &name)
{
    if(_battle_animation.find(name) == _battle_animation.end())
        return &_battle_animation["idle"];

    return &_battle_animation.at(name);
}

void GlobalCharacter::AcknowledgeGrowth() {
    if (!ReachedNewExperienceLevel())
        return;

    // A new experience level has been gained. Retrieve the growth data for the new experience level
    ++_experience_level;

    // Retrieve the growth data for the new experience level and check for any additional growth
    std::string filename = "dat/actors/characters.lua";
    ReadScriptDescriptor character_script;
    if(!character_script.OpenFile(filename)) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to open character data file: " << filename << std::endl;
        return;
    }

    // Clear the growth members before filling their data
    _hit_points_growth = 0;
    _skill_points_growth = 0;
    _strength_growth = 0;
    _vigor_growth = 0;
    _fortitude_growth = 0;
    _protection_growth = 0;
    _agility_growth = 0;
    _evade_growth = 0.0f;

    try {
        // Update Growth data and set XP for next level
        ScriptCallFunction<void>(character_script.GetLuaState(), "DetermineLevelGrowth", this);
    } catch(const luabind::error& e) {
        ScriptManager->HandleLuaError(e);
    } catch(const luabind::cast_failed& e) {
        ScriptManager->HandleCastError(e);
    }

    // Reset the skills learned container and add any skills learned at this level
    _new_skills_learned.clear();
    try {
        ScriptCallFunction<void>(character_script.GetLuaState(), "DetermineNewSkillsLearned", this);
    } catch(const luabind::error& e) {
        ScriptManager->HandleLuaError(e);
    } catch(const luabind::cast_failed& e) {
        ScriptManager->HandleCastError(e);
    }

    // Add all growth stats to the character actor
    if(_hit_points_growth != 0) {
        AddMaxHitPoints(_hit_points_growth);
        if (_hit_points > 0)
            AddHitPoints(_hit_points_growth);
    }

    if(_skill_points_growth != 0) {
        AddMaxSkillPoints(_skill_points_growth);
        if (_skill_points > 0)
            AddSkillPoints(_skill_points_growth);
    }

    if(_strength_growth != 0)
        AddStrength(_strength_growth);
    if(_vigor_growth != 0)
        AddVigor(_vigor_growth);
    if(_fortitude_growth != 0)
        AddFortitude(_fortitude_growth);
    if(_protection_growth != 0)
        AddProtection(_protection_growth);
    if(_agility_growth != 0)
        AddAgility(_agility_growth);
    if(!IsFloatEqual(_evade_growth, 0.0f))
        AddEvade(_evade_growth);

    character_script.CloseFile();
    return;
} // bool GlobalCharacter::AcknowledgeGrowth()

void GlobalCharacter::_CalculateAttackRatings()
{
    _total_physical_attack = _strength.GetValue();

    if(_weapon_equipped) {
        _total_physical_attack += _weapon_equipped->GetPhysicalAttack();
        for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i) {
            _total_magical_attack[i] = (_vigor.GetValue() + _weapon_equipped->GetMagicalAttack())
                                       * GetElementalModifier((GLOBAL_ELEMENTAL) i);
        }
    }
    else {
        for (uint32 i = 0; i < GLOBAL_ELEMENTAL_TOTAL; ++i) {
            _total_magical_attack[i] = _vigor.GetValue() * GetElementalModifier((GLOBAL_ELEMENTAL) i);
        }
    }
}

void GlobalCharacter::_CalculateDefenseRatings()
{
    // Re-calculate the defense ratings for all attack points
    for(uint32 i = 0; i < _attack_points.size(); i++) {
        if((i < _armor_equipped.size()) && (_armor_equipped[i] != NULL))
            _attack_points[i]->CalculateTotalDefense(_armor_equipped[i]);
        else
            _attack_points[i]->CalculateTotalDefense(NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////
// GlobalEnemy class
////////////////////////////////////////////////////////////////////////////////

GlobalEnemy::GlobalEnemy(uint32 id) :
    GlobalActor(),
    _no_stat_randomization(false),
    _sprite_width(0),
    _sprite_height(0),
    _drunes_dropped(0)
{
    _id = id;

    // Use the id member to determine the name of the data file that the enemy is defined in
    std::string file_ext;
    std::string filename;

    if(_id == 0)
        PRINT_ERROR << "invalid id for loading enemy data: " << _id << std::endl;

    // Open the script file and table that store the enemy data
    ReadScriptDescriptor enemy_data;
    if(!enemy_data.OpenFile("dat/actors/enemies.lua")) {
        PRINT_ERROR << "failed to open enemy data file: " << filename << std::endl;
        return;
    }

    if (!enemy_data.OpenTable("enemies") || !enemy_data.OpenTable(_id)) {
        enemy_data.CloseFile();
        PRINT_ERROR << "Failed to open the enemies[" << _id << "] table in "
            << filename << std::endl;
    }

    // Load the enemy's name and sprite data
    _name = MakeUnicodeString(enemy_data.ReadString("name"));

    // Attempt to load the animations for each harm levels
    _battle_animations.assign(GLOBAL_ENEMY_HURT_TOTAL, AnimatedImage());
    if (enemy_data.OpenTable("battle_animations" )) {
        std::vector<uint32> animations_id;
        std::vector<std::string> animations;
        enemy_data.ReadTableKeys(animations_id);
        for (uint32 i = 0; i < animations_id.size(); ++i) {
            uint32 anim_id = animations_id[i];
            if (anim_id >= GLOBAL_ENEMY_HURT_TOTAL) {
                PRINT_WARNING << "Invalid table id in 'battle_animations' table for enemy: "
                    << _id << std::endl;
                continue;
            }

            _battle_animations[anim_id].LoadFromAnimationScript(enemy_data.ReadString(anim_id));

            // Updates the sprite dimensions
            if (_battle_animations[anim_id].GetWidth() > _sprite_width)
                _sprite_width =_battle_animations[anim_id].GetWidth();
            if (_battle_animations[anim_id].GetHeight() > _sprite_height)
                _sprite_height =_battle_animations[anim_id].GetHeight();
        }

        enemy_data.CloseTable();
    }
    else {
        PRINT_WARNING << "No 'battle_animations' table for enemy: " << _id << std::endl;
    }

    std::string stamina_icon_filename = enemy_data.ReadString("stamina_icon");
    if(!stamina_icon_filename.empty()) {
        if(!_stamina_icon.Load(stamina_icon_filename)) {
            PRINT_WARNING << "Invalid stamina icon image: " << stamina_icon_filename
                          << " for enemy: " << MakeStandardString(_name) << ". Loading default one." << std::endl;

            _stamina_icon.Load("img/icons/actors/default_stamina_icon.png");
        }
    } else {
        _stamina_icon.Load("img/icons/actors/default_stamina_icon.png");
    }

    // Load the enemy's base stats
    if(enemy_data.DoesBoolExist("no_stat_randomization") == true) {
        _no_stat_randomization = enemy_data.ReadBool("no_stat_randomization");
    }

    // Loads enemy battle animation scripts
    if (enemy_data.OpenTable("scripts")) {
        _death_script_filename = enemy_data.ReadString("death");
        enemy_data.CloseTable();
    }

    if (enemy_data.OpenTable("base_stats")) {
        _max_hit_points = enemy_data.ReadUInt("hit_points");
        _hit_points = _max_hit_points;
        _max_skill_points = enemy_data.ReadUInt("skill_points");
        _skill_points = _max_skill_points;
        _experience_points = enemy_data.ReadUInt("experience_points");
        _strength.SetBase(enemy_data.ReadUInt("strength"));
        _vigor.SetBase(enemy_data.ReadUInt("vigor"));
        _fortitude.SetBase(enemy_data.ReadUInt("fortitude"));
        _protection.SetBase(enemy_data.ReadUInt("protection"));
        _agility.SetBase(enemy_data.ReadUInt("agility"));
        _evade.SetBase(enemy_data.ReadFloat("evade"));
        _drunes_dropped = enemy_data.ReadUInt("drunes");
        enemy_data.CloseTable();
    }

    // Create the attack points for the enemy
    if (enemy_data.OpenTable("attack_points")) {
        uint32 ap_size = enemy_data.GetTableSize();
        for(uint32 i = 1; i <= ap_size; i++) {
            _attack_points.push_back(new GlobalAttackPoint(this));
            if (enemy_data.OpenTable(i)) {
                if(_attack_points.back()->LoadData(enemy_data) == false) {
                    IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to load data for an attack point: "
                        << i << std::endl;
                }
                enemy_data.CloseTable();
            }
        }
        enemy_data.CloseTable();
    }

    // Add the set of skills for the enemy
    if (enemy_data.OpenTable("skills")) {
        for(uint32 i = 1; i <= enemy_data.GetTableSize(); i++) {
            _skill_set.push_back(enemy_data.ReadUInt(i));
        }
        enemy_data.CloseTable();
    }

    // Load the possible items that the enemy may drop
    if (enemy_data.OpenTable("drop_objects")) {
        for(uint32 i = 1; i <= enemy_data.GetTableSize(); i++) {
            enemy_data.OpenTable(i);
            _dropped_objects.push_back(enemy_data.ReadUInt(1));
            _dropped_chance.push_back(enemy_data.ReadFloat(2));
            enemy_data.CloseTable();
        }
        enemy_data.CloseTable();
    }

    enemy_data.CloseTable(); // enemies[_id]
    enemy_data.CloseTable(); // enemies

    if(enemy_data.IsErrorDetected()) {
        if(GLOBAL_DEBUG) {
            PRINT_WARNING << "one or more errors occurred while reading the enemy data - they are listed below"
                          << std::endl << enemy_data.GetErrorMessages() << std::endl;
        }
    }

    enemy_data.CloseFile();

    _CalculateAttackRatings();
    _CalculateDefenseRatings();
    _CalculateEvadeRatings();
} // GlobalEnemy::GlobalEnemy(uint32 id)



bool GlobalEnemy::AddSkill(uint32 skill_id)
{
    if(skill_id == 0) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function received an invalid skill_id argument: " << skill_id << std::endl;
        return false;
    }

    if(HasSkill(skill_id)) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to add skill because the enemy already knew this skill: " << skill_id << std::endl;
        return false;
    }

    GlobalSkill *skill = new GlobalSkill(skill_id);
    if(skill->IsValid() == false) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "the skill to add failed to load: " << skill_id << std::endl;
        delete skill;
        return false;
    }

    // Insert the pointer to the new skill inside of the global skills vectors
    _skills.push_back(skill);
    _skills_id.push_back(skill_id);
    return true;
}



void GlobalEnemy::Initialize()
{
    if(_skills.empty() == false) { // Indicates that the enemy has already been initialized
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function was invoked for an already initialized enemy: " << _id << std::endl;
        return;
    }

    // Add all new skills that should be available at the current experience level
    for(uint32 i = 0; i < _skill_set.size(); i++) {
        AddSkill(_skill_set[i]);
    }

    if(_skills.empty()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "no skills were added for the enemy: " << _id << std::endl;
    }

    // Randomize the stats by using a guassian random variable
    if(_no_stat_randomization == false) {
        // Use the base stats as the means and a standard deviation of 10% of the mean
        _max_hit_points = GaussianRandomValue(_max_hit_points, _max_hit_points / 10.0f);
        _max_skill_points = GaussianRandomValue(_max_skill_points, _max_skill_points / 10.0f);
        _experience_points = GaussianRandomValue(_experience_points, _experience_points / 10.0f);
        _strength.SetBase(GaussianRandomValue(_strength.GetBase(), _strength.GetBase() / 10.0f));
        _vigor.SetBase(GaussianRandomValue(_vigor.GetBase(), _vigor.GetBase() / 10.0f));
        _fortitude.SetBase(GaussianRandomValue(_fortitude.GetBase(), _fortitude.GetBase() / 10.0f));
        _protection.SetBase(GaussianRandomValue(_protection.GetBase(), _protection.GetBase() / 10.0f));
        _agility.SetBase(GaussianRandomValue(_agility.GetBase(), _agility.GetBase() / 10.0f));

        // Multiply the evade value by 10 to permit the decimal to be kept
        float evade = _evade.GetBase() * 10.0f;
        _evade.SetBase(static_cast<float>(GaussianRandomValue(evade, evade / 10.0f)) / 10.0f);

        _drunes_dropped = GaussianRandomValue(_drunes_dropped, _drunes_dropped / 10.0f);
    }

    // Set the current hit points and skill points to their new maximum values
    _hit_points = _max_hit_points;
    _skill_points = _max_skill_points;
} // void GlobalEnemy::Initialize(uint32 xp_level)



void GlobalEnemy::DetermineDroppedObjects(std::vector<GlobalObject *>& objects)
{
    objects.clear();

    for(uint32 i = 0; i < _dropped_objects.size(); i++) {
        if(RandomFloat() < _dropped_chance[i]) {
            objects.push_back(GlobalCreateNewObject(_dropped_objects[i]));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// GlobalParty class
////////////////////////////////////////////////////////////////////////////////

void GlobalParty::AddCharacter(GlobalCharacter *character, int32 index)
{
    if(character == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function received a NULL character argument" << std::endl;
        return;
    }

    if(_allow_duplicates == false) {
        // Check that this character is not already in the party
        for(uint32 i = 0; i < _characters.size(); i++) {
            if(character->GetID() == _characters[i]->GetID()) {
                IF_PRINT_WARNING(GLOBAL_DEBUG) << "attempted to add an character that was already in the party "
                                               << "when duplicates were not allowed: " << character->GetID() << std::endl;
                return;
            }
        }
    }

    // Add character to the end of the party if index is negative
    if(index < 0) {
        _characters.push_back(character);
        return;
    }

    // Check that the requested index does not exceed the size of the container
    if(static_cast<uint32>(index) >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded the current party size: " << index << std::endl;
        _characters.push_back(character); // Add the character to the end of the party instead
        return;
    } else {
        std::vector<GlobalCharacter *>::iterator position = _characters.begin();
        for(int32 i = 0; i < index; i++, position++);
        _characters.insert(position, character);
    }
}



GlobalCharacter *GlobalParty::RemoveCharacterAtIndex(uint32 index)
{
    if(index >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded current party size: "
                                       << index << std::endl;
        return NULL;
    }

    GlobalCharacter *removed_character = _characters[index];
    std::vector<GlobalCharacter *>::iterator position = _characters.begin();
    for(uint32 i = 0; i < index; i++, position++);
    _characters.erase(position);

    return removed_character;
}



GlobalCharacter *GlobalParty::RemoveCharacterByID(uint32 id)
{
    if(_allow_duplicates) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "tried to remove character when duplicates were allowed in the party: " << id << std::endl;
        return NULL;
    }

    GlobalCharacter *removed_character = NULL;
    for(std::vector<GlobalCharacter *>::iterator position = _characters.begin(); position != _characters.end(); position++) {
        if(id == (*position)->GetID()) {
            removed_character = *position;
            _characters.erase(position);
            break;
        }
    }

    if(removed_character == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to find an character in the party with the requested id: " << id << std::endl;
    }

    return removed_character;
}



GlobalCharacter *GlobalParty::GetCharacterAtIndex(uint32 index) const
{
    if(index >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded current party size: " << index << std::endl;
        return NULL;
    }

    return _characters[index];
}



GlobalCharacter *GlobalParty::GetCharacterByID(uint32 id) const
{
    if(_allow_duplicates) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "tried to retrieve character when duplicates were allowed in the party: " << id << std::endl;
        return NULL;
    }

    for(uint32 i = 0; i < _characters.size(); i++) {
        if(_characters[i]->GetID() == id) {
            return _characters[i];
        }
    }

    IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to find an character in the party with the requested id: " << id << std::endl;
    return NULL;
}



void GlobalParty::SwapCharactersByIndex(uint32 first_index, uint32 second_index)
{
    if(first_index == second_index) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "first_index and second_index arguments had the same value: " << first_index << std::endl;
        return;
    }
    if(first_index >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "first_index argument exceeded current party size: " << first_index << std::endl;
        return;
    }
    if(second_index >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "second_index argument exceeded current party size: " << second_index << std::endl;
        return;
    }

    GlobalCharacter *tmp = _characters[first_index];
    _characters[first_index] = _characters[second_index];
    _characters[second_index] = tmp;
}



void GlobalParty::SwapCharactersByID(uint32 first_id, uint32 second_id)
{
    if(first_id == second_id) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "first_id and second_id arguments had the same value: " << first_id << std::endl;
        return;
    }
    if(_allow_duplicates) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "tried to swap characters when duplicates were allowed in the party: " << first_id << std::endl;
        return;
    }

    std::vector<GlobalCharacter *>::iterator first_position;
    std::vector<GlobalCharacter *>::iterator second_position;
    for(first_position = _characters.begin(); first_position != _characters.end(); first_position++) {
        if((*first_position)->GetID() == first_id)
            break;
    }
    for(second_position = _characters.begin(); second_position != _characters.end(); second_position++) {
        if((*second_position)->GetID() == second_id)
            break;
    }

    if(first_position == _characters.end()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to find an character in the party with the requested first_id: " << first_id << std::endl;
        return;
    }
    if(second_position == _characters.end()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to find an character in the party with the requested second_id: " << second_id << std::endl;
        return;
    }

    GlobalCharacter *tmp = *first_position;
    *first_position = *second_position;
    *second_position = tmp;
}



GlobalCharacter *GlobalParty::ReplaceCharacterByIndex(uint32 index, GlobalCharacter *new_character)
{
    if(new_character == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function received a NULL new_character argument" << std::endl;
        return NULL;
    }
    if(index >= _characters.size()) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "index argument exceeded current party size: " << index << std::endl;
        return NULL;
    }

    GlobalCharacter *tmp = _characters[index];
    _characters[index] = new_character;
    return tmp;
}



GlobalCharacter *GlobalParty::ReplaceCharacterByID(uint32 id, GlobalCharacter *new_character)
{
    if(_allow_duplicates) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "tried to replace character when duplicates were allowed in the party: " << id << std::endl;
        return NULL;
    }
    if(new_character == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "function received a NULL new_character argument" << std::endl;
        return NULL;
    }

    GlobalCharacter *removed_character = NULL;
    for(std::vector<GlobalCharacter *>::iterator position = _characters.begin(); position != _characters.end(); position++) {
        if((*position)->GetID() == id) {
            removed_character = *position;
            *position = new_character;
            break;
        }
    }

    if(removed_character == NULL) {
        IF_PRINT_WARNING(GLOBAL_DEBUG) << "failed to find an character in the party with the requested id: " << id << std::endl;
    }

    return removed_character;
}



float GlobalParty::AverageExperienceLevel() const
{
    if(_characters.empty())
        return 0.0f;

    float xp_level_sum = 0.0f;
    for(uint32 i = 0; i < _characters.size(); i++)
        xp_level_sum += static_cast<float>(_characters[i]->GetExperienceLevel());
    return (xp_level_sum / static_cast<float>(_characters.size()));
}



void GlobalParty::AddHitPoints(uint32 hp)
{
    for(std::vector<GlobalCharacter *>::iterator i = _characters.begin(); i != _characters.end(); i++) {
        (*i)->AddHitPoints(hp);
    }
}



void GlobalParty::AddSkillPoints(uint32 sp)
{
    for(std::vector<GlobalCharacter *>::iterator i = _characters.begin(); i != _characters.end(); i++) {
        (*i)->AddSkillPoints(sp);
    }
}

} // namespace vt_global
