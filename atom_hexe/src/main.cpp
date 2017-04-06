#include "tinyxml2/tinyxml2.h"
#include "json/json.hpp"
#include "xml2json/xml2json.hpp"

#include <regex>

using json = nlohmann::json;

const std::string g_InputDir = "I:\\Y2016C-Y2-PROGTeam04\\documentation\\doxygen\\output\\xml\\";
const std::string g_OutputDir = "I:\\Y2016C-Y2-PROGTeam04\\documentation\\doxygen\\output\\json\\";
const json g_ParamValues = {
  {"const char *", "string"},
  {"const char*", "string"},
  {"bool", "boolean"},
  {"int", "number"},
  {"unsigned int", "number"},
  {"float", "number"},
  {"ScriptHandle", "pointer"},
  {"glm,,vec2", "table"},
  {"glm,,vec3", "table"},
  {"ScriptTable", "table"},
  {"SmartScriptTable", "table"},
  {"hexe,,gameplay,,tile,,TileCoord", "table"},
  {"gameplay,,tile,,TileCoord", "table"},
  {"tile,,TileCoord", "table"},
  {"TileCoord", "table"},
  {"hexe,,component,,Entity", "number"},
  {"component,,Entity", "number"},
  {"Entity", "number"},
  {"KeyCode", "number"},
  {"input,,KeyCode", "number"},
  {"hexe,,input,,KeyCode", "number"},
  {"uint32_t", "number"}
};
const std::string g_EnginePrefix = "hexe::service::scripts::scriptbinds::ScriptBind_";
const std::string g_GamePrefix = "hexegame::scriptbinds::ScriptBind_";

void XmlErrorCheck(const tinyxml2::XMLError a_LoadResult, tinyxml2::XMLDocument* a_XmlDoc) {
  if (a_LoadResult != tinyxml2::XML_SUCCESS) {
    printf("Couldn't load index XML file. Error %s\n", a_XmlDoc->ErrorName());
    a_XmlDoc->ClearError();
  }
  else {
    printf("XML file load successful\n");
  }
}

int main(int argc, char* argv[]) {
  tinyxml2::XMLDocument xmlDoc{};
  tinyxml2::XMLError xmlLoadResult = xmlDoc.LoadFile((g_InputDir + "index.xml").c_str());

  XmlErrorCheck(xmlLoadResult, &xmlDoc);

  tinyxml2::XMLPrinter xmlStr;
  xmlDoc.Print(&xmlStr);
  std::string xmlJsonStr = xml2json(xmlStr.CStr());

  std::vector<std::string> fileNames{};

  json jsonFinal{};
  json jsonTmp = json::parse(xmlJsonStr.c_str());

  json scriptbinds = jsonTmp["doxygenindex"]["compound"];

  for (auto& scriptbind : scriptbinds) {
    auto enginePrefixPos = scriptbind["name"].get<std::string>().find(g_EnginePrefix);
    auto gamePrefixPos = scriptbind["name"].get<std::string>().find(g_GamePrefix);

    if (enginePrefixPos != std::string::npos ||
        gamePrefixPos != std::string::npos) {
      fileNames.push_back(scriptbind["@refid"].get<std::string>() + ".xml");

      std::string name{};
      if (enginePrefixPos != std::string::npos) {
        auto namePos = scriptbind["name"].get<std::string>().find("_");
        name = scriptbind["name"].get<std::string>().replace(enginePrefixPos, namePos + 1, "");
      }
      else if (gamePrefixPos != std::string::npos) {
        auto namePos = scriptbind["name"].get<std::string>().find("_");
        name = scriptbind["name"].get<std::string>().replace(gamePrefixPos, namePos + 1, "");
      }

      jsonFinal["scriptbinds"][name] = {
        {"description", ""},
        {"methods", json::object()}
      };
    }
  }

  xmlDoc.Clear();
  for (auto& fileName : fileNames) {
    xmlLoadResult = xmlDoc.LoadFile((g_InputDir + fileName).c_str());

    XmlErrorCheck(xmlLoadResult, &xmlDoc);

    xmlStr.ClearBuffer();
    xmlDoc.Print(&xmlStr);
    xmlJsonStr = xml2json(xmlStr.CStr());
    jsonTmp = json::parse(xmlJsonStr.c_str());

    auto scriptbind = jsonTmp["doxygen"]["compounddef"];

    std::regex nonChar("(\040)$|(\056)(\040)$");

    auto scriptBindName = std::string{};

    auto enginePrefixPos = scriptbind["compoundname"].get<std::string>().find(g_EnginePrefix);
    auto gamePrefixPos = scriptbind["compoundname"].get<std::string>().find(g_GamePrefix);

    if (enginePrefixPos != std::string::npos) {
      auto namePos = scriptbind["compoundname"].get<std::string>().find("_");
      scriptBindName = scriptbind["compoundname"].get<std::string>().replace(enginePrefixPos, namePos + 1, "");
    }
    else if (gamePrefixPos != std::string::npos) {
      auto namePos = scriptbind["compoundname"].get<std::string>().find("_");
      scriptBindName = scriptbind["compoundname"].get<std::string>().replace(gamePrefixPos, namePos + 1, "");
    }

    json methods{};

    if (scriptbind["sectiondef"].is_array()) {
      methods = scriptbind["sectiondef"].at(1)["memberdef"];
    }
    else {
      methods = scriptbind["sectiondef"]["memberdef"];
    }

    if (scriptbind["briefdescription"].find("para") != scriptbind["briefdescription"].end()) {
      jsonFinal["scriptbinds"][scriptBindName]["description"] =
        regex_replace(scriptbind["briefdescription"]["para"].get<std::string>(), nonChar, "");
    }
    else {
      printf("No description on script bind %s\n", scriptBindName.c_str());
    }

    for (auto i = 1; i < methods.size(); i++) {
      json method = {
        {"description", ""},
        {"params", json::object()},
        {"ret", {
          {"type", ""},
          {"description", ""}
        }}
      };
      auto methodName = methods.at(i)["name"].get<std::string>();

      if (methods.at(i)["briefdescription"].find("para") != methods.at(i)["briefdescription"].end()) {
        method["description"] = 
          regex_replace(methods.at(i)["briefdescription"]["para"].get<std::string>(), nonChar, "");
      }
      else {
        printf("No description on function %s for script bind %s\n", methodName.c_str(), scriptBindName.c_str());
      }

      method["ret"]["type"] = "void";

      if (methods.at(i)["detaileddescription"].find("para") != methods.at(i)["detaileddescription"].end()) {
        auto paramList = methods.at(i)["detaileddescription"]["para"]["parameterlist"];

        if (paramList.is_array()) {
          for (auto& paramItem : paramList) {
            auto paramitem = paramItem["parameteritem"];
            if (paramItem["@kind"].get<std::string>().compare("param") == 0) {
              for (auto param = 1; param < methods.at(i)["param"].size(); param++) {
                auto paramName = methods.at(i)["param"].at(param)["declname"];
              }
            }
          }
        }
        else {
          
        }
      }

      jsonFinal["scriptbinds"][scriptBindName]["methods"][methodName] = method;
    }
  }

  printf("%s\n", jsonFinal.dump(2).c_str());

  return 0;
}
