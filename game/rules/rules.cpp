#include "game/rules/rules.h"
#include "game/rules/rules_private.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include <tinyxml2.h>
#include "game/ufopaedia/ufopaedia.h"

namespace OpenApoc
{

Rules::Rules(Framework &fw, const UString &rootFileName)
{
	TRACE_FN_ARGS1("rootFileName", rootFileName);
	UString systemPath;
	auto file = fw.data->fs.open(rootFileName);
	if (!file)
	{
		LogError("Failed to find rule file \"%s\"", rootFileName.c_str());
		return;
	}
	systemPath = file.systemPath();

	tinyxml2::XMLDocument doc;
	doc.LoadFile(systemPath.c_str());
	tinyxml2::XMLElement *root = doc.RootElement();
	if (!root)
	{
		LogError("Failed to parse rule file \"%s\"", systemPath.c_str());
		return;
	}

	UString nodeName = root->Name();
	if (nodeName != "openapoc_rules")
	{
		LogError("Unexpected root node \"%s\" in \"%s\" - expected \"%s\"", nodeName.c_str(),
		         systemPath.c_str(), "openapoc_rules");
	}

	UString rulesetName = root->Attribute("name");
	LogInfo("Loading ruleset \"%s\" from \"%s\"", rulesetName.c_str(), systemPath.c_str());

	if (!RulesLoader::ParseRules(fw, *this, root))
	{
		LogError("Error loading ruleset \"%s\" from \"%s\"", rulesetName.c_str(),
		         systemPath.c_str());
	}

	/* Post-processing/checks go here */
	for (auto &p : this->buildingTiles)
	{
		auto &sceneryTile = p.second;
		if (sceneryTile.damagedTileID != "")
		{
			auto damagedTileIt = this->buildingTiles.find(sceneryTile.damagedTileID);
			if (damagedTileIt == this->buildingTiles.end())
			{
				LogError("Tile \"%s\" has damaged tile ID \"%s\" which does not exist",
				         p.first.c_str(), sceneryTile.damagedTileID.c_str());
				continue;
			}
			sceneryTile.damagedTile = &damagedTileIt->second;
		}
	}
}

bool RulesLoader::ParseRules(Framework &fw, Rules &rules, tinyxml2::XMLElement *root)
{
	TRACE_FN;
	UString nodeName = root->Name();
	if (nodeName != "openapoc_rules")
	{
		LogError("Unexpected root node \"%s\" - expected \"%s\"", nodeName.c_str(),
		         "openapoc_rules");
		return false;
	}

	for (tinyxml2::XMLElement *e = root->FirstChildElement(); e != nullptr;
	     e = e->NextSiblingElement())
	{
		UString name = e->Name();
		if (name == "vehicledef")
		{
			if (!ParseVehicleDefinition(fw, rules, e))
				return false;
		}
		else if (name == "organisation")
		{
			if (!ParseOrganisationDefinition(fw, rules, e))
				return false;
		}
		else if (name == "city")
		{
			if (!ParseCityDefinition(fw, rules, e))
				return false;
		}
		else if (name == "weapon")
		{
			if (!ParseWeaponDefinition(fw, rules, e))
				return false;
		}
		else if (name == "facilitydef")
		{
			if (!ParseFacilityDefinition(fw, rules, e))
				return false;
		}
		else if (name == "ufopaedia")
		{
			tinyxml2::XMLElement *nodeufo;
			UString nodename;
			for (nodeufo = e->FirstChildElement(); nodeufo != nullptr;
			     nodeufo = nodeufo->NextSiblingElement())
			{
				nodename = nodeufo->Name();
				if (nodename == "category")
				{
					Ufopaedia::UfopaediaDB.push_back(
					    std::make_shared<UfopaediaCategory>(fw, nodeufo));
				}
			}
		}
		else if (name == "include")
		{
			UString rootFileName = e->GetText();
			UString systemPath;
			auto file = fw.data->fs.open(rootFileName);
			if (!file)
			{
				LogError("Failed to find included rule file \"%s\"", rootFileName.c_str());
				return false;
			}
			systemPath = file.systemPath();
			TRACE_FN_ARGS1("include", systemPath);
			LogInfo("Loading included ruleset from \"%s\"", systemPath.c_str());
			tinyxml2::XMLDocument doc;
			doc.LoadFile(systemPath.c_str());
			tinyxml2::XMLElement *incRoot = doc.RootElement();
			if (!incRoot)
			{
				LogError("Failed to parse included rule file \"%s\"", systemPath.c_str());
				return false;
			}
			if (!ParseRules(fw, rules, incRoot))
			{
				LogError("Error loading included ruleset \"%s\"", systemPath.c_str());
				return false;
			}
		}
		else if (name == "doodad")
		{
			if (!ParseDoodadDefinition(fw, rules, e))
				return false;
		}
		else
		{
			LogError("Unexpected node \"%s\"", name.c_str());
			return false;
		}
	}

	return true;
}

const UString &Rules::getSceneryTileAt(Vec3<int> offset) const
{
	static const UString noTile = "";
	if (offset.x < 0 || offset.x >= citySize.x || offset.y < 0 || offset.y >= citySize.y ||
	    offset.z < 0 || offset.z >= citySize.z)
	{
		LogError("Trying to get tile {%d,%d,%d} in city of size {%d,%d,%d}", offset.x, offset.y,
		         offset.z, citySize.x, citySize.y, citySize.z);
		return noTile;
	}
	unsigned index = offset.z * citySize.x * citySize.y + offset.y * citySize.x + offset.x;
	if (index >= tileIDs.size())
	{
		LogError("Tile {%d,%d,%d} would go over ID array! (city size {%d,%d,%d}, id offset %u, "
		         "id array size %u",
		         offset.x, offset.y, offset.z, citySize.x, citySize.y, citySize.z, index,
		         tileIDs.size());
		return noTile;
	}
	return tileIDs[index];
}

}; // namespace OpenApoc
