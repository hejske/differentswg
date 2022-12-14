bestine_quest_sword = {
	minimumLevel = 0,
	maximumLevel = 0,
	customObjectName = "",
	directObjectTemplate = "object/weapon/melee/sword/bestine_quest_sword.iff",
	craftingValues = {
		{"mindamage",25,46,0},
		{"maxdamage",98,182,0},
		{"attackspeed",6.5,4.5,1},
		{"woundchance",11,20,1},
		{"hitpoints",750,1500,0},
		{"zerorange",0,0,0},
		{"zerorangemod",-5,-5,0},
		{"midrange",3,3,0},
		{"midrangemod",-5,-5,0},
		{"maxrange",4,4,0},
		{"maxrangemod",-5,-5,0},
		{"attackhealthcost",78,42,0},
		{"attackactioncost",40,22,0},
		{"attackmindcost",13,7,0},
	},
	customizationStringNames = {},
	customizationValues = {},

	-- randomDotChance: The chance of this weapon object dropping with a random dot on it. Higher number means less chance. Set to 0 to always have a random dot.
	randomDotChance = 500,

	-- staticDotChance: The chance of this weapon object dropping with a static dot on it. Higher number means less chance. Set to 0 to always have a static dot.
	staticDotChance = 0,

	-- staticDotType: 1 = Poison, 2 = Disease, 3 = Fire, 4 = Bleed
	staticDotType = 3,

	-- staticDotValues: Object map that can randomly or statically generate a dot (used for weapon objects.)
	staticDotValues = {
		{"attribute", 0, 0}, -- See CreatureAttributes.h in src for numbers.
		{"strength", 110, 110},
		{"duration", 30, 240},
		{"potency", 1, 100},
		{"uses", 250, 9999}
	},
	junkDealerTypeNeeded = JUNKARMS,
	junkMinValue = 20,
	junkMaxValue = 60,
}

addLootItemTemplate("bestine_quest_sword", bestine_quest_sword)
