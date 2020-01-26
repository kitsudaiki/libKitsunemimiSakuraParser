/**
 * @file        sakura_parsing.cpp
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

#include <libKitsunemimiSakuraParser/sakura_parsing.h>

#include <sakura_parsing/sakura_parser_interface.h>

#include <libKitsunemimiCommon/common_methods/string_methods.h>
#include <libKitsunemimiCommon/common_items/data_items.h>

#include <libKitsunemimiJson/json_item.h>
#include <libKitsunemimiPersistence/files/text_file.h>

using Kitsunemimi::Persistence::readFile;

namespace Kitsunemimi
{
namespace Sakura
{

/**
 * @brief constructor
 */
SakuraParsing::SakuraParsing(const bool debug)
{
    m_debug = debug;
    m_parser = new SakuraParserInterface(m_debug);
}

/**
 * @brief destructor
 */
SakuraParsing::~SakuraParsing()
{
    delete m_parser;
}

/**
 * @brief parse all tree-files at a specific location
 *
 * @param rootPath path to file or directory with the file(s) to parse
 *
 * @return true, if pasing all files was successful, else false
 */
bool
SakuraParsing::parseFiles(const std::string &rootPath)
{
    JsonItem result;
    m_fileContents.clear();

    // init error-message
    m_errorMessage.clearTable();
    m_errorMessage.addColumn("key");
    m_errorMessage.addColumn("value");
    m_errorMessage.addRow(std::vector<std::string>{"ERROR", " "});
    m_errorMessage.addRow(std::vector<std::string>{"component", "libKitsunemimiSakuraParser"});

    // parse
    if(parseAllFiles(rootPath) == false) {
        return false;
    }

    return true;
}

/**
 * @brief request the error-message, in case that parseFiles had failed
 *
 * @return error-message as table-item
 */
TableItem
SakuraParsing::getError() const
{
    return m_errorMessage;
}

/**
 * @brief search and parse all files in a specific location
 *
 * @param rootPath path to file or directory with the file(s) to parse
 *
 * @return true, if all was successful, else false
 */
bool
SakuraParsing::parseAllFiles(const std::string &rootPath)
{
    boost::filesystem::path rootPathObj(rootPath);    

    // precheck
    if(exists(rootPathObj) == false)
    {
        m_errorMessage.addRow(std::vector<std::string>{"source", "while reading sakura-files"});
        m_errorMessage.addRow(std::vector<std::string>{"message",
                                                       "path doesn't exist: " + rootPath});
        return false;
    }

    // get all files
    if(is_directory(rootPathObj))
    {
        getFilesInDir(rootPathObj);
        // check result
        if(m_fileContents.size() == 0)
        {
            m_errorMessage.addRow(std::vector<std::string>{"source", "while reading sakura-files"});
            m_errorMessage.addRow(std::vector<std::string>{"message",
                                                           "no files found in the directory: "
                                                           + rootPath});
            return false;
        }
    }
    else
    {
        // store file-path with a placeholder in a list
        m_fileContents.push_back(std::make_pair(rootPath, JsonItem()));
    }

    // get and parse file-contents
    for(uint32_t i = 0; i < m_fileContents.size(); i++)
    {
        const std::string filePath = m_fileContents.at(i).first;

        // read file
        std::string errorMessage = "";
        std::pair<bool, std::string> result = readFile(filePath, errorMessage);
        if(result.first == false)
        {
            m_errorMessage.addRow(std::vector<std::string>{"source", "while reading sakura-files"});
            m_errorMessage.addRow(std::vector<std::string>{"message",
                                                           "failed to read file-path: "
                                                           + filePath
                                                           + " with error: "
                                                           + errorMessage});
            return false;
        }

        // parse file-content
        const bool parserResult = m_parser->parse(result.second);
        if(parserResult == false)
        {
            m_errorMessage = m_parser->getErrorMessage();
            return false;
        }

        // get the parsed result from the parser and get path of the file,
        // where the skript actually is and add it to the parsed content.
        m_fileContents[i].second = m_parser->getOutput()->copy()->toMap();
        m_fileContents[i].second.insert("b_path",
                                        new DataValue(m_fileContents.at(i).first),
                                        true);

        // debug-output to print the parsed file-content as json-string
        if(m_debug) {
            std::cout<<m_fileContents[i].second.toString(true)<<std::endl;
        }
    }

    return true;
}

/**
 * @brief request the parsed content of a specific subtree
 *
 * @param name Name of the requested file-content. If string is empty, the content of the first
 *             file in the list will be returned.
 *
 * @return Subtree-content as json-item. This is an invalid item, when the requested name
 *         doesn't exist in the parsed file list.
 */
const JsonItem
SakuraParsing::getParsedFileContent(const std::string &name)
{
    // precheck
    if(name == ""
            && m_fileContents.size() > 0)
    {
        return m_fileContents.at(0).second;
    }

    // search
    std::vector<std::pair<std::string, JsonItem>>::iterator it;
    for(it = m_fileContents.begin();
        it != m_fileContents.end();
        it++)
    {
        if(it->second.get("b_id").toString() == name) {
            return it->second;
        }
    }

    return JsonItem();
}

/**
 * @brief get all file-paths in a directory and its subdirectory
 *
 * @param directory parent-directory for searching
 */
void
SakuraParsing::getFilesInDir(const boost::filesystem::path &directory)
{
    directory_iterator end_itr;
    for(directory_iterator itr(directory);
        itr != end_itr;
        ++itr)
    {
        if(is_directory(itr->path()))
        {
            // process subdirectories, but no directories named tempales or files, because
            // they shouldn't contain any tree-files
            if(itr->path().leaf().string() != "templates"
                    && itr->path().leaf().string() != "files")
            {
                getFilesInDir(itr->path());
            }
        }
        else
        {
            if(m_debug) {
                std::cout<<"found file: "<<itr->path().string()<<std::endl;
            }
            m_fileContents.push_back(std::make_pair(itr->path().string(), JsonItem()));
        }
    }
}

}  // namespace Sakura
}  // namespace Kitsunemimi
