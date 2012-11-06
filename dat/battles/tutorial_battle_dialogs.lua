-- This script executes for the very first battle that the player encounters in a new game.
-- Its purpose is to present a dialogue to the player at the start of the battle to provide
-- a brief explanation of the battle system.
--
-- This script is only used by the river cave access map. All battles on that map load this script,
-- but we only want the script to execute for the first battle. So the "global_events" event group is
-- used to record when the first battle occurs. If it has already occurred, this script effectively does
-- nothing

local ns = {}
setmetatable(ns, {__index = _G})
tutorial_battle_dialogs = ns;
setfenv(1, ns);

function Initialize(battle_instance)
	Battle = battle_instance;
	stop_script = false;

	if (GlobalManager:DoesEventExist("story", "first_battle") == false) then
		GlobalManager:SetEventValue("story", "first_battle", 1);
		stop_script = false;
	else
		stop_script = true;
		return;
	end

	DialogueManager = Battle:GetDialogueSupervisor();

	main_dialogue = {};

	-- Add all speakers for the dialogues to be added
	-- TODO: all of these custom speaker calls should be replaced with calls to AddCharacterSpeaker() later
	DialogueManager:AddCustomSpeaker(1000, "Bronann", "img/portraits/bronann.png");
	DialogueManager:AddCustomSpeaker(1002, "Kalya", "img/portraits/kalya.png");

	local text;
	-- The dialogue constructed below offers the player instructions on how to do battle. It is displayed only once in the first few seconds
	-- of battle, before any action can be taken. The player is presented with several options that they can read to get more information on
	-- the battle system. One of the options that the player may select from will finish the dialogue, allow the battle to resume.
	main_dialogue = hoa_battle.BattleDialogue(1);
		text = hoa_system.Translate("Woah, I wouldn't have expected enemies so close to the village. Bronann, is there anything that you need to ask about battles?");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("...");
		main_dialogue:AddLine(text, 1000);
		text = hoa_system.Translate("Ask about battle basics.");
		main_dialogue:AddOption(text, 2);
		text = hoa_system.Translate("Ask about issuing commands.");
		main_dialogue:AddOption(text, 8);
		text = hoa_system.Translate("Ask about status effects.");
		main_dialogue:AddOption(text, 15);
		text = hoa_system.Translate("Ask nothing. I know how to fight.");
		main_dialogue:AddOption(text, 21);
		-- [Line 2] After selecting option: Ask about battle basics.
		text = hoa_system.Translate("Er, I could use a refresher on the fundamentals of combat.");
		main_dialogue:AddLine(text, 1000);
		text = hoa_system.Translate("Every actor on the battle screen has a hit point (HP) and skill point (SP) gauge. Each character's HP and SP are displayed at the bottom of the screen. As enemies take damage in battle, their appearance will reflect that damage.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("When an actor's HP reaches zero, they become incapacitated. Victory in battle is obtained by reducing the HP of all enemies to zero.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Every action taken in battle requires some amount of SP before it is able to be used. Most basic actions consume no SP. Using an item also consumes no SP.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("The stamina gauge on the right side of the screen determines when an actor may take an action. Once an actor's icon reaches the green command bar, they will begin preparing to execute their selected action. When they reach the top of the gauge, they will execute that action.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("The lower right corner of the screen displays each character's selected action and target. When a command is being issued for a character, this area of the screen is replaced with the command menu.");
		main_dialogue:AddLine(text, 1002, 22);
		-- [Line 8] After selecting option: Ask about issuing commands.
		text = hoa_system.Translate("How do I select commands to execute?");
		main_dialogue:AddLine(text, 1000);
		text = hoa_system.Translate("Each character has three types of skills they may execute as an action: attack, defend, and support skills.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Battle actions requires a certain amount of time to prepare to use as well as time to recover from after using. These periods are known as the warm up time and cool down periods. In general, the more powerful the skill the longer these periods will be.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("At the bottom of the screen, a directional button is shown for each character that you may select a command for. You can set or change the action and/or target issued to a character whenever these buttons are visible.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Once the character's icon reaches the green command bar on the stamina gauge, they will begin preparing to execute any action they have set. If a character does not have a command set when their icon reaches the green command bar, the battle will stop until a command is issued for that character.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("All actions affect only one type of target. The target type for each action is indicated by an icon in the command menu. Possible target types include an ally, enemy, or an entire party. There are also target types which affect an attack point, which is a specific part on the body of an actor.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Attack points have properties that make them susceptible to certain types of attacks or more resistant to others. For example, typically attacking the head of a target will deal more damage, but the attack is more likely to miss this small target.");
		main_dialogue:AddLine(text, 1002, 22);
		-- [Line 15] After selecting option: Ask about about status effects.
		text = hoa_system.Translate("What do I need to know about status effects?");
		main_dialogue:AddLine(text, 1000);
		text = hoa_system.Translate("Status effects cause temporary changes in the state of the afflicted actor. These changes may be either beneficial or harmful to the their target. Positive status effects are indicated by an upward green arrow on the status icon, whereas a negative status effect displays a downward red arrow on its icon.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("There are a total of four intensity levels for each status effect. As an example, if you target an actor that is already inflicted with a negative reduction in strength with a skill that reduces strength, the intensity of the status will be increased.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("By selecting attacks which target specific attack points on your foes, you can sometimes naturally trigger a status effect. For example, try attacking an enemy's arms or claws to reduce their strength. Or choose to go for their wings or legs to reduce their agility.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Status effects naturally grow weaker in intensity and dissipate with time. You must continually reinforce status effects that you wish to remain active.");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("Finally, status effects disappear completely when a battle ends, so there is no need to remedy your characters from status effects after a battle has been won.");
		main_dialogue:AddLine(text, 1002, 22);
		-- [Line 21] After selection option: Ask nothing.
		text = hoa_system.Translate("Don't worry Kalya, I have not forgotten my training.");
		main_dialogue:AddLine(text, 1000, 25);
		-- [Line 22] Returning to the option selection after asking about a topic
		text = hoa_system.Translate("Is there anything else you need to ask?");
		main_dialogue:AddLine(text, 1002);
		text = hoa_system.Translate("...");
		main_dialogue:AddLine(text, 1000);
		text = hoa_system.Translate("Ask about battle basics.");
		main_dialogue:AddOption(text, 2);
		text = hoa_system.Translate("Ask about issuing commands.");
		main_dialogue:AddOption(text, 8);
		text = hoa_system.Translate("Ask about status effects.");
		main_dialogue:AddOption(text, 15);
		text = hoa_system.Translate("Ask nothing.");
		main_dialogue:AddOption(text, 24);
		-- [Line 24] After selecting option: Ask nothing. (After a topic has already been asked).
		text = hoa_system.Translate("Thanks Kalya, I'm prepared now.");
		main_dialogue:AddLine(text, 1000, 25);
		-- [Line 25] Final line of dialogue
		text = hoa_system.Translate("Good. Now let us quickly dispatch this minor threat.");
		main_dialogue:AddLine(text, 1002);
	DialogueManager:AddDialogue(main_dialogue);

	-- Construct a timer so we can start the dialogue a couple seconds after the battle begins
	start_timer = hoa_system.SystemTimer(1000, 0);
end


function Update()
	if (stop_script == true) then
		return;
	end

	start_timer:Update();

	-- Wait until the initial battle sequence ends to begin running the dialogue start timer
	if ((start_timer:IsInitial() == true) and (Battle:GetState() ~= hoa_battle.BattleMode.BATTLE_STATE_INITIAL)) then
		start_timer:Run();
	end

	-- If the dialogue has not been seen yet, check if its time to start it
	if (main_dialogue:HasAlreadySeen() == false) then
		if ((start_timer:IsFinished() == true) and (DialogueManager:IsDialogueActive() == false)) then
			DialogueManager:BeginDialogue(1);
		end
	end
end
