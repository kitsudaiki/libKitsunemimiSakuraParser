/**
 * @file        validator.cpp
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

#include "validator.h"

#include <items/item_methods.h>

#include <libKitsunemimiSakuraLang/sakura_lang_interface.h>
#include <libKitsunemimiSakuraLang/blossom.h>

namespace Kitsunemimi
{
namespace Sakura
{

/**
 * @brief check if all values are set inside a blossom-item
 *
 * @param blossomItem blossom-item with the information
 *
 * @return true, if all necessary values are set, else false
 */
bool
checkBlossomItem(SakuraLangInterface* interface,
                 BlossomItem &blossomItem,
                 std::string &errorMessage)
{
    bool result = false;

    TreeItem* item = interface->m_garden->getRessource(blossomItem.blossomType);
    if(item != nullptr) {
        return true;
    }

    Blossom* blossom = interface->getBlossom(blossomItem.blossomGroupType,
                                             blossomItem.blossomType);
    if(blossom == nullptr)
    {
        errorMessage = createError(blossomItem, "validator", "unknow blossom-type");
        return false;
    }

    result = blossom->validateInput(blossomItem, errorMessage);

    return result;
}

/**
 * @brief central method of the thread to check the current part of the execution-tree
 *
 * @param sakuraItem subtree, which should be checked
 *
 * @return true if successful, else false
 */
bool
checkSakuraItem(SakuraLangInterface* interface,
                SakuraItem* sakuraItem,
                const std::string &filePath,
                std::string &errorMessage)
{
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::SEQUENTIELL_ITEM)
    {
        SequentiellPart* sequential = dynamic_cast<SequentiellPart*>(sakuraItem);
        for(SakuraItem* item : sequential->childs)
        {
            if(checkSakuraItem(interface, item, filePath, errorMessage) == false) {
                return false;
            }
        }
        return true;
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::TREE_ITEM)
    {
        TreeItem* treeItem = dynamic_cast<TreeItem*>(sakuraItem);
        const std::string completePath = treeItem->rootPath + "/" + treeItem->relativePath;
        return checkSakuraItem(interface, treeItem->childs, completePath, errorMessage);
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::SUBTREE_ITEM)
    {
        return true;
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::BLOSSOM_ITEM)
    {
        BlossomItem* blossomItem = dynamic_cast<BlossomItem*>(sakuraItem);
        blossomItem->blossomPath = filePath;
        return checkBlossomItem(interface, *blossomItem, errorMessage);
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::BLOSSOM_GROUP_ITEM)
    {
        BlossomGroupItem* blossomGroupItem = dynamic_cast<BlossomGroupItem*>(sakuraItem);
        for(BlossomItem* blossomItem : blossomGroupItem->blossoms)
        {
            blossomItem->blossomGroupType = blossomGroupItem->blossomGroupType;
            blossomItem->blossomName = blossomGroupItem->id;

            overrideItems(blossomItem->values,
                          blossomGroupItem->values,
                          ONLY_NON_EXISTING);

            if(checkSakuraItem(interface, blossomItem, filePath, errorMessage) == false) {
                return false;
            }
        }

        return true;
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::IF_ITEM)
    {
        IfBranching* ifBranching = dynamic_cast<IfBranching*>(sakuraItem);
        if(checkSakuraItem(interface, ifBranching->ifContent, filePath, errorMessage) == false) {
            return false;
        }

        if(checkSakuraItem(interface, ifBranching->elseContent, filePath, errorMessage) == false) {
            return false;
        }

        return true;
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::FOR_EACH_ITEM)
    {
        ForEachBranching* forEachBranching = dynamic_cast<ForEachBranching*>(sakuraItem);
        return checkSakuraItem(interface, forEachBranching->content, filePath, errorMessage);
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::FOR_ITEM)
    {
        ForBranching* forBranching = dynamic_cast<ForBranching*>(sakuraItem);
        return checkSakuraItem(interface, forBranching->content, filePath, errorMessage);
    }
    //----------------------------------------------------------------------------------------------
    if(sakuraItem->getType() == SakuraItem::PARALLEL_ITEM)
    {
        ParallelPart* parallel = dynamic_cast<ParallelPart*>(sakuraItem);
        return checkSakuraItem(interface, parallel->childs, filePath, errorMessage);
    }
    //----------------------------------------------------------------------------------------------
    // TODO: error-message

    return false;
}

/**
 * @brief checkAllItems
 * @param garden
 * @return
 */
bool
checkAllItems(SakuraLangInterface *interface,
              const SakuraGarden &garden,
              std::string &errorMessage)
{
    std::map<std::string, TreeItem*>::const_iterator mapIt;
    for(mapIt = garden.trees.begin();
        mapIt != garden.trees.end();
        mapIt++)
    {
        if(checkSakuraItem(interface,mapIt->second, "", errorMessage) == false) {
            return false;
        }
    }

    return true;
}

}
}