/**
 * @file        sakura_garden.h
 *
 * @author      Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright   Apache License Version 2.0
 *
 *      Copyright 2019 Tobias Anker
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#ifndef KITSUNEMIMI_SAKURA_LANG_GARDEN_H
#define KITSUNEMIMI_SAKURA_LANG_GARDEN_H

#include <vector>
#include <string>
#include <map>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace Kitsunemimi
{
class DataBuffer;

namespace Sakura
{
class TreeItem;
class SakuraParsing;

class SakuraGarden
{
public:
    SakuraGarden(const bool enableDebug);
    ~SakuraGarden();

    const bfs::path getRelativePath(const bfs::path &blossomFilePath,
                                    const bfs::path &blossomInternalRelPath);

    // add
    bool addTree(const bfs::path &treePath,
                 std::string &errorMessage);
    bool addResource(const std::string &content,
                     const bfs::path &treePath,
                     std::string &errorMessage);

    // get
    TreeItem* getTree(const std::string &relativePath,
                      const std::string &rootPath = "");
    TreeItem* getRessource(const std::string &id);
    const std::string getTemplate(const std::string &relativePath);
    DataBuffer* getFile(const std::string &relativePath);


    // object-handling
    std::string rootPath = "";
    std::map<std::string, TreeItem*> trees;
    std::map<std::string, TreeItem*> resources;
    std::map<std::string, std::string> templates;
    std::map<std::string, Kitsunemimi::DataBuffer*> files;

private:
    SakuraParsing* m_parser = nullptr;

    TreeItem* getTreeByPath(const bfs::path &relativePath);
};

} // namespace Sakura
} // namespace Kitsunemimi

#endif // KITSUNEMIMI_SAKURA_LANG_GARDEN_H
