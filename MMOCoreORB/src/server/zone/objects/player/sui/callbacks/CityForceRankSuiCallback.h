
#ifndef CITYFORCERANKSUICALLBACK_H_
#define CITYFORCERANKSUICALLBACK_H_

#include "server/zone/objects/player/sui/SuiCallback.h"

namespace server {
namespace zone {
namespace objects {
namespace creature {
	class CreatureObject;
}
namespace region {
	class CityRegion;
}
}
}
}

using namespace server::zone::objects::creature;
using namespace server::zone::objects::region;

class CityForceRankSuiCallback : public SuiCallback {
	ManagedWeakReference<CityRegion*> cityRegion;
	bool rankUp;

public:
	CityForceRankSuiCallback(ZoneServer* server, CityRegion* city, bool rank) : SuiCallback(server) {
		cityRegion = city;
		rankUp = rank;
	}

	void run(CreatureObject* player, SuiBox* suiBox, uint32 eventIndex, Vector<UnicodeString>* args) {
		bool cancelPressed = (eventIndex == 1);

		ManagedReference<CityRegion*> city = cityRegion.get();

		if (city == nullptr)
			return;

		if (!suiBox->isMessageBox() || player == nullptr || cancelPressed) {
			return;
		}

		PlayerObject* ghost = player->getPlayerObject();

		if (ghost == nullptr)
			return;

		if (!ghost->isPrivileged())
			return;

		Locker clocker(city, player);

		StringIdChatParameter params("city/city", "force_city_rank"); // Forcing a city rank change for city %TO.
		params.setTO(city->getCityRegionName());
		player->sendSystemMessage(params);

		CityManager* cityManager = server->getCityManager();

		if (rankUp)
			cityManager->expandCity(city);
		else
			cityManager->contractCity(city);
	}
};

#endif /* CITYFORCERANKSUICALLBACK_H_ */
