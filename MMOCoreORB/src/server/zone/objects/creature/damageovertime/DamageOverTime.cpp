/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions. */


#include "templates/params/creature/CreatureAttribute.h"
#include "templates/params/creature/CreatureState.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/creature/commands/effect/CommandEffect.h"
#include "DamageOverTime.h"
#include "server/zone/ZoneServer.h"
#include "server/zone/managers/combat/CombatManager.h"
#include "server/zone/objects/player/PlayerObject.h"

DamageOverTime::DamageOverTime() {
	setAttackerID(0);
	setType(CreatureState::BLEEDING);
	setAttribute(CreatureAttribute::HEALTH);
	strength = 0;
	setDuration(0);
	setExpires(Time((uint32) 0));
	setSecondaryStrength(0);
	addSerializableVariables();
}

DamageOverTime::DamageOverTime(CreatureObject* attacker,
		uint64 tp,
		uint8 attrib,
		uint32 str,
		uint32 dur,
		int secondaryStrength) {

	if (attacker != nullptr)
		setAttackerID(attacker->getObjectID());

	setType(tp);
	setAttribute(attrib);
	strength = str;
	setDuration(dur);
	setSecondaryStrength(secondaryStrength);
	applied.updateToCurrentTime();
	activate();

	addSerializableVariables();
}

DamageOverTime::DamageOverTime(const DamageOverTime& dot) : Object(), Serializable() {
	addSerializableVariables();

	attackerID = dot.attackerID;
	type = dot.type;
	attribute = dot.attribute;
	strength = dot.strength;
	duration = dot.duration;
	applied = dot.applied;
	expires = dot.expires;
	nextTick = dot.nextTick;
	secondaryStrength = dot.secondaryStrength;

}

DamageOverTime& DamageOverTime::operator=(const DamageOverTime& dot) {
	if (this == &dot)
		return *this;

	attackerID = dot.attackerID;
	type = dot.type;
	attribute = dot.attribute;
	strength = dot.strength;
	duration = dot.duration;
	applied = dot.applied;
	expires = dot.expires;
	nextTick = dot.nextTick;
	secondaryStrength = dot.secondaryStrength;


	return *this;
}

void DamageOverTime::addSerializableVariables() {
	addSerializableVariable("attackerID", &attackerID);
	addSerializableVariable("type", &type);
	addSerializableVariable("attribute", &attribute);
	addSerializableVariable("strength", &strength);
	addSerializableVariable("duration", &duration);
	addSerializableVariable("applied", &applied);
	addSerializableVariable("expires", &expires);
	addSerializableVariable("nextTick", &nextTick);
	addSerializableVariable("secondaryStrength", &secondaryStrength);

}

void to_json(nlohmann::json& j, const DamageOverTime& t) {
	j["attackerID"] = t.attackerID;
	j["type"] = t.type;
	j["attribute"] = t.attribute;
	j["strength"] = t.strength;
	j["duration"] = t.duration;
	j["applied"] = t.applied;
	j["expires"] = t.expires;
	j["nextTick"] = t.nextTick;
	j["secondaryStrength"] = t.secondaryStrength;
}

void DamageOverTime::activate() {
	expires.updateToCurrentTime();
	expires.addMiliTime(duration * 1000);
}

uint32 DamageOverTime::applyDot(CreatureObject* victim) {
	if (expires.isPast() || !nextTick.isPast())
		return 0;

	nextTick.updateToCurrentTime();

	uint32 power = 0;
	ManagedReference<CreatureObject*> attacker = victim->getZoneServer()->getObject(attackerID).castTo<CreatureObject*>();

	if (attacker == nullptr)
		attacker = victim;

	switch(type) {
	case CreatureState::BLEEDING:
		power = doBleedingTick(victim, attacker);
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::POISONED:
		power = doPoisonTick(victim, attacker);
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::DISEASED:
		power = doDiseaseTick(victim, attacker);
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::ONFIRE:
		power = doFireTick(victim, attacker);
		nextTick.addMiliTime(3000);
		break;
	case CommandEffect::FORCECHOKE:
		power = doForceChokeTick(victim, attacker);
		nextTick.addMiliTime(3000);
		break;
	}

	return power;
}

uint32 DamageOverTime::initDot(CreatureObject* victim, CreatureObject* attacker) {
	uint32 power = 0;
	int absorptionMod = 0;
	nextTick.updateToCurrentTime();

	switch(type) {
	case CreatureState::BLEEDING:
		absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_bleeding")));
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::ONFIRE:
		absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_fire")));
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::POISONED:
		absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_poison")));
		nextTick.addMiliTime(3000);
		break;
	case CreatureState::DISEASED:
		absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_disease")));
		nextTick.addMiliTime(3000);
		break;
	case CommandEffect::FORCECHOKE:
		nextTick.addMiliTime(3000);
		strength *= ((100 - System::random(20)) * 0.01f);
		victim->showFlyText("combat_effects", "choke", 0xFF, 0, 0);

		break;
	}

	power = (uint32)(strength * (1.f - absorptionMod / 100.f));

	//victim->addDamage(attacker,1);

	return power;
}

uint32 DamageOverTime::doBleedingTick(CreatureObject* victim, CreatureObject* attacker) {
	// TODO: Do we try to resist again?
	// we need to allow dots to tick while incapped, but not do damage
	if (victim->isIncapacitated() && victim->isFeigningDeath() == false)
		return 0;

	uint32 attr = victim->getHAM(attribute);
	int absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_bleeding")));

	// absorption reduces the strength of a dot by the given %.
	int damage = (int)(strength * (1.f - absorptionMod / 100.f));

	if (victim->isPlayerCreature()) {
		ManagedReference<PlayerObject*> victimPlayer = victim->getPlayerObject();
		if (victimPlayer != nullptr) {
			float kinRes = victimPlayer->getKineticResistance();
			if (kinRes > 0)
				damage *= kinRes;
			float bleedRes = victimPlayer->getBleedResistance();
			if (bleedRes > 0)
				damage *= bleedRes;
		}
	}
	else if (victim->isAiAgent()) {
		ManagedReference<AiAgent*> npc = cast<AiAgent*>(victim);
		if (npc != nullptr) {
			damage *= (1 - npc->getKinetic() / 100.f);
			if (victim->isPet()) {
				petResistReduction(victim, damage);
			}
		}
	}

	if (attr < damage) {
		//System::out << "setting strength to " << attr -1 << endl;
		damage = attr - 1;
	}

	Reference<CreatureObject*> attackerRef = attacker;
	Reference<CreatureObject*> victimRef = victim;
	auto attribute = this->attribute;

	Core::getTaskManager()->executeTask([victimRef, attackerRef, attribute, damage] () {
		Locker locker(victimRef);

		Locker crossLocker(attackerRef, victimRef);

		victimRef->inflictDamage(attackerRef, attribute, damage, false, "dotDMG", true, false);

		if (victimRef->hasAttackDelay())
			victimRef->removeAttackDelay();

		victimRef->playEffect("clienteffect/dot_bleeding.cef","");
	}, "BleedTickLambda");

	return damage;
}

uint32 DamageOverTime::doFireTick(CreatureObject* victim, CreatureObject* attacker) {
	// we need to allow dots to tick while incapped, but not do damage
	if (victim->isIncapacitated() && victim->isFeigningDeath() == false)
		return 0;

	uint32 attr = victim->getHAM(attribute);
	int absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_fire")));

	// absorption reduces the strength of a dot by the given %.
	int damage = (int)(strength * (1.f - absorptionMod / 100.f));
	if (victim->isPlayerCreature()) {
		ManagedReference<PlayerObject*> victimPlayer = victim->getPlayerObject();
		if (victimPlayer != nullptr) {
			float heatRes = victimPlayer->getHeatResistance();
			if (heatRes > 0)
				damage *= heatRes;
			float fireRes = victimPlayer->getFireResistance();
			if (fireRes > 0)
				damage *= fireRes;
		}
	}
	else if (victim->isAiAgent()) {
		ManagedReference<AiAgent*> npc = cast<AiAgent*>(victim);
		if (npc != nullptr) {
			damage *= (1 - npc->getHeat() / 100.f);
			if (victim->isPet()) {
				petResistReduction(victim, damage);
			}
		}
	}

	if (attr < damage) {
		//System::out << "setting strength to " << attr -1 << endl;
		damage = attr - 1;
	}

	// int woundsToApply = (int)(secondaryStrength * ((100.f + victim->getShockWounds()) / 100.0f));
	// int maxWoundsToApply = victim->getBaseHAM(attribute) - 1 - victim->getWounds(attribute);

	// woundsToApply = Math::min(woundsToApply, maxWoundsToApply);
	int woundsToApply = 0;

	Reference<CreatureObject*> attackerRef = attacker;
	Reference<CreatureObject*> victimRef = victim;
	auto attribute = this->attribute;
	auto secondaryStrength = this->secondaryStrength;

	Core::getTaskManager()->executeTask([victimRef, attackerRef, attribute, woundsToApply, secondaryStrength, damage] () {
		Locker locker(victimRef);

		Locker crossLocker(attackerRef, victimRef);

		// if (woundsToApply > 0) {
		// 	// need to do damage to account for wounds first, or it will possibly get
		// 	// applied twice
		// 	if (attribute % 3 == 0)
		// 		victimRef->inflictDamage(attackerRef, attribute, woundsToApply, true, "dotDMG", true, false);

		// 	victimRef->addWounds(attribute, woundsToApply, true, false);
		// }

		//victimRef->addShockWounds((int)(secondaryStrength * 0.075f));

		victimRef->inflictDamage(attackerRef, attribute - attribute % 3, damage, true, "dotDMG", true, false);
		if (victimRef->hasAttackDelay())
			victimRef->removeAttackDelay();

		victimRef->playEffect("clienteffect/dot_fire.cef","");
	}, "FireTickLambda");

	return damage;
}

uint32 DamageOverTime::doPoisonTick(CreatureObject* victim, CreatureObject* attacker) {
	// we need to allow dots to tick while incapped, but not do damage
	if (victim->isIncapacitated() && victim->isFeigningDeath() == false)
		return 0;

	uint32 attr = victim->getHAM(attribute);
	int absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_poison")));

	// absorption reduces the strength of a dot by the given %.
	int damage = (int)(strength * (1.f - absorptionMod / 100.f));

	if (victim->isPlayerCreature()) {
		ManagedReference<PlayerObject*> victimPlayer = victim->getPlayerObject();
		if (victimPlayer != nullptr) {
			float acidRes = victimPlayer->getAcidResistance();
			if (acidRes > 0)
				damage *= acidRes;
			float poisonRes = victimPlayer->getPoisonResistance();
			if (poisonRes > 0)
				damage *= poisonRes;
		}
	}
	else if (victim->isAiAgent()) {
		ManagedReference<AiAgent*> npc = cast<AiAgent*>(victim);
		if (npc != nullptr) {
			damage *= (1 - npc->getAcid() / 100.f);
			if (victim->isPet()) {
				petResistReduction(victim, damage);
			}
		}
	}

	if (attr < damage) {
		//System::out << "setting strength to " << attr -1 << endl;
		damage = attr - 1;
	}

	Reference<CreatureObject*> attackerRef = attacker;
	Reference<CreatureObject*> victimRef = victim;
	auto attribute = this->attribute;

	Core::getTaskManager()->executeTask([victimRef, attackerRef, attribute, damage] () {
		Locker locker(victimRef);

		Locker crossLocker(attackerRef, victimRef);

		victimRef->inflictDamage(attackerRef, attribute, damage, false, "dotDMG", true, false);
		if (victimRef->hasAttackDelay())
			victimRef->removeAttackDelay();

		victimRef->playEffect("clienteffect/dot_poisoned.cef","");
	}, "PoisonTickLambda");

	return damage;
}

uint32 DamageOverTime::doDiseaseTick(CreatureObject* victim, CreatureObject* attacker) {
	// we need to allow dots to tick while incapped, but not do damage
	if (victim->isIncapacitated() && victim->isFeigningDeath() == false)
		return 0;

	int absorptionMod = Math::max(0, Math::min(50, victim->getSkillMod("absorption_disease")));

	// absorption reduces the strength of a dot by the given %.
	// make sure that the CM dots modify the strength
	int damage = (int)(strength * (1.f - absorptionMod / 100.f) * (1.f + victim->getShockWounds() / 100.0f));

	if (victim->isPlayerCreature()) {
		ManagedReference<PlayerObject*> victimPlayer = victim->getPlayerObject();
		if (victimPlayer != nullptr) {
			float acidRes = victimPlayer->getAcidResistance();
			if (acidRes > 0)
				damage *= acidRes;
			float diseaseRes = victimPlayer->getDiseaseResistance();
			if (diseaseRes > 0)
				damage *= diseaseRes;
		}
	}
	else if (victim->isAiAgent()) {
		ManagedReference<AiAgent*> npc = cast<AiAgent*>(victim);
		if (npc != nullptr) {
			damage *= (1 - npc->getAcid() / 100.f);
			if (victim->isPet()) {
				petResistReduction(victim, damage);
			}
		}
	}
	
	int maxDamage = victim->getBaseHAM(attribute) - 1 - victim->getWounds(attribute);

	damage = Math::min(damage, maxDamage);

	Reference<CreatureObject*> attackerRef = attacker;
	Reference<CreatureObject*> victimRef = victim;
	auto attribute = this->attribute;
	auto strength = this->strength;

	Core::getTaskManager()->executeTask([victimRef, attackerRef, attribute, damage, strength] () {
		Locker locker(victimRef);
		Locker crossLocker(attackerRef, victimRef);

		if ((int)damage > 0) {
			// need to do damage to account for wounds first, or it will possibly get
			// applied twice
			if (attribute % 3 == 0)
				victimRef->inflictDamage(attackerRef, attribute, damage, true, "dotDMG", true, false);

			//victimRef->addWounds(attribute, damage, true, false);
		}

		//victimRef->addShockWounds((int)(strength * 0.075f));

		if (victimRef->hasAttackDelay())
			victimRef->removeAttackDelay();

		victimRef->playEffect("clienteffect/dot_diseased.cef","");
	}, "DiseaseTickLambda");

	return damage;
}

uint32 DamageOverTime::doForceChokeTick(CreatureObject* victim, CreatureObject* attacker) {
	// we need to allow dots to tick while incapped, but not do damage
	if (victim->isIncapacitated() && victim->isFeigningDeath() == false)
		return 0;

	Reference<CreatureObject*> attackerRef = attacker;
	Reference<CreatureObject*> victimRef = victim;
	auto attribute = this->attribute;
	auto strength = this->strength;

	if (victimRef->isAiAgent()) {
		ManagedReference<AiAgent*> npc = cast<AiAgent*>(victim);
		if (npc != nullptr) {
			strength *= (1 - npc->getLightSaber() / 100.f);
			if (victim->isPet()) {
				petResistReduction(victim, strength);
			}
		}
	}
	else if (victimRef->isPlayerCreature()) {
		ManagedReference<PlayerObject*> victimPlayer = victimRef->getPlayerObject();
		if (victimPlayer != nullptr) {
			float acidRes = victimPlayer->getLightsaberResistance();
			if (acidRes > 0)
				strength *= acidRes;
			float diseaseRes = victimPlayer->getChokeResistance();
			if (diseaseRes > 0)
				strength *= diseaseRes;
		}
	}

	Core::getTaskManager()->executeTask([victimRef, attackerRef, attribute, strength] () {
		Locker locker(victimRef);

		Locker crossLocker(attackerRef, victimRef);

		uint32 chokeDam = strength;
		
		float jediBuffDamage = 0;
		float rawDamage = chokeDam;

		// Force Shield
		int forceShield = victimRef->getSkillMod("force_shield");
		if (forceShield > 0) {
			jediBuffDamage = rawDamage - (chokeDam *= 1.f - (forceShield / 100.f));
			victimRef->notifyObservers(ObserverEventType::FORCESHIELD, attackerRef, jediBuffDamage);
			CombatManager::instance()->sendMitigationCombatSpam(victimRef, nullptr, (int)jediBuffDamage, CombatManager::FORCESHIELD);
		}
		// //PSG with lightsaber resistance only
		// ManagedReference<ArmorObject*> psg = CombatManager::instance()->getPSGArmor(victimRef);
		// if (psg != nullptr && !psg->isVulnerable(SharedWeaponObjectTemplate::LIGHTSABER)) {
		// 	float armorReduction =  CombatManager::instance()->getArmorObjectReduction(psg, SharedWeaponObjectTemplate::LIGHTSABER);

		// if (armorReduction > 0)
		// 	chokeDam *= 1.f - (armorReduction / 100.f);

		// }

		CombatManager::instance()->broadcastCombatSpam(attackerRef, victimRef, nullptr, chokeDam, "cbt_spam", "forcechoke_hit", 1);
		victimRef->inflictDamage(attackerRef, attribute, chokeDam, true, "dotDMG", true, false);

		if (victimRef->hasAttackDelay())
			victimRef->removeAttackDelay();

		victimRef->playEffect("clienteffect/pl_force_choke.cef", "");
		victimRef->sendSystemMessage("@combat_effects:choke_single");
		victimRef->showFlyText("combat_effects", "choke", 0xFF, 0, 0);
	}, "ForceChokeTickLambda");

	return strength;

}

float DamageOverTime::reduceTick(float reduction) {
	//System::out << "reducing tick with reduction " << reduction << endl;
	if (reduction < 0.f) // this ensures we can't increase a dot strength
		return reduction;

	if (reduction >= strength) {
		expireTick();
		return reduction - strength;
	} else {
		//System::out << "strength before dotRed " << strength << endl;
		strength -= reduction;
		//System::out << "strength after dotRed " << strength << endl;
	}

	return 0.f;
}

void DamageOverTime::multiplyDuration(float multiplier) {
	Time newTime;
	uint64 timeToAdd = (expires.getMiliTime() - newTime.getMiliTime()) * multiplier;
	newTime.addMiliTime(timeToAdd);
	expires = newTime;

}

void DamageOverTime::petResistReduction(CreatureObject* pet, int damage) {
	ManagedReference<CreatureObject*> petMasterA = pet->getLinkedCreature().get();
	if (petMasterA != nullptr) {
		int res = petMasterA->getSkillMod("pet_dot_resistance");
		if (res > 0) {
			damage *= (1 + res / 100.f);
		}
	}	
}