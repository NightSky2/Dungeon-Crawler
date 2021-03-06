#pragma once

#include <iostream>
#include <json/json.h>

namespace MyJson
{
	bool checkValid(Json::Value &value, std::string member);

	Json::Value get(Json::Value &value, std::string member);


}
