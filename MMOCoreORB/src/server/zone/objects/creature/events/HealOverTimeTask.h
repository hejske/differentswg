#ifndef HEALOVERTIMETASK_H_
#define HEALOVERTIMETASK_H_

#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/creature/commands/QueueCommand.h"


class HealOverTimeTask : public Task {
	ManagedReference<CreatureObject*> targetCreature;
	int healing;
	int amountOfTicks;
	int tickFrequency;
    String name;
	//int actualHealing;

public:

	HealOverTimeTask(CreatureObject* creo, int healAmount, String cName, int ticks, int frequency) { // This is the healer
		targetCreature = creo; // This is the target (one that receives healing)
        healing = healAmount;
		amountOfTicks = ticks;
		tickFrequency = frequency;
        name = cName;
	}
	void run() {
		Locker locker(targetCreature);
		if (targetCreature != nullptr) {
			if (amountOfTicks > 0 && !targetCreature->isDead() && !targetCreature->isIncapacitated()) {
				//actualHealing = healing;
				// if (targetCreature->isPlayerCreature()) {
				// 	ManagedReference<PlayerObject*> ghost = targetCreature->getPlayerObject();
				// 	if (ghost != nullptr) {
				// 		if (ghost->hasTef()) {
				// 			actualHealing /=2;
				// 		}
				// 		if (!targetCreature->hasBuff(STRING_HASHCODE("saber_dw_light_stance"))) {
				// 			actualHealing /= 2;
				// 		}
				// 	}
				// }

				targetCreature->healDamage(targetCreature, 0, healing, true);
				//targetCreature->healDamage(targetCreature, 3, healing, true);
				//	targetCreature->healDamage(targetCreature, 6, healing, true);					
				targetCreature->playEffect("clienteffect/healing_healdamage.cef", "");
                amountOfTicks--;
				this->reschedule(tickFrequency * 1000); // Reschedule in 3 seconds...
			}
			else {
				targetCreature->removePendingTask(name);
			}
		}
	}

    void setTicks(int val) {
        amountOfTicks = val;
    }

    int getTicks() {
        return amountOfTicks;
    }

    void setHealing(int val) {
        healing = val;
    }

    int getHealing() {
        return healing;
    }

};


#endif