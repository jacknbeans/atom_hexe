#include "tinyxml2/tinyxml2.h"
#include "json/json.hpp"
#include "xml2json/xml2json.hpp"

#include <regex>
#include <vector>

using json = nlohmann::json;

// A json object used to translate C++ values to Lua values
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
  {"scripts::ScriptTable", "table"},
  {"service::scripts::ScriptTable", "table"},
  {"hexe::service::scripts::ScriptTable", "table"},
  {"SmartScriptTable", "table"},
  {"scripts::SmartScriptTable", "table"},
  {"service::scripts::SmartScriptTable", "table"},
  {"hexe::service::scripts::SmartScriptTable", "table"},
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
// Prefixs that doxygen uses in the xml output that I want to remove
const std::string g_EnginePrefix = "hexe::service::scripts::scriptbinds::ScriptBind_";
const std::string g_GamePrefix = "hexegame::scriptbinds::ScriptBind_";

// Just some error checking when loading an xml file
void XmlErrorCheck(const tinyxml2::XMLError a_LoadResult, tinyxml2::XMLDocument* a_XmlDoc, const char* a_XmlFileName) {
  if (a_LoadResult != tinyxml2::XML_SUCCESS) {
    printf("Couldn't load %s. Error %s\n", a_XmlFileName, a_XmlDoc->ErrorName());
    a_XmlDoc->ClearError();
  }
}

int main(int argc, char* argv[]) {
  // If no arguments are given to the command take an early exit
  if (argc == 1) {
    printf("No arguments, atom_hexe requires -i for input directory and -o for output directory\n");
    return -1;
  }

  auto inputDir = std::string{};
  auto outputDir = std::string{};

  // Checking each argument passed to the command
  for (auto i = 0; i < argc; i++) {
    // -o is for telling the command where json file should be placed
    if (strcmp("-o", argv[i]) == 0) {
      outputDir = argv[i + 1];
    }
    // -i is for telling the command where the xml output from doxygen resides
    else if (strcmp("-i", argv[i]) == 0) {
      inputDir = argv[i + 1];
    }
    // -h is for showing help information on the command
    else if (strcmp("-h", argv[i]) == 0) {
      printf("\nHelp for atom_hexe:\n\n"
             "    -i \"input_dir\"    Point to where doxygen has produced the XML documentation\n"
             "    -o \"output_dir\"   Point to where the JSON file should be output\n");
      return 0;
    }
  }

  tinyxml2::XMLDocument xmlDoc{};
  // Loading the xml file into memory
  tinyxml2::XMLError xmlLoadResult = xmlDoc.LoadFile((inputDir + "\\index.xml").c_str());

  XmlErrorCheck(xmlLoadResult, &xmlDoc, "index.xml");

  // Converting xml file into a json string
  tinyxml2::XMLPrinter xmlStr;
  xmlDoc.Print(&xmlStr);
  std::string xmlJsonStr = xml2json(xmlStr.CStr());

  std::vector<std::string> fileNames{};

  // Parsing the json string from the xml input into a json object
  json jsonFinal{};
  json jsonTmp = json::parse(xmlJsonStr.c_str());

  // Creating our own json object that contains only what we need in a clear and concise manner
  json scriptbinds = jsonTmp["doxygenindex"]["compound"];

  // Iterate over every single script bind that doxygen generated and filter out all 
  // the useless information that we do not require
  for (auto& scriptbind : scriptbinds) {
    // Finding both prefixes because the script bind could be either from the engine or from the game
    auto enginePrefixPos = scriptbind["name"].get<std::string>().find(g_EnginePrefix);
    auto gamePrefixPos = scriptbind["name"].get<std::string>().find(g_GamePrefix);

    if (enginePrefixPos != std::string::npos ||
        gamePrefixPos != std::string::npos) {
      // Storing the filename for later use
      fileNames.push_back(scriptbind["@refid"].get<std::string>() + ".xml");

      std::string name{};
      // If there was an engine prefix then remove that prefix from the name of the script bind
      if (enginePrefixPos != std::string::npos) {
        auto namePos = scriptbind["name"].get<std::string>().find("_");
        name = scriptbind["name"].get<std::string>().replace(enginePrefixPos, namePos + 1, "");
      }
      // Same thing as the engine prefix
      else if (gamePrefixPos != std::string::npos) {
        auto namePos = scriptbind["name"].get<std::string>().find("_");
        name = scriptbind["name"].get<std::string>().replace(gamePrefixPos, namePos + 1, "");
      }

      // Filling our final json object with the name of the script bind and a template for the description
      // of the script bind and what methods it has
      jsonFinal["scriptbinds"][name] = {
        {"description", ""},
        {"methods", json::object()}
      };
    }
  }

  // Iterate over every single file that is a script bind and get the description of the script bind itself
  // and all of the information about the methods
  for (auto& fileName : fileNames) {
    xmlDoc.Clear();
    xmlStr.ClearBuffer();

    xmlLoadResult = xmlDoc.LoadFile((inputDir + "\\" + fileName).c_str());

    XmlErrorCheck(xmlLoadResult, &xmlDoc, fileName.c_str());

    xmlDoc.Print(&xmlStr);
    xmlJsonStr = xml2json(xmlStr.CStr());
    jsonTmp = json::parse(xmlJsonStr.c_str());

    auto scriptbind = jsonTmp["doxygen"]["compounddef"];

    // This is to remove any whitespace at the end of any string
    std::regex nonChar("(\040)$");

    // Store the script bind name for later use when I need to add the methods and description to it
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

    // Depending on whether or not the sectiondef is an array, either get the first element or just
    // directly access the member
    if (scriptbind["sectiondef"].is_array()) {
      methods = scriptbind["sectiondef"].at(1)["memberdef"];
    }
    else {
      methods = scriptbind["sectiondef"]["memberdef"];
    }

    // Find the description of the script bind and put it in our final json object
    if (scriptbind["briefdescription"].find("para") != scriptbind["briefdescription"].end()) {
      jsonFinal["scriptbinds"][scriptBindName]["description"] =
        std::regex_replace(scriptbind["briefdescription"]["para"].get<std::string>(), nonChar, "");
    }
    else {
      printf("No description on script bind %s\n", scriptBindName.c_str());
    }

    if (!methods.is_array()) continue;

    // Iterate over every one of the methods found for the script bind and grab it's information
    for (auto i = 1; i < methods.size(); i++) {
      // Template for the method json object
      json method = {
        {"description", ""},
        {"params", json::array()},
        {"ret", {
          {"type", ""},
          {"description", ""}
        }}
      };
      auto methodName = methods.at(i)["name"].get<std::string>();

      // Find the description of the method
      if (methods.at(i)["briefdescription"].find("para") != methods.at(i)["briefdescription"].end()) {
        method["description"] = 
          std::regex_replace(methods.at(i)["briefdescription"]["para"].get<std::string>(), nonChar, "");
      }
      else {
        printf("No description on function %s for script bind %s\n", methodName.c_str(), scriptBindName.c_str());
      }

      method["ret"]["type"] = "void";

      // There are a lot of checks here because the xml output varies so much depending on whether or not
      // there was a custom return value, how many params there are, etc.

      //This first check is to find out whether or not there are any parameters and/or a custom return statement
      if (methods.at(i)["detaileddescription"].find("para") != methods.at(i)["detaileddescription"].end()) {
        auto paramList = methods.at(i)["detaileddescription"]["para"]["parameterlist"];

        // If the parameter list is an array, means there is a parameter(s) and a custom return statement
        if (paramList.is_array()) {
          for (auto& j : paramList) {
            auto paramitem = j["parameteritem"];

            // Check whether the item in the parameter list is a parameter
            if (j["@kind"].get<std::string>().compare("param") == 0) {
              if (methods.at(i)["param"].is_array()) {
                // Loop through each parameter in the array (this will always skip the first 
                // one since it's always the function handler and that isn't used in the scripts)
                for (auto param = 1; param < methods.at(i)["param"].size(); param++) {
                  auto paramName = methods.at(i)["param"].at(param)["declname"].get<std::string>();
                  auto paramType = g_ParamValues[methods.at(i)["param"].at(param)["type"].get<std::string>()].get<std::string>();
                  auto paramDesc = std::string{};

                  // If the parameter item is an array, means that there could be multiple lines in the description of the parameter
                  if (paramitem.is_array()) {
                    // If the parameter item is an object, that means it has multiple lines
                    if (paramitem.at(param - 1)["parameterdescription"]["para"].is_object()) {
                      auto paramText = paramitem.at(param - 1)["parameterdescription"]["para"]["#text"].at(0).get<std::string>();
                      auto results = std::smatch{};
                      auto lastChar = std::string{};

                      // Finding every single whitespace character in the string and replacing it with an empty one
                      while (std::regex_search(paramText, results, nonChar)) {
                        lastChar = results[0];
                        paramText = std::regex_replace(paramText, nonChar, "");
                      }

                      // Reconstructing the string to be only one line instead of multiple
                      paramText += lastChar;
                      paramDesc += paramText;

                      // TODO(jack): Needs further testing to see if multilined comments without a url would still work
                      paramDesc +=
                        paramitem.at(param - 1)["parameterdescription"]["para"]["ulink"]["@url"].get<std::string>();

                      paramText = paramitem.at(param - 1)["parameterdescription"]["para"]["#text"].at(1).get<std::string>();
                      paramDesc += paramText;
                    }
                    // If the parameter item is not an object, than it doesn't have multiple lines and can just be grabbed in it's entirety
                    else {
                      paramDesc =
                        std::regex_replace(paramitem.at(param - 1)["parameterdescription"]["para"].get<std::string>(), nonChar, "");
                    }
                  }
                  // If the parameter item is also not an array, it can be grabbed in it's entirety
                  else {
                    paramDesc =
                      std::regex_replace(paramitem["parameterdescription"]["para"].get<std::string>(), nonChar, "");
                  }

                  // When putting the methods in the method template I'm using an array so that I can ensure
                  // that the order will stay the same since with a json object the order doesn't usually matter
                  // but in this case it does
                  method["params"].push_back(json::object({ {paramName, json::object({ {"type", paramType} })} }));
                  method["params"].at(param - 1)[paramName]["description"] = paramDesc;
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
        // If the parameter list is not an array, means that there is either just parameters or a custom return statement
        else {
          auto paramitem = paramList["parameteritem"];
          // If the parameter isn't an array, means that we don't have any parameters and instead we might have a custom return statement
          if (methods.at(i)["param"].is_array()) {
            // Same process as above for grabbing the methods description, etc.
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

              method["params"].push_back(json::object({ {paramName, json::object({ {"type", paramType} })} }));
              method["params"].at(param - 1)[paramName]["description"] = paramDesc;
            }
          }
          // Grab the custom return statement and fill it's information in the json method
          else {
            if (paramList["@kind"].get<std::string>().compare("retval") == 0) {
              method["ret"]["type"] = 
                g_ParamValues[paramitem["parameternamelist"]["parametername"].get<std::string>()].get<std::string>();
              method["ret"]["description"] =
                std::regex_replace(paramitem["parameterdescription"]["para"].get<std::string>(), nonChar, "");
            }
          }
        }
      }

      // We made it! The method can be placed in the script bind under it's methods member
      jsonFinal["scriptbinds"][scriptBindName]["methods"][methodName] = method;
    }
  }

  // Serialize the data into a json file by just dumping the whole object into a file
  std::ofstream file{};
  file.open((outputDir + "\\scriptbinds.json").c_str());
  file << std::setw(2) << jsonFinal;
  file.close();

  return 0;
}
