/*
 * DespawnCreatureTask.h
 *
 *  Created on: 12/07/2010
 *      Author: victor
 */

#ifndef DESPAWNCREATURETASK_H_
#define DESPAWNCREATURETASK_H_

#include "server/zone/objects/creature/ai/AiAgent.h"
#include "server/zone/Zone.h"

class DespawnCreatureTask : public Task {
	ManagedReference<AiAgent*> creature;

public:
	DespawnCreatureTask(AiAgent* cr) {
		creature = cr;

		auto zone = cr->getZone();

		if (zone != nullptr) {
			setCustomTaskQueue(zone->getZoneName());
		}
	}

	void run() {

		if (creature != nullptr) {
			Locker locker(creature);

			Zone* zone = creature->getZone();


			//hopefully this ugly derp will fix despawning o.o
			if (creature->getPendingTask("despawnSecurity") != nullptr)
				creature->removePendingTask("despawnSecurity");
			
			if (creature->getPendingTask("ninjaDespawn") != nullptr)
				creature->removePendingTask("ninjaDespawn");

			if (creature->getPendingTask("despawn") != nullptr)
				creature->removePendingTask("despawn");

			if (zone == nullptr)
				return;

			creature->destroyObjectFromWorld(false);
			creature->notifyDespawn(zone);
		}
	}
};


#endif /* DESPAWNCREATURETASK_H_ */