/**
 * @file       sakura_lang_interface.cpp
 *
 * @author     Tobias Anker <tobias.anker@kitsunemimi.moe>
 *
 * @copyright  Apache License Version 2.0
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

#include <libKitsunemimiSakuraLang/sakura_lang_interface.h>

#include <libKitsunemimiSakuraLang/sakura_garden.h>
#include <validator.h>

#include <processing/subtree_queue.h>
#include <processing/thread_pool.h>

#include <items/item_methods.h>

#include <libKitsunemimiJinja2/jinja2_converter.h>
#include <libKitsunemimiPersistence/logger/logger.h>

namespace Kitsunemimi
{
namespace Sakura
{

/**
 * @brief constructor
 *
 * @param enableDebug set to true to enable the debug-output of the parser
 */
SakuraLangInterface::SakuraLangInterface(const bool enableDebug)
{
    garden = new SakuraGarden(enableDebug);
    m_queue = new SubtreeQueue();
    jinja2Converter = new Kitsunemimi::Jinja2::Jinja2Converter();
    // TODO: make number of threads configurable
    m_threadPoos = new ThreadPool(6, this);
}

/**
 * @brief destructor
 */
SakuraLangInterface::~SakuraLangInterface()
{
    delete garden;
    delete m_queue;
    delete m_threadPoos;
    delete jinja2Converter;
}

/**
 * @brief SakuraLangInterface::processFiles
 *
 * @param inputPath path to the initial sakura-file or directory with the root.sakura file
 * @param initialValues map-item with initial values to override the items of the initial tree-item
 * @param dryRun set to true to only parse and check the files, without executing them
 * @param errorMessage reference for error-message
 *
 * @return true, if successfule, else false
 */
bool
SakuraLangInterface::processFiles(const std::string &inputPath,
                                  const DataMap &initialValues,
                                  const bool dryRun,
                                  std::string &errorMessage)
{
    // precheck input
    if(bfs::is_regular_file(inputPath) == false
            && bfs::is_directory(inputPath) == false)
    {
        errorMessage = "Not a regular file or directory as input-path " + inputPath;
        return false;
    }

    // set default-file in case that a directory instead of a file was selected
    std::string treeFile = inputPath;
    if(bfs::is_directory(treeFile)) {
        treeFile = treeFile + "/root.sakura";
    }

    // parse all files
    if(garden->addTree(treeFile, errorMessage) == false)
    {
        errorMessage = "failed to add trees\n    " + errorMessage;
        return false;
    }

    // get relative path from the input-path
    const bfs::path parent = bfs::path(treeFile).parent_path();
    const std::string relPath = bfs::relative(treeFile, parent).string();

    // get initial tree-item
    SakuraItem* tree = garden->getTree(relPath, parent.string());
    if(tree == nullptr)
    {
        errorMessage = "No tree found for the input-path " + treeFile;
        return false;
    }

    // check if input-values match with the first tree
    const std::vector<std::string> failedInput = checkInput(tree->values, initialValues);
    if(failedInput.size() > 0)
    {
        errorMessage = "Following input-values are not valid for the initial tress:\n";

        for(const std::string& item : failedInput) {
            errorMessage += "    " + item + "\n";
        }

        return false;
    }

    // validate parsed blossoms
    if(checkAllItems(this, errorMessage) == false) {
        return false;
    }

    // in case of a dry-run, cancel here before executing the scripts
    if(dryRun) {
        return true;
    }

    // process sakura-file with initial values
    if(runProcess(tree, initialValues, errorMessage) == false) {
        return false;
    }

    return true;
}

/**
 * @brief SakuraLangInterface::doesBlossomExist
 *
 * @param groupName
 * @param itemName
 *
 * @return
 */
bool
SakuraLangInterface::doesBlossomExist(const std::string &groupName,
                                      const std::string &itemName)
{
    std::map<std::string, std::map<std::string, Blossom*>>::const_iterator groupIt;
    groupIt = m_registeredBlossoms.find(groupName);

    if(groupIt != m_registeredBlossoms.end())
    {
        std::map<std::string, Blossom*>::const_iterator itemIt;
        itemIt = groupIt->second.find(itemName);

        if(itemIt != groupIt->second.end()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief SakuraLangInterface::addBlossom
 *
 * @param groupName
 * @param itemName
 * @param newBlossom
 *
 * @return
 */
bool
SakuraLangInterface::addBlossom(const std::string &groupName,
                                const std::string &itemName,
                                Blossom* newBlossom)
{
    if(doesBlossomExist(groupName, itemName) == true) {
        return false;
    }

    std::map<std::string, std::map<std::string, Blossom*>>::iterator groupIt;
    groupIt = m_registeredBlossoms.find(groupName);

    if(groupIt == m_registeredBlossoms.end())
    {
        std::map<std::string, Blossom*> newMap;
        m_registeredBlossoms.insert(std::make_pair(groupName, newMap));
    }

    groupIt = m_registeredBlossoms.find(groupName);
    groupIt->second.insert(std::make_pair(itemName, newBlossom));

    return true;
}

/**
 * @brief SakuraLangInterface::getBlossom
 *
 * @param groupName
 * @param itemName
 *
 * @return
 */
Blossom*
SakuraLangInterface::getBlossom(const std::string &groupName,
                                const std::string &itemName)
{
    std::map<std::string, std::map<std::string, Blossom*>>::const_iterator groupIt;
    groupIt = m_registeredBlossoms.find(groupName);

    if(groupIt != m_registeredBlossoms.end())
    {
        std::map<std::string, Blossom*>::const_iterator itemIt;
        itemIt = groupIt->second.find(itemName);

        if(itemIt != groupIt->second.end()) {
            return itemIt->second;
        }
    }

    return nullptr;
}

/**
 * @brief SakuraLangInterface::runProcess
 *
 * @param item
 * @param initialValues
 * @param errorMessage reference for error-message
 *
 * @return
 */
bool
SakuraLangInterface::runProcess(SakuraItem* item,
                                const DataMap &initialValues,
                                std::string &errorMessage)
{
    std::vector<SakuraItem*> childs;
    childs.push_back(item);
    std::vector<std::string> hierarchy;

    const bool result = m_queue->spawnParallelSubtrees(childs,
                                                       "",
                                                       hierarchy,
                                                       initialValues,
                                                       errorMessage);
    return result;
}

/**
 * @brief convert blossom-group-item into an output-message
 *
 * @param blossomItem blossom-group-tem to generate the output
 */
void
SakuraLangInterface::printOutput(const BlossomGroupItem &blossomGroupItem)
{
    std::string output = "";

    // print call-hierarchy
    for(uint32_t i = 0; i < blossomGroupItem.nameHirarchie.size(); i++)
    {
        for(uint32_t j = 0; j < i; j++) {
            output += "   ";
        }
        output += blossomGroupItem.nameHirarchie.at(i) + "\n";
    }

    printOutput(output);
}

/**
 * @brief convert blossom-item into an output-message
 *
 * @param blossomItem blossom-item to generate the output
 */
void
SakuraLangInterface::printOutput(const BlossomItem &blossomItem)
{
    printOutput(convertBlossomOutput(blossomItem));
}

/**
 * @brief print output-string
 *
 * @param output string, which should be printed
 */
void
SakuraLangInterface::printOutput(const std::string &output)
{
    m_mutex.lock();

    // get width of the termial to draw the separator-line
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    uint32_t terminalWidth = size.ws_col;

    // limit the length of the line to avoid problems in the gitlab-ci-runner
    if(terminalWidth > 300) {
        terminalWidth = 300;
    }

    // draw separator line
    std::string line(terminalWidth, '=');

    LOG_INFO(line + "\n\n" + output + "\n");

    m_mutex.unlock();
}

} // namespace Sakura
} // namespace Kitsunemimi
