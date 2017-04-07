#include "tinyxml2/tinyxml2.h"
#include "json/json.hpp"
#include "xml2json/xml2json.hpp"

#include <regex>

using json = nlohmann::json;

const json g_ParamValues = {
  {"const char *", "string"},
  {"const char*", "string"},
  {"bool", "boolean"},
  {"int", "number"},
  {"unsigned int", "number"},
  {"float", "number"},
  {"ScriptHandle", "pointer"},
  {"glm::vec2", "table"},
  {"const glm::vec2", "table"},
  {"glm::vec3", "table"},
  {"const glm::vec3", "table"},
  {"ScriptTable", "table"},
  {"SmartScriptTable", "table"},
  {"hexe::gameplay::tile::TileCoord", "table"},
  {"gameplay::tile::TileCoord", "table"},
  {"tile::TileCoord", "table"},
  {"TileCoord", "table"},
  {"hexe::component,,Entity", "number"},
  {"component::Entity", "number"},
  {"Entity", "number"},
  {"KeyCode", "number"},
  {"input::KeyCode", "number"},
  {"hexe::input::KeyCode", "number"},
  {"uint32_t", "number"}
};
const std::string g_EnginePrefix = "hexe::service::scripts::scriptbinds::ScriptBind_";
const std::string g_GamePrefix = "hexegame::scriptbinds::ScriptBind_";

void XmlErrorCheck(const tinyxml2::XMLError a_LoadResult, tinyxml2::XMLDocument* a_XmlDoc) {
  if (a_LoadResult != tinyxml2::XML_SUCCESS) {
    printf("Couldn't load index XML file. Error %s\n", a_XmlDoc->ErrorName());
    a_XmlDoc->ClearError();
  }
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    printf("No arguments, atom_hexe requires -i for input directory and -o for output directory\n");
    return -1;
  }

  auto inputDir = std::string{};
  auto outputDir = std::string{};

  for (auto i = 0; i < argc; i++) {
    if (strcmp("-o", argv[i]) == 0) {
      outputDir = argv[i + 1];
    }
    else if (strcmp("-i", argv[i]) == 0) {
      inputDir = argv[i + 1];
    }
    else if (strcmp("-h", argv[i]) == 0) {
      printf("atom_hexe help\n");
      return 0;
    }
  }

  tinyxml2::XMLDocument xmlDoc{};
  tinyxml2::XMLError xmlLoadResult = xmlDoc.LoadFile((inputDir + "\\index.xml").c_str());

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

  for (auto& fileName : fileNames) {
    xmlDoc.Clear();
    xmlStr.ClearBuffer();

    xmlLoadResult = xmlDoc.LoadFile((inputDir + "\\" + fileName).c_str());

    XmlErrorCheck(xmlLoadResult, &xmlDoc);

    xmlDoc.Print(&xmlStr);
    xmlJsonStr = xml2json(xmlStr.CStr());
    jsonTmp = json::parse(xmlJsonStr.c_str());

    auto scriptbind = jsonTmp["doxygen"]["compounddef"];

    std::regex nonChar("(\040)$");

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
        std::regex_replace(scriptbind["briefdescription"]["para"].get<std::string>(), nonChar, "");
    }
    else {
      printf("No description on script bind %s\n", scriptBindName.c_str());
    }

    if (!methods.is_array()) continue;

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
          std::regex_replace(methods.at(i)["briefdescription"]["para"].get<std::string>(), nonChar, "");
      }
      else {
        printf("No description on function %s for script bind %s\n", methodName.c_str(), scriptBindName.c_str());
      }

      method["ret"]["type"] = "void";

      if (methods.at(i)["detaileddescription"].find("para") != methods.at(i)["detaileddescription"].end()) {
        auto paramList = methods.at(i)["detaileddescription"]["para"]["parameterlist"];

        if (paramList.is_array()) {
          for (auto& j : paramList) {
            auto paramitem = j["parameteritem"];

            if (j["@kind"].get<std::string>().compare("param") == 0) {
              if (methods.at(i)["param"].is_array()) {
                for (auto param = 1; param < methods.at(i)["param"].size(); param++) {
                  auto paramName = methods.at(i)["param"].at(param)["declname"].get<std::string>();
                  auto paramType = g_ParamValues[methods.at(i)["param"].at(param)["type"].get<std::string>()].get<std::string>();
                  auto paramDesc = std::string{};

                  if (paramitem.is_array()) {
                    if (paramitem.at(param - 1)["parameterdescription"]["para"].is_object()) {
                      auto paramText = paramitem.at(param - 1)["parameterdescription"]["para"]["#text"].at(0).get<std::string>();
                      auto results = std::smatch{};
                      auto lastChar = std::string{};

                      while (std::regex_search(paramText, results, nonChar)) {
                        lastChar = results[0];
                        paramText = std::regex_replace(paramText, nonChar, "");
                      }
                      paramText += lastChar;
                      paramDesc += paramText;

                      paramDesc +=
                        paramitem.at(param - 1)["parameterdescription"]["para"]["ulink"]["@url"].get<std::string>();

                      paramText = paramitem.at(param - 1)["parameterdescription"]["para"]["#text"].at(1).get<std::string>();
                      paramDesc += paramText;
                    }
                    else {
                      paramDesc =
                        std::regex_replace(paramitem.at(param - 1)["parameterdescription"]["para"].get<std::string>(), nonChar, "");
                    }
                  }
                  else {
                    paramDesc =
                      std::regex_replace(paramitem["parameterdescription"]["para"].get<std::string>(), nonChar, "");
                  }

                  method["params"][paramName]["type"] = paramType;
                  method["params"][paramName]["description"] = paramDesc;
                }
              }
            }
            else if(j["@kind"].get<std::string>().compare("retval") == 0) {
              method["ret"]["type"] = 
                g_ParamValues[paramitem["parameternamelist"]["parametername"].get<std::string>()].get<std::string>();
              method["ret"]["description"] =
                std::regex_replace(paramitem["parameterdescription"]["para"].get<std::string>(), nonChar, "");
            }
          }
        }
        else {
          auto paramitem = paramList["parameteritem"];
          if (methods.at(i)["param"].is_array()) {
            for (auto param = 1; param < methods.at(i)["param"].size(); param++) {
              auto type = methods.at(i)["param"];
              auto paramName = methods.at(i)["param"].at(param)["declname"].get<std::string>();
              auto paramType = g_ParamValues[methods.at(i)["param"].at(param)["type"].get<std::string>()].get<std::string>();
              auto paramDesc = std::string{};

              if (paramitem.is_array()) {
                paramDesc =
                  std::regex_replace(paramitem.at(param - 1)["parameterdescription"]["para"].get<std::string>(), nonChar, "");
              }
              else {
                paramDesc =
                  std::regex_replace(paramitem["parameterdescription"]["para"].get<std::string>(), nonChar, "");
              }

              method["params"][paramName]["type"] = paramType;
              method["params"][paramName]["description"] = paramDesc;
            }
          }
        }
      }

      jsonFinal["scriptbinds"][scriptBindName]["methods"][methodName] = method;
    }
  }

  std::ofstream file{};
  file.open((outputDir + "\\scriptbinds.json").c_str());
  file << jsonFinal.dump(2);
  file.close();

  return 0;
}
