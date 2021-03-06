#pragma once
#include "library/sp.h"

#include <random>
#include <memory>
#include <vector>
#include "library/strings.h"
#include "game/organisation.h"
#include "game/base/base.h"

namespace OpenApoc
{

class City;
class Framework;
class Rules;

class GameState
{
  private:
	sp<Organisation> player;

  public:
	GameState(Framework &fw, Rules &rules);
	std::unique_ptr<City> city;

	std::map<UString, sp<Organisation>> organisations;
	std::vector<sp<Base>> playerBases;

	bool showTileOrigin;
	bool showVehiclePath;
	bool showSelectableBounds;

	std::default_random_engine rng;

	UString getPlayerBalance() const;
	sp<Organisation> getOrganisation(const UString &orgID);
	sp<Organisation> getPlayer() const;
};

}; // namespace OpenApoc
