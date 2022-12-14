/*
 * PlaceCityHallSuiCallback.h
 *
 *  Created on: Feb 5, 2012
 *      Author: xyborn
 */

#ifndef PLACECITYHALLSUICALLBACK_H_
#define PLACECITYHALLSUICALLBACK_H_

#include "server/zone/objects/player/sui/SuiCallback.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/tangible/deed/structure/StructureDeed.h"
#include "server/zone/managers/structure/StructureManager.h"
#include "server/zone/managers/planet/PlanetManager.h"
#include "server/zone/managers/name/NameManager.h"
#include "server/zone/managers/city/CityManager.h"
#include "server/zone/Zone.h"
#include "server/zone/ZoneProcessServer.h"

class PlaceCityHallSuiCallback : public SuiCallback, public Logger {
	ManagedWeakReference<Zone*> zone;
	ManagedWeakReference<StructureDeed*> deed;

	float x;
	float y;
	int angle;

public:
	PlaceCityHallSuiCallback(Zone* zone, StructureDeed* deed, float x, float y, int angle)
			: SuiCallback(zone->getZoneServer()) {

		this->zone = zone;
		this->deed = deed;
		this->x = x;
		this->y = y;
		this->angle = angle;

		setLoggingName("PlaceCityHallSuiCallback");
	}

	void run(CreatureObject* creature, SuiBox* sui, uint32 eventIndex, Vector<UnicodeString>* args) {
		bool cancelPressed = (eventIndex == 1);

		if (!sui->isInputBox() || cancelPressed || creature == nullptr)
			return;

		if (args->size() < 0)
			return;

		ManagedReference<Zone*> zone = this->zone.get();

		if (zone == nullptr)
			return;

		String cityName = args->get(0).toString();

		ZoneProcessServer* zps = creature->getZoneProcessServer();
		NameManager* nameManager = zps->getNameManager();

		int nameResult = nameManager->validateCityName(cityName);

		switch (nameResult) {
		case NameManagerResult::DECLINED_PROFANE:
		case NameManagerResult::DECLINED_DEVELOPER:
		case NameManagerResult::DECLINED_FICT_RESERVED:
		case NameManagerResult::DECLINED_RESERVED:
			creature->sendSystemMessage("@player_structure:obscene"); //That name was rejected by the name filter. Try a different name.
			return;
			break;

		case NameManagerResult::DECLINED_RACE_INAPP:
		case NameManagerResult::DECLINED_EMPTY:
		case NameManagerResult::DECLINED_SYNTAX:
			creature->sendSystemMessage("@player_structure:not_valid_name"); // That is not a valid name.
			return;
			break;
		case NameManagerResult::ACCEPTED:
			break;
		}

		//Check unique
		PlanetManager* planetManager = zone->getPlanetManager();
		CityManager* cityManager = server->getCityManager();

		if (planetManager == nullptr || cityManager == nullptr) {
			return;
		}

		try {
			Locker locker(cityManager);

			if (!cityManager->validateCityInRange(creature, zone, x, y) || !planetManager->validateClientCityInRange(creature, x, y)) {
				return;
			}

			if (!planetManager->validateRegionName(cityName) || !cityManager->validateCityName(cityName)) {
				creature->sendSystemMessage("@player_structure:cityname_not_unique"); //Another city already has this name. Your city's name must be unique.
				//Resend the sui.
				return;
			}

			int result = StructureManager::instance()->placeStructureFromDeed(creature, deed.get(), x, y, angle);

			if (result == 0) {
				cityManager->createCity(creature, cityName, x, y);
			}
		} catch (const Exception& e) {
			e.printStackTrace();
		}
	}
};

#endif /* PLACECITYHALLSUICALLBACK_H_ */
