awareCrackdown = {
	{id="2298222454",	name="Selector",	pid="none"},
	{id="2264743342",	name="LookForTarget",	pid="2298222454",	args={bypassAttackable=false}},
	{id="694324695",	name="Sequence",	pid="2264743342"},
	{id="3924187176",	name="CalculateAggroMod",	pid="694324695"},
	{id="179038857",	name="If",	pid="694324695"},
	{id="2459160625",	name="CheckProspectInRange",	pid="179038857",	args={condition=0.0}},
	{id="3089790844",	name="AlwaysSucceed",	pid="694324695"},
	{id="369467436",	name="TreeSocket",	pid="3089790844",	args={slot=LOOKAT}},
	{id="4223164170",	name="Sequence",	pid="694324695"},
	{id="4253120352",	name="If",	pid="4223164170"},
	{id="3587338575",	name="CheckFollowState",	pid="4253120352",	args={condition=WATCHING}},
	{id="1739242947",	name="Sequence",	pid="4223164170"},
	{id="3787846453",	name="If",	pid="1739242947"},
	{id="2303354763",	name="CheckAggroDelayPast",	pid="3787846453"},
	{id="853857154",	name="TreeSocket",	pid="1739242947",	args={slot=AGGRO}},
	{id="382608539",	name="TreeSocket",	pid="2298222454",	args={slot=KILL}},
	{id="4105309737",	name="AlwaysFail",	pid="2298222454"},
	{id="858199136",	name="Sequence",	pid="4105309737"},
	{id="1627703410",	name="EraseBlackboard",	pid="858199136",	args={param="aggroMod"}},
	{id="1066229818",	name="EraseBlackboard",	pid="858199136",	args={param="targetProspect"}}}
addAiTemplate("awareCrackdown", awareCrackdown)

idleCrackdown = {
	{id="341662921",	name="Selector",	pid="none"},
	{id="2934497343",	name="Sequence",	pid="341662921"},
	{id="2134156106",	name="Sequence",	pid="2934497343"},
	{id="3631848012",	name="If",	pid="2134156106"},
	{id="2897325830",	name="CheckDestination",	pid="3631848012",	args={condition=0.0}},
	{id="116360107",	name="Not",	pid="2134156106"},
	{id="883722203",	name="If",	pid="116360107"},
	{id="3219226529",	name="CheckFollowState",	pid="883722203",	args={condition=FOLLOWING}},
	{id="515022344",	name="Not",	pid="2134156106"},
	{id="567368115",	name="If",	pid="515022344"},
	{id="236122192",	name="CheckFollowState",	pid="567368115",	args={condition=PATROLLING}},
	{id="1137971962",	name="Wait",	pid="2934497343",	args={duration=-1.0}},
	{id="1025565676",	name="Sequence",	pid="341662921"},
	{id="3326622834",	name="Selector",	pid="1025565676"},
	{id="3071138585",	name="Sequence",	pid="3326622834"},
	{id="1908035156",	name="If",	pid="3071138585"},
	{id="3873815212",	name="CheckIsDroid",	pid="1908035156"},
	{id="2472779767",	name="WriteBlackboard",	pid="3071138585",	args={key="moveMode", val=RUN}},
	{id="3225520752",	name="WriteBlackboard",	pid="3326622834",	args={key="moveMode", val=WALK}},
	{id="3869875258",	name="TreeSocket",	pid="1025565676",	args={slot=MOVE}}}
addAiTemplate("idleCrackdown", idleCrackdown)

rootCrackdown = {
	{id="3448069951",	name="Selector",	pid="none"},
	{id="1535795012",	name="Sequence",	pid="3448069951"},
	{id="3812183071",	name="Not",	pid="1535795012"},
	{id="4114823391",	name="If",	pid="3812183071"},
	{id="2314700981",	name="CheckFollowState",	pid="4114823391",	args={condition=FLEEING}},
	{id="4186622147",	name="TreeSocket",	pid="1535795012",	args={slot=TARGET}},
	{id="9271994",	name="ParallelSelector",	pid="1535795012"},
	{id="3801372768",	name="Sequence",	pid="9271994"},
	{id="1114709841",	name="TreeSocket",	pid="3801372768",	args={slot=EQUIP}},
	{id="417864954",	name="TreeSocket",	pid="3801372768",	args={slot=ATTACK}},
	{id="2572943843",	name="Sequence",	pid="9271994"},
	{id="2411220363",	name="Selector",	pid="2572943843"},
	{id="1092511889",	name="Sequence",	pid="2411220363"},
	{id="3798852741",	name="If",	pid="1092511889"},
	{id="2561729607",	name="CheckIsInCombat",	pid="3798852741"},
	{id="300809128",	name="WriteBlackboard",	pid="1092511889",	args={key="moveMode", val=RUN}},
	{id="2814641736",	name="WriteBlackboard",	pid="2411220363",	args={key="moveMod", val=WALK}},
	{id="1512143592",	name="TreeSocket",	pid="2572943843",	args={slot=MOVE}},
	{id="617162459",	name="TreeSocket",	pid="3448069951",	args={slot=AWARE}},
	{id="3529678842",	name="TreeSocket",	pid="3448069951",	args={slot=IDLE}}}
addAiTemplate("rootCrackdown", rootCrackdown)

