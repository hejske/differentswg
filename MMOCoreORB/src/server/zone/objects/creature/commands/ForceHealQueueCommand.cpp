/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#include "ForceHealQueueCommand.h"
#include "server/zone/managers/combat/CombatManager.h"
#include "templates/params/creature/CreatureAttribute.h"
#include "server/zone/managers/stringid/StringIdManager.h"
#include "server/zone/managers/collision/CollisionManager.h"
#include "server/zone/managers/frs/FrsManager.h"
#include "server/zone/objects/player/FactionStatus.h"
#include "server/zone/objects/building/BuildingObject.h"
#include "server/zone/objects/creature/events/HealOverTimeTask.h"

ForceHealQueueCommand::ForceHealQueueCommand(const String& name, ZoneProcessServer* server) : JediQueueCommand(name, server) {
	speed = 3;
	allowedTarget = TARGET_AUTO;

	forceCost = 0;
	forceCostMultiplier = 0;

	statesToHeal = 0;
	healStateCost = 0;

	healBleedingCost = 0;
	healPoisonCost = 0;
	healDiseaseCost = 0;
	healFireCost = 0;

	attributesToHeal = 0;
	woundAttributesToHeal = 0;

	healBattleFatigue = 0;
	healAmount = 0;
	healWoundAmount = 0;

	bleedHealIterations = 1;
	poisonHealIterations = 1;
	diseaseHealIterations = 1;
	fireHealIterations = 1;

	visMod = 10;

	range = 0;

	clientEffect = "clienteffect/pl_force_heal_self.cef";
	animationCRC = STRING_HASHCODE("force_healing_1");

	healingScaling = 0;

	isArea = false;

	isJediHealOverTime = false;

	hotHealAmount = 0;

	hotTicks = 3;

	hotTickFrequency = 3;

	hotHealingScaling = 0;

	maxHotTicks = hotTicks * 3;


}

int ForceHealQueueCommand::runCommand(CreatureObject* creature, CreatureObject* targetCreature) const {
	ManagedReference<PlayerObject*> playerObject = creature->getPlayerObject();

	if (playerObject == nullptr)
		return GENERALERROR;

	int currentForce = playerObject->getForcePower();
	int totalCost = forceCost;
	bool healPerformed = false;

	int actualHealAmount = 0;
	int actualHotHealAmount = 0;

	int healingPower = creature->getSkillMod("force_healing_power");

	int moreHealing = creature->getSkillMod("more_force_healing") + creature->getSkillMod("more_healing");

	int increasedHealing = creature->getSkillMod("increased_force_healing");

	//regular healing calc
	if (healAmount > 0) {
		//flat healing power
		if (healingScaling != 0) {
			if (healingPower > 0) {
				actualHealAmount = healAmount + healingScaling * healingPower;
			}
		}
	
		//% healing increase
		if (increasedHealing > 0)
			actualHealAmount *= (1 + increasedHealing / 100.f);

		//more force healing
		if (moreHealing > 0)
			actualHealAmount *= (1 + moreHealing / 100.f);
	}

	//hot healing calc
	if (hotHealAmount > 0) {
		//flat healing power
		if (hotHealingScaling != 0) {
			if (healingPower > 0) {
				actualHotHealAmount = hotHealAmount + hotHealingScaling * healingPower;
			}
		}
		//% healing increase
		if (increasedHealing > 0)
			actualHotHealAmount *= (1 + increasedHealing / 100.f);

		//more force healing
		if (moreHealing > 0)
			actualHotHealAmount *= (1 + moreHealing / 100.f);
	}

		//check for crit
		int critRating = creature->getSkillMod("increased_force_healing_critical_chance") + creature->getSkillMod("increased_healing_critical_chance") + creature->getSkillMod("increased_critical_chance");
		if (critRating > 0) {
			if (System::random(100) < 5 * (1 + critRating / 100.f) + 1) {
				actualHealAmount *= 1.5 + 0.5 * ((creature->getSkillMod("increased_force_healing_critical_amount") + creature->getSkillMod("increased_healing_critical_amount")) / 100.f);
			}
		}
		else {
			if (System::random(100) < 6) {
				actualHealAmount *= 1.5 + 0.5 * ((creature->getSkillMod("increased_force_healing_critical_amount") + creature->getSkillMod("increased_healing_critical_amount")) / 100.f);
			}
		}

	if (isArea && currentForce > totalCost) {
		//creature->sendSystemMessage("You attempt to cast an area force heal");
		if (creature == targetCreature) {
			creature->playEffect(clientEffect, "");
		}
		else {
			creature->doCombatAnimation(targetCreature, animationCRC, 0, 0xFF);
		}
		handleJediHealArea(creature, creature, targetCreature, actualHealAmount);
		playerObject->setForcePower(currentForce - totalCost);
		return SUCCESS;
	}

	if (isJediHealOverTime) {
		handleJediHealOverTime(targetCreature, actualHotHealAmount);
	}

	// Attribute Wound Healing
	for (int i = 0; i < 3; i++) {
		// Attrib Values: Health = 1, Action = 2, Mind = 4
		if (woundAttributesToHeal & (1 << i)) {
			for (int j = 0; j < 3; j++) {
				if (totalCost < currentForce) {
					uint8 attrib = (i * 3) + j;
					int woundAmount = targetCreature->getWounds(attrib);

					if (healWoundAmount > 0 && woundAmount > healWoundAmount)
						woundAmount = healWoundAmount;

					totalCost += woundAmount * forceCostMultiplier;

					if (totalCost > currentForce) {
						int forceDiff = totalCost - currentForce;
						totalCost -= forceDiff;
						woundAmount -= forceDiff / forceCostMultiplier;
					}

					if (woundAmount > 0) {
						targetCreature->healWound(creature, attrib, woundAmount, true);
						healPerformed = true;
						sendHealMessage(creature, targetCreature, HEAL_WOUNDS, attrib, woundAmount);
					}
				}
			}
		}
	}

	// HAM Attribute Healing
	for (int i = 0; i < 3; i++) {
		// Attrib Values: Health = 1, Action = 2, Mind = 4
		if (attributesToHeal & (1 << i)) {
			if (totalCost < currentForce) {
				uint8 attrib = i * 3;
				int curHam = targetCreature->getHAM(attrib);
				int maxHam = targetCreature->getMaxHAM(attrib) - targetCreature->getWounds(attrib);
				int amtToHeal = maxHam - curHam;

				if (actualHealAmount > 0 && amtToHeal > actualHealAmount)
					amtToHeal = actualHealAmount;

				totalCost += amtToHeal * forceCostMultiplier;

				if (totalCost > currentForce) {
					int forceDiff = totalCost - currentForce;
					totalCost -= forceDiff;
					amtToHeal -= forceDiff / forceCostMultiplier;
				}

				if (amtToHeal > 0) {
					targetCreature->healDamage(creature, attrib, amtToHeal, true);
					healPerformed = true;
					sendHealMessage(creature, targetCreature, HEAL_DAMAGE, attrib, amtToHeal);
				}
			}
		}
	}

	// Battle fatigue
	if (totalCost < currentForce && healBattleFatigue != 0) {
		int battleFatigue = targetCreature->getShockWounds();

		if (healBattleFatigue > 0 && battleFatigue > healBattleFatigue)
			battleFatigue = healBattleFatigue;

		totalCost += battleFatigue * forceCostMultiplier;

		if (totalCost > currentForce) {
			int forceDiff = totalCost - currentForce;
			totalCost -= forceDiff;
			battleFatigue -= forceDiff / forceCostMultiplier;
		}

		if (battleFatigue > 0) {
			targetCreature->addShockWounds(-battleFatigue, true, false);
			sendHealMessage(creature, targetCreature, HEAL_FATIGUE, 0, battleFatigue);
			healPerformed = true;
		}
	}

	// States - Stun, Blind, Dizzy, Intim
	if (totalCost < currentForce) {
		int totalStates = 0;
		int healedStates = 0;
		for (int i = 12; i <= 15; i++) {
			int state = (1 << i);

			if ((statesToHeal & state) && targetCreature->hasState(state)) {
				totalStates++;
				int newTotal = totalCost + healStateCost;

				if (newTotal < currentForce) {
					targetCreature->removeStateBuff(state);
					totalCost = newTotal;
					healPerformed = true;
					healedStates++;
				}
			}
		}

		if (healedStates > 0 && healedStates == totalStates)
			sendHealMessage(creature, targetCreature, HEAL_STATES, 0, 0);
	}

	// Bleeding
	if (targetCreature->isBleeding() && healBleedingCost > 0 && totalCost + healBleedingCost < currentForce) {
		bool result = false;
		int iteration = 1;

		while (!result && (totalCost + healBleedingCost < currentForce) && (bleedHealIterations == -1 || iteration <= bleedHealIterations)) {
			result = targetCreature->healDot(CreatureState::BLEEDING, 250, false);
			totalCost += healBleedingCost;
			iteration++;
		}

		if (result) {
			sendHealMessage(creature, targetCreature, HEAL_BLEEDING, 0, 1);
		} else {
			sendHealMessage(creature, targetCreature, HEAL_BLEEDING, 0, 0);
		}

		healPerformed = true;
	}

	// Poison
	if (targetCreature->isPoisoned() && healPoisonCost > 0 && totalCost + healPoisonCost < currentForce) {
		bool result = false;
		int iteration = 1;

		while (!result && (totalCost + healPoisonCost < currentForce) && (poisonHealIterations == -1 || iteration <= poisonHealIterations)) {
			result = targetCreature->healDot(CreatureState::POISONED, 250, false);
			totalCost += healPoisonCost;
			iteration++;
		}

		if (result) {
			sendHealMessage(creature, targetCreature, HEAL_POISON, 0, 1);
		} else {
			sendHealMessage(creature, targetCreature, HEAL_POISON, 0, 0);
		}

		healPerformed = true;
	}

	// Disease
	if (targetCreature->isDiseased() && healDiseaseCost > 0 && totalCost + healDiseaseCost < currentForce) {
		bool result = false;
		int iteration = 1;

		while (!result && (totalCost + healDiseaseCost < currentForce) && (diseaseHealIterations == -1 || iteration <= diseaseHealIterations)) {
			result = targetCreature->healDot(CreatureState::DISEASED, 200, false);
			totalCost += healDiseaseCost;
			iteration++;
		}

		if (result) {
			sendHealMessage(creature, targetCreature, HEAL_DISEASE, 0, 1);
		} else {
			sendHealMessage(creature, targetCreature, HEAL_DISEASE, 0, 0);
		}

		healPerformed = true;
	}

	// Fire
	if (targetCreature->isOnFire() && healFireCost > 0 && totalCost + healFireCost < currentForce) {
		bool result = false;
		int iteration = 1;

		while (!result && (totalCost + healFireCost < currentForce) && (fireHealIterations == -1 || iteration <= fireHealIterations)) {
			result = targetCreature->healDot(CreatureState::ONFIRE, 500, false);
			totalCost += healFireCost;
			iteration++;
		}

		if (result) {
			sendHealMessage(creature, targetCreature, HEAL_FIRE, 0, 1);
		} else {
			sendHealMessage(creature, targetCreature, HEAL_FIRE, 0, 0);
		}

		healPerformed = true;
	}

	bool selfHeal = creature->getObjectID() == targetCreature->getObjectID();

	if (healPerformed) {
		if (selfHeal)
			creature->playEffect(clientEffect, "");
		else
			creature->doCombatAnimation(targetCreature, animationCRC, 0, 0xFF);

		if (currentForce < totalCost) {
			playerObject->setForcePower(0);
			creature->error("Did not have enough force to pay for the healing he did. Total cost of command: " + String::valueOf(totalCost) + ", player's current force: " + String::valueOf(currentForce));
		} else {
			playerObject->setForcePower(currentForce - totalCost);
		}

		VisibilityManager::instance()->increaseVisibility(creature, visMod);

		// Active Area pvp area TEF, applies TEF to healer if the target has the area TEF
		if (!selfHeal && targetCreature->isPlayerCreature()) {
			PlayerObject* targetGhost = targetCreature->getPlayerObject().get();

			if (targetGhost != nullptr) {
				bool covertOvert = ConfigManager::instance()->useCovertOvertSystem();

				if (covertOvert) {
					int healerStatus = creature->getFactionStatus();
					int targetStatus = targetCreature->getFactionStatus();

					if (!CombatManager::instance()->areInDuel(creature, targetCreature)) {
						if ((healerStatus >= FactionStatus::COVERT && targetGhost->hasGcwTef()) || (targetStatus == FactionStatus::OVERT && healerStatus == FactionStatus::COVERT)) {
							playerObject->updateLastGcwPvpCombatActionTimestamp();
						}
					}
				}

				if (targetGhost->isInPvpArea(true)) {
					playerObject->updateLastPvpAreaCombatActionTimestamp();
				}
			}
		}

		return SUCCESS;
	} else {
		if (selfHeal) {
			creature->sendSystemMessage("@jedi_spam:no_damage_heal_self");
		} else {
			creature->sendSystemMessage("@jedi_spam:no_damage_heal_other");
		}

		return GENERALERROR;
	}
}

void ForceHealQueueCommand::sendHealMessage(CreatureObject* creature, CreatureObject* target, int healType, int healSpec, int amount) const {
	if (creature == nullptr || target == nullptr || amount < 0)
		return;

	uint64 playerID = creature->getObjectID();
	uint64 targetID = target->getObjectID();
	const bool selfHeal = playerID == targetID;

	if (healType == HEAL_DAMAGE || healType == HEAL_WOUNDS || healType == HEAL_FATIGUE) {
		String strVal = "@jedi_spam:";

		switch (healType) {
		case HEAL_DAMAGE: strVal = strVal + CreatureAttribute::getName(healSpec) + "_damage"; break;
		case HEAL_WOUNDS: strVal = strVal + CreatureAttribute::getName(healSpec) + "_wounds"; break;
		default: strVal = strVal + "battle_fatigue";
		}

		String statStr = StringIdManager::instance()->getStringId(strVal.hashCode()).toString();

		if (selfHeal) {
			StringIdChatParameter message("jedi_spam", "heal_self");
			message.setTO(statStr);
			message.setDI(amount);
			creature->sendSystemMessage(message);
		} else {
			StringIdChatParameter message("jedi_spam", "heal_other_self");
			message.setTO(statStr);
			message.setDI(amount);
			message.setTT(targetID);
			creature->sendSystemMessage(message);

			if (target->isPlayerCreature()) {
				StringIdChatParameter message("jedi_spam", "heal_other_other");
				message.setTO(statStr);
				message.setDI(amount);
				message.setTT(playerID);
				target->sendSystemMessage(message);
			}
		}
	} else if (healType == HEAL_STATES && amount == 0 && !selfHeal) {
		StringIdChatParameter message("jedi_spam", "stop_states_other");
		message.setTT(targetID);
		creature->sendSystemMessage(message);
	} else if (healType == HEAL_POISON && !selfHeal) {
		if (amount == 1) {
			StringIdChatParameter message("jedi_spam", "stop_poison_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		} else {
			StringIdChatParameter message("jedi_spam", "staunch_poison_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		}
	} else if (healType == HEAL_DISEASE && !selfHeal) {
		if (amount == 1) {
			StringIdChatParameter message("jedi_spam", "stop_disease_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		} else {
			StringIdChatParameter message("jedi_spam", "staunch_disease_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		}
	} else if (healType == HEAL_BLEEDING && !selfHeal) {
		if (amount == 1) {
			StringIdChatParameter message("jedi_spam", "stop_bleeding_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		} else {
			StringIdChatParameter message("jedi_spam", "staunch_bleeding_other");
			message.setTT(targetID);
			creature->sendSystemMessage(message);
		}
	}

	if (amount == 0) {
		if (healType == HEAL_POISON) {
			target->getDamageOverTimeList()->sendDecreaseMessage(target, CreatureState::POISONED);
		} else if (healType == HEAL_DISEASE) {
			target->getDamageOverTimeList()->sendDecreaseMessage(target, CreatureState::DISEASED);
		} else if (healType == HEAL_BLEEDING) {
			target->getDamageOverTimeList()->sendDecreaseMessage(target, CreatureState::BLEEDING);
		} else if (healType == HEAL_FIRE) {
			target->getDamageOverTimeList()->sendDecreaseMessage(target, CreatureState::ONFIRE);
		}
	}
}

int ForceHealQueueCommand::runCommandWithTarget(CreatureObject* creature, CreatureObject* targetCreature) const {
	if (creature == nullptr || targetCreature == nullptr)
		return GENERALERROR;

	if (creature->getObjectID() == targetCreature->getObjectID()) // no self healing
		return GENERALERROR;

	if ((!targetCreature->isPlayerCreature() && !targetCreature->isPet()) || targetCreature->isDroidObject() || targetCreature->isWalkerSpecies())
		return INVALIDTARGET;

	if (targetCreature->isDead() || targetCreature->isAttackableBy(creature))
		return GENERALERROR;

	Locker crossLocker(targetCreature, creature);

	if (creature->isKnockedDown())
		return GENERALERROR;

	if(!checkDistance(creature, targetCreature, range))
		return TOOFAR;

	if (checkForArenaDuel(targetCreature)) {
		creature->sendSystemMessage("@jedi_spam:no_help_target"); // You are not permitted to help that target.
		return GENERALERROR;
	}

	if (!targetCreature->isHealableBy(creature)) {
		creature->sendSystemMessage("@healing:pvp_no_help"); // It would be unwise to help such a patient.
		return GENERALERROR;
	}

	if (!CollisionManager::checkLineOfSight(creature, targetCreature)) {
		creature->sendSystemMessage("@healing:no_line_of_sight"); // You cannot see your target.
		return GENERALERROR;
	}

	if (!playerEntryCheck(creature, targetCreature)) {
		return GENERALERROR;
	}

	return runCommand(creature, targetCreature);
}

int ForceHealQueueCommand::doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {
	if (creature == nullptr || !creature->isPlayerCreature())
		return GENERALERROR;

	if (!checkInvalidLocomotions(creature))
		return INVALIDLOCOMOTION;

	int comResult = doCommonJediSelfChecks(creature);

	if (comResult != SUCCESS)
		return comResult;

	ManagedReference<CreatureObject*> targetCreature = nullptr;
	bool isRemoteHeal = range > 0 && ((allowedTarget == TARGET_AUTO) || (allowedTarget & TARGET_OTHER));

	if (isRemoteHeal && target != 0 && target != creature->getObjectID()) {
		ManagedReference<SceneObject*> sceno = server->getZoneServer()->getObject(target);

		if (sceno != nullptr && sceno->isCreatureObject()) {
			targetCreature = sceno.castTo<CreatureObject*>();
		}
	}

	const bool selfHealingAllowed = (allowedTarget & TARGET_SELF) || !isRemoteHeal;
	if (!isRemoteHeal || (targetCreature == nullptr && selfHealingAllowed) || (targetCreature != nullptr && (!targetCreature->isHealableBy(creature)) && selfHealingAllowed)) {
		isRemoteHeal = false;
		targetCreature = creature;
	}

	if (targetCreature == nullptr)
		return GENERALERROR;

	int retval = GENERALERROR;

	if (isRemoteHeal)
		retval = runCommandWithTarget(creature, targetCreature);
	else
		retval = runCommand(creature, targetCreature);

	return retval;
}

void ForceHealQueueCommand::handleJediHealArea(CreatureObject* creature, CreatureObject* animCreature, CreatureObject* areaCenter, int healingToDoDo) const {

	Zone* zone = creature->getZone();

	if (zone == nullptr)
		return;

	try {
		CloseObjectsVector* closeObjectsVector = (CloseObjectsVector*) areaCenter->getCloseObjects();
		SortedVector<QuadTreeEntry*> closeObjects;
		closeObjectsVector->safeCopyReceiversTo(closeObjects, CloseObjectsVector::CREOTYPE);

		for (int i = 0; i < closeObjects.size(); i++) {	
			SceneObject* object = static_cast<SceneObject*>( closeObjects.get(i));
			if (!object->isPlayerCreature() && !object->isPet())
				continue;
			if (object->isDroidObject())
			// if (object == areaCenter || object->isDroidObject())
				continue;
			if (areaCenter->getWorldPosition().distanceTo(object->getWorldPosition()) - object->getTemplateRadius() > areaHealRange)
				continue;
			CreatureObject* targetCreature = cast<CreatureObject*>( object);
			if (targetCreature->isAttackableBy(creature))
				continue;
			if (!targetCreature->isHealableBy(creature))
				continue;
			if (creature->isPlayerCreature() && object->getParentID() != 0 && creature->getParentID() != object->getParentID()) {
				Reference<CellObject*> targetCell = object->getParent().get().castTo<CellObject*>();
				if (targetCell != nullptr) {
					if (object->isPlayerCreature()) {
						auto perms = targetCell->getContainerPermissions();
						if (!perms->hasInheritPermissionsFromParent()) {
							if (!targetCell->checkContainerPermission(creature, ContainerPermissions::WALKIN))
								continue;
						}
					}
					ManagedReference<SceneObject*> parentSceneObject = targetCell->getParent().get();
					if (parentSceneObject != nullptr) {
						BuildingObject* buildingObject = parentSceneObject->asBuildingObject();
						if (buildingObject != nullptr && !buildingObject->isAllowedEntry(creature))
							continue;
					}
				}
			}
			if (creature != targetCreature && checkForArenaDuel(targetCreature))
				continue;


			try {
				Locker crossLocker(targetCreature, creature);
				if (checkjediHealAreaTarget(creature, targetCreature)) {
					// creature->doCombatAnimation(targetCreature, STRING_HASHCODE("force_intimidate_chain"), 0x01, 0xFF);
					if (animCreature != targetCreature) {
						animCreature->doCombatAnimation(targetCreature, animationCRC, 0, 0xFF);
						animCreature = targetCreature;
					}	
					if (healingToDoDo > 0) {
						if (targetCreature->hasDamage(CreatureAttribute::HEALTH)|| targetCreature->hasDamage(CreatureAttribute::ACTION) || targetCreature->hasDamage(CreatureAttribute::MIND)) {
							doAreaJediHealAction(animCreature, targetCreature, healingToDoDo);
						}
						if (isJediHealOverTime) {
							int hotHealing = healingToDoDo / 5.f;
							if (hotHealing > 0) {
								handleJediHealOverTime(targetCreature, healingToDoDo);
							}
						}
					}
				}
			} 
			catch (Exception& e) {}
		} 
	} 
	catch (Exception& e) {}
}

bool ForceHealQueueCommand::checkjediHealAreaTarget(CreatureObject* creature, CreatureObject* targetCreature)  const {
	// if (!targetCreature->hasDamage(CreatureAttribute::HEALTH) && !targetCreature->hasDamage(CreatureAttribute::ACTION) && !targetCreature->hasDamage(CreatureAttribute::MIND)) {
	// 	return false;
	// }

	// PlayerManager* playerManager = server->getPlayerManager();

	if (creature != targetCreature && !CollisionManager::checkLineOfSight(creature, targetCreature)) {
		return false;
	}

	if (targetCreature->isDead())
		return false;

	return true;
}

void ForceHealQueueCommand::handleJediHealOverTime(CreatureObject* targetCreature, int healing) const {
	// if (creature == targetCreature) {
	// 	creature->playEffect(clientEffect, "");
	// 	// creature->sendSystemMessage("You cast a heal over time effect on yourself");
	// }
	// else {
	// 	creature->doCombatAnimation(targetCreature, animationCRC, 0, 0xFF);
	// 	// creature->sendSystemMessage("You cast a heal over time effect");
	// 	targetCreature-> sendSystemMessage("A heal over time effect was cast on you");
	// }

	StringBuffer buff;
	buff << getName() << "healovertimetask";	
	Reference<HealOverTimeTask*> hotCheck = targetCreature->getPendingTask(buff.toString()).castTo<HealOverTimeTask*>();
	if(hotCheck != nullptr) {
		int ticks = hotCheck->getTicks();
		if ((ticks + hotTicks) > maxHotTicks) {
			hotCheck->setTicks(maxHotTicks);
		}
		else {
			hotCheck->setTicks(ticks + hotTicks);
		}

		int hotHealing = hotCheck->getHealing();
		if (hotHealing < healing)
			hotCheck->setHealing(healing);
	}
	else {
		Reference<HealOverTimeTask*> jediHealOverTimeTask = new HealOverTimeTask(targetCreature, healing, buff.toString(), hotTicks, hotTickFrequency);
		targetCreature->addPendingTask(buff.toString(), jediHealOverTimeTask, 3000);
	}
}

void ForceHealQueueCommand::doAreaJediHealAction(CreatureObject* creature, CreatureObject* targetCreature, int healingToDo) const {
	for (int i = 0; i < 3; i++) {
		// Attrib Values: Health = 1, Action = 2, Mind = 4
		if (attributesToHeal & (1 << i)) {
			uint8 attrib = i * 3;
			int curHam = targetCreature->getHAM(attrib);
			int maxHam = targetCreature->getMaxHAM(attrib) - targetCreature->getWounds(attrib);
			int amtToHeal = maxHam - curHam;

			healingToDo *= targetCreature->calculateBFRatio();

			if (amtToHeal > 0) {
				targetCreature->healDamage(creature, attrib, healingToDo, true);
				// sendHealMessage(creature, targetCreature, HEAL_DAMAGE, attrib, amtToHeal);
			}
		}
	}
}