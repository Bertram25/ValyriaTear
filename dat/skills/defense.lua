------------------------------------------------------------------------------[[
-- Filename: defense.lua
--
-- Description: This file contains the definitions of all defense skills.
-- Each defense skill has a unique integer identifier
-- that is used as its key in the skills table below. Some skills are primarily
-- intended for characters to use while others are intended for enemies to use.
-- Normally, we do not want to share skills between characters and enemies as
-- character skills animate the sprites while enemy skills do not.
--
-- Skill IDs 10,001 through 20,000 are reserved for defense skills.
--
-- Each skill entry requires the following data to be defined:
-- {name}: Text that defines the name of the skill
-- {description}: A brief (one sentence) description of the skill
--                (This field is required only for character skills and is optional for enemy skills)
-- {sp_required}: The number of skill points (SP) that are required to use the skill
--                (Zero is a valid value for this field, but a negative number is not)
-- {warmup_time}: The number of milliseconds that the actor using the skill must wait between
--                selecting the skill and executing it (a value of zero is valid).
-- {cooldown_time}: The number of milliseconds that the actor using the skill must wait after
--                  executing the skill before their stamina begins regenrating (zero is valid).
-- {target_type}: The type of target the skill affects, which may be an attack point, actor, or party.
--
-- Each skill entry requires a function called {BattleExecute} to be defined. This function implements the
-- execution of the skill in battle, buffing defense, causing status changes, playing sounds, and animating
-- sprites.
------------------------------------------------------------------------------]]

-- All defense skills definitions are stored in this table
if (skills == nil) then
	skills = {}
end


--------------------------------------------------------------------------------
-- IDs 10,001 - 11,000 are reserved for character defense skills
--------------------------------------------------------------------------------

skills[10001] = {
	name = hoa_system.Translate("Defensive Stance"),
	description = hoa_system.Translate("Increases an ally's physical defense by a slight degree."),
	sp_required = 3,
	warmup_time = 300,
	cooldown_time = 0,
	warmup_action_name = "magic_prepare",
	action_name = "magic_cast",
	target_type = hoa_global.GameGlobal.GLOBAL_TARGET_ALLY,

	BattleExecute = function(user, target)
		target_actor = target:GetActor();
        local effect_duration = user:GetProtection() * 2000;
        if (effect_duration < 10000) then effect_duration = 10000 end
		target_actor:RegisterStatusChange(hoa_global.GameGlobal.GLOBAL_STATUS_FORTITUDE_RAISE,
                                          hoa_global.GameGlobal.GLOBAL_INTENSITY_POS_MODERATE,
                                          effect_duration);
		local Battle = ModeManager:GetTop();
		Battle:TriggerBattleParticleEffect("dat/effects/particles/defensive_stance.lua",
				target_actor:GetXLocation(), target_actor:GetYLocation() - 5);
	end
}

skills[10002] = {
	name = hoa_system.Translate("First Aid"),
	description = hoa_system.Translate("Performs basic medical assistance, healing the target by a minor degree."),
	sp_required = 4,
	warmup_time = 1500,
	cooldown_time = 200,
	warmup_action_name = "magic_prepare",
	action_name = "magic_cast",
	target_type = hoa_global.GameGlobal.GLOBAL_TARGET_ALLY,

	BattleExecute = function(user, target)
		target_actor = target:GetActor();
		local hit_points = (user:GetVigor() * 3) +  hoa_utils.RandomBoundedInteger(0, 15);
        target_actor:RegisterHealing(hit_points, true);
		AudioManager:PlaySound("snd/heal_spell.wav");
		local Battle = ModeManager:GetTop();
		Battle:TriggerBattleParticleEffect("dat/effects/particles/heal_particle.lua",
				target_actor:GetXLocation(), target_actor:GetYLocation() - 5);
	end,

	FieldExecute = function(target, instigator)
		target:AddHitPoints((instigator:GetVigor() * 5) + hoa_utils.RandomBoundedInteger(0, 30));
		AudioManager:PlaySound("snd/heal_spell.wav");
	end
}

--------------------------------------------------------------------------------
-- IDs 11,001 - 20,000 are reserved for enemy defense skills
--------------------------------------------------------------------------------


