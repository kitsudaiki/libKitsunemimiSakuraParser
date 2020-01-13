/**
 * @file        sakura_parser.y
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

%skeleton "lalr1.cc"

%defines

//requires 3.2 to avoid the creation of the stack.hh
%require "3.0.4"
%define parser_class_name {SakuraParser}

%define api.prefix {sakura}
%define api.namespace {Kitsunemimi::Sakura}
%define api.token.constructor
%define api.value.type variant
%locations

%define parse.assert

%code requires
{
#include <string>
#include <iostream>
#include <vector>
#include <libKitsunemimiCommon/common_items/data_items.h>

using Kitsunemimi::Common::DataItem;
using Kitsunemimi::Common::DataArray;
using Kitsunemimi::Common::DataValue;
using Kitsunemimi::Common::DataMap;

namespace Kitsunemimi
{
namespace Sakura
{

class SakuraParserInterface;

}  // namespace Sakura
}  // namespace Kitsunemimi
}

// The parsing context.
%param { Kitsunemimi::Sakura::SakuraParserInterface& driver }

%locations

%code
{
#include <sakura_parsing/sakura_parser_interface.h>
# undef YY_DECL
# define YY_DECL \
    Kitsunemimi::Sakura::SakuraParser::symbol_type sakuralex (Kitsunemimi::Sakura::SakuraParserInterface& driver)
YY_DECL;
}

// Token
%define api.token.prefix {Sakura_}
%token
    END  0  "end of file"
    BOOL_TRUE  "true"
    BOOL_FALSE "false"
    SEED  "seed"
    TREE  "tree"
    SUBTREE  "sutbree"
    PARALLEL_FOR "parallel_for"
    PARALLEL  "parallel"
    IF  "if"
    ELSE  "else"
    FOR  "for"
    ASSIGN  ":"
    SEMICOLON  ";"
    DOT  "."
    COMMA  ","
    DELIMITER  "|"
    ARROW   "->"
    MINUS   "-"
    PLUS    "+"
    EQUAL   "="
    LBRACK  "["
    RBRACK  "]"
    LBRACKBOW  "{"
    RBRACKBOW  "}"
    LROUNDBRACK  "("
    RROUNDBRACK  ")"
    EQUAL_COMPARE "=="
    UNEQUAL_COMPARE "!="
    GREATER_EQUAL_COMPARE ">="
    SMALLER_EQUAL_COMPARE "<="
    GREATER_COMPARE ">"
    SMALLER_COMPARE "<"
    SHIFT_LEFT "<<"
    SHIFT_RIGHT ">>"
;

%token <std::string> IDENTIFIER "identifier"
%token <std::string> STRING "string"
%token <std::string> STRING_PLN "string_pln"
%token <long> NUMBER "number"
%token <double> FLOAT "float"

%type  <std::string> name_item
%type  <std::string> compare_type
%type  <DataMap*>  value_item
%type  <DataArray*>  value_item_list
%type  <std::string> regiterable_identifier;

%type  <DataMap*> blossom_group
%type  <DataArray*> blossom_group_set
%type  <DataMap*> blossom
%type  <DataArray*> blossom_set

%type  <DataMap*> item
%type  <DataArray*> item_set
%type  <DataArray*> string_array

%type  <DataMap*> access
%type  <DataArray*> access_list
%type  <DataMap*> function
%type  <DataArray*> function_list

%type  <DataMap*> if_condition
%type  <DataMap*> for_loop

%type  <DataMap*> parallel
%type  <DataMap*> parallel_for_loop

%type  <DataMap*> subtree
%type  <DataMap*> tree_fork

%type  <DataItem*> json_abstract
%type  <DataValue*> json_value
%type  <DataArray*> json_array
%type  <DataArray*> json_array_content
%type  <DataMap*> json_object
%type  <DataMap*> json_object_content

%%
%start startpoint;

startpoint:
    subtree
    {
        driver.setOutput($1);
    }

subtree:
   "[" name_item "]" item_set blossom_group_set
   {
       $$ = new DataMap();
       $$->insert("b_id", new DataValue($2));
       $$->insert("b_type", new DataValue("subtree"));
       $$->insert("items", $4);
       $$->insert("parts", $5);
   }

if_condition:
   "if" "(" value_item compare_type value_item ")" "{" blossom_group_set "}" "else" "{" blossom_group_set "}"
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("if"));
       $$->insert("if_type", new DataValue($4));
       $$->insert("left", $3);
       $$->insert("right", $5);

       $$->insert("if_parts", $8);
       $$->insert("else_parts", $12);
   }
|
    "if" "(" value_item compare_type value_item ")" "{" blossom_group_set "}"
    {
        $$ = new DataMap();
        $$->insert("b_type", new DataValue("if"));
        $$->insert("if_type", new DataValue($4));
        $$->insert("left", $3);
        $$->insert("right", $5);

        $$->insert("if_parts", $8);
        $$->insert("else_parts", new DataArray());
    }

for_loop:
    "for" "(" regiterable_identifier ":" value_item ")" item_set "{" blossom_group_set "}"
    {
        $$ = new DataMap();
        $$->insert("b_type", new DataValue("for_each"));
        $$->insert("variable", new DataValue($3));
        $$->insert("list", $5);
        $$->insert("items", $7);
        $$->insert("parts", $9);
    }
|
    "for" "(" regiterable_identifier "=" value_item ";" "identifier" "<" value_item ";" "identifier" "+" "+" ")" item_set "{" blossom_group_set "}"
    {
        if($7 != $3)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $7 + "\"",
                         true);
            return 1;
        }
        if($11 != $3)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $11 + "\"",
                         true);
            return 1;
        }

        $$ = new DataMap();
        $$->insert("b_type", new DataValue("for"));
        $$->insert("variable1", new DataValue($3));
        $$->insert("variable2", new DataValue($7));
        $$->insert("variable3", new DataValue($11));
        $$->insert("start", $5);
        $$->insert("end", $9);
        $$->insert("items", $15);
        $$->insert("parts", $17);
    }

parallel:
    "parallel" "(" ")" "{" blossom_group_set "}"
    {
        $$ = new DataMap();
        $$->insert("b_type", new DataValue("parallel"));
        $$->insert("parts", $5);
    }

parallel_for_loop:
    "parallel_for" "(" regiterable_identifier ":" value_item ")" item_set "{" blossom_group_set "}"
    {
        $$ = new DataMap();
        $$->insert("b_type", new DataValue("parallel_for_each"));
        $$->insert("variable", new DataValue($3));
        $$->insert("list", $5);
        $$->insert("items", $7);
        $$->insert("parts", $9);
    }
|
    "parallel_for" "(" regiterable_identifier "=" value_item ";" "identifier" "<" value_item ";" "identifier" "+" "+" ")" item_set "{" blossom_group_set "}"
    {
        if($7 != $3)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $7 + "\"",
                         true);
            return 1;
        }
        if($11 != $3)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $11 + "\"",
                         true);
            return 1;
        }

        $$ = new DataMap();
        $$->insert("b_type", new DataValue("parallel_for"));
        $$->insert("variable1", new DataValue($3));
        $$->insert("variable2", new DataValue($7));
        $$->insert("variable3", new DataValue($11));
        $$->insert("start", $5);
        $$->insert("end", $9);
        $$->insert("items", $15);
        $$->insert("parts", $17);
    }

blossom_group_set:
    blossom_group_set if_condition
    {
        $1->append($2);
        $$ = $1;
    }
|
    blossom_group_set for_loop
    {
        $1->append($2);
        $$ = $1;
    }
|
    blossom_group_set parallel
    {
        $1->append($2);
        $$ = $1;
    }
|
    blossom_group_set parallel_for_loop
    {
        $1->append($2);
        $$ = $1;
    }
|
    blossom_group_set blossom_group
    {
        $1->append($2);
        $$ = $1;
    }
|
    if_condition
    {
        $$ = new DataArray();
        $$->append($1);
    }
|
    for_loop
    {
        $$ = new DataArray();
        $$->append($1);
    }
|
    parallel
    {
        $$ = new DataArray();
        $$->append($1);
    }
|
    parallel_for_loop
    {
        $$ = new DataArray();
        $$->append($1);
    }
|
    blossom_group
    {
        $$ = new DataArray();
        $$->append($1);
    }

blossom_group:
   "identifier" "(" name_item ")" item_set blossom_set
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("blossom_group"));
       $$->insert("name", new DataValue($3));
       $$->insert("blossom-group-type", new DataValue($1));
       $$->insert("items-input", $5);
       $$->insert("blossoms", $6);
   }
|
   "identifier" "(" name_item ")" blossom_set
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("blossom_group"));
       $$->insert("name", new DataValue($3));
       $$->insert("blossom-group-type", new DataValue($1));
       $$->insert("items-input", new DataArray());
       $$->insert("blossoms", $5);
   }
|
   "identifier" "(" name_item ")" item_set
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("blossom_group"));
       $$->insert("name", new DataValue($3));
       $$->insert("blossom-group-type", new DataValue($1));
       $$->insert("items-input", $5);
       $$->insert("blossoms", new DataArray());
   }
|
  "identifier" "(" name_item ")"
  {
      $$ = new DataMap();
      $$->insert("b_type", new DataValue("blossom_group"));
      $$->insert("name", new DataValue($3));
      $$->insert("blossom-group-type", new DataValue($1));
      $$->insert("items-input", new DataArray());
      $$->insert("blossoms", new DataArray());
  }

blossom_set:
   blossom_set blossom
   {
       $1->append($2);
       $$ = $1;
   }
|
   blossom
   {
       $$ = new DataArray();
       $$->append($1);
   }

blossom:
   "->" "identifier"
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("blossom"));
       $$->insert("blossom-type", new DataValue($2));
       $$->insert("output", new DataValue());
       $$->insert("items-input", new DataArray());
   }
|
   "->" "identifier" ":" item_set
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("blossom"));
       $$->insert("blossom-type", new DataValue($2));
       $$->insert("output", new DataValue());
       $$->insert("items-input", $4);
   }

item_set:
   %empty
   {
       $$ = new DataArray();
   }
|
   item_set  item
   {
       $1->append($2);
       $$ = $1;
   }
|
   item
   {
       $$ = new DataArray();
       $$->append($1);
   }

regiterable_identifier:
   "identifier"
   {
       driver.m_registeredKeys.push_back($1);
       $$ = $1;
   }

item:
   "-" regiterable_identifier "=" "{" "{" "}" "}"
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("assign"));
       $$->insert("key", new DataValue($2));
       std::string empty = "{{}}";
       $$->insert("value", new DataValue(empty));
   }
|
   "-" regiterable_identifier "=" value_item
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("assign"));
       $$->insert("key", new DataValue($2));
       $$->insert("value", $4);
   }
|
   "-" regiterable_identifier "=" json_abstract
   {
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("assign"));
       $$->insert("key", new DataValue($2));
       $$->insert("value", $4);
   }
|
   "-" value_item ">>" "identifier"
   {
       if(driver.isKeyRegistered($4) == false)
       {
           driver.error(yyla.location,
                        "undefined identifier \"" + $4 + "\"",
                        true);
           return 1;
       }
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("output"));
       $$->insert("key", new DataValue($4));
       $$->insert("value", $2);
   }
|
   "-" "identifier" compare_type value_item
   {
       if(driver.isKeyRegistered($2) == false)
       {
           driver.error(yyla.location,
                        "undefined identifier \"" + $2 + "\"",
                        true);
           return 1;
       }
       $$ = new DataMap();
       $$->insert("b_type", new DataValue("compare"));
       $$->insert("key", new DataValue($2));
       $$->insert("compare_type", new DataValue($3));
       $$->insert("value", $4);
   }

string_array:
   string_array "," name_item
   {
       $1->append(new DataValue($3));
       $$ = $1;
   }
|
   name_item
   {
       $$ = new DataArray();
       $$->append(new DataValue($1));
   }
|
   %empty
   {
       $$ = new DataArray();
   }

tree_fork:
   "subtree" "(" "identifier" ")" item_set
   {
       DataMap* tempItem = new DataMap();
       tempItem->insert("b_type", new DataValue("branch"));
       tempItem->insert("b_id", new DataValue($3));
       tempItem->insert("items-input", $5);
       $$ = tempItem;
   }
|
   "seed" "(" "identifier" ")" item_set "{"  "}"
   {
       DataMap* tempItem = new DataMap();
       tempItem->insert("b_type", new DataValue("seed"));
       tempItem->insert("b_id", new DataValue($3));
       tempItem->insert("connection", $5);
       $$ = tempItem;
   }

value_item_list:
   value_item_list ","  value_item
   {
       $1->append($3);
       $$ = $1;
   }
|
   value_item
   {
       $$ = new DataArray();
       $$->append($1);
   }

value_item:
    "float"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue($1));
        tempItem->insert("b_type", new DataValue("value"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "number"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue($1));
        tempItem->insert("b_type", new DataValue("value"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "true"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue(true));
        tempItem->insert("b_type", new DataValue("value"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "false"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue(false));
        tempItem->insert("b_type", new DataValue("value"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "string"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue(driver.removeQuotes($1)));
        tempItem->insert("b_type", new DataValue("value"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "identifier"
    {
        if(driver.isKeyRegistered($1) == false)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $1 + "\"",
                         true);
            return 1;
        }
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue($1));
        tempItem->insert("b_type", new DataValue("identifier"));
        tempItem->insert("functions", new DataArray());
        $$ = tempItem;
    }
|
    "identifier" function_list
    {
        if(driver.isKeyRegistered($1) == false)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $1 + "\"",
                         true);
            return 1;
        }
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue($1));
        tempItem->insert("b_type", new DataValue("identifier"));
        tempItem->insert("functions", $2);
        $$ = tempItem;
    }
|
    "identifier" access_list
    {
        if(driver.isKeyRegistered($1) == false)
        {
            driver.error(yyla.location,
                         "undefined identifier \"" + $1 + "\"",
                         true);
            return 1;
        }
        DataMap* tempItem = new DataMap();
        tempItem->insert("item", new DataValue($1));
        tempItem->insert("b_type", new DataValue("identifier"));
        tempItem->insert("functions", $2);
        $$ = tempItem;
    }

function_list:
    function_list function
    {
        $1->append($2);
        $$ = $1;
    }
|
    function
    {
        $$ = new DataArray();
        $$->append($1);
    }

function:
    "." "identifier" "(" value_item_list ")"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("b_type", new DataValue($2));
        tempItem->insert("args", $4);
        $$ = tempItem;
    }
|
    "." "identifier" "(" ")"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("b_type", new DataValue($2));
        tempItem->insert("args", new DataArray());
        $$ = tempItem;
    }

access_list:
    access_list access
    {
        $1->append($2);
        $$ = $1;
    }
|
    access
    {
        $$ = new DataArray();
        $$->append($1);
    }

access:
    "[" "identifier" "]"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("b_type", new DataValue("get"));

        DataArray* args = new DataArray();
        args->append(new DataValue($2));
        tempItem->insert("args", args);
        $$ = tempItem;
    }
|
    "[" "number" "]"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("b_type", new DataValue("get"));

        DataArray* args = new DataArray();
        args->append(new DataValue($2));
        tempItem->insert("args", args);
        $$ = tempItem;
    }
|
    "[" "string" "]"
    {
        DataMap* tempItem = new DataMap();
        tempItem->insert("b_type", new DataValue("get"));

        DataArray* args = new DataArray();
        args->append(new DataValue(driver.removeQuotes($2)));
        tempItem->insert("args", args);
        $$ = tempItem;
    }

name_item:
   "identifier"
   {
       $$ = $1;
   }
|
   "string"
   {
       $$ = driver.removeQuotes($1);
   }

compare_type:
   "=="
   {
       $$ = "==";
   }
|
   ">="
   {
       $$ = ">=";
   }
|
   "<="
   {
       $$ = "<=";
   }
|
   ">"
   {
       $$ = ">";
   }
|
   "<"
   {
       $$ = "<";
   }
|
   "!="
   {
       $$ = "!=";
   }


json_abstract:
   json_object
   {
       $$ = (DataItem*)$1;
   }
|
   json_array
   {
       $$ = (DataItem*)$1;
   }

json_object:
   "{" json_object_content "}"
   {
       $$ = $2;
   }
|
   "{" "}"
   {
       $$ = new DataMap();
   }

json_object_content:
   json_object_content "," "identifier" ":" json_abstract
   {
       $1->insert($3, $5);
       $$ = $1;
   }
|
   "identifier" ":" json_abstract
   {
       $$ = new DataMap();
       $$->insert($1, $3);
   }
|
   json_object_content "," "string_pln" ":" json_abstract
   {
       $1->insert(driver.removeQuotes($3), $5);
       $$ = $1;
   }
|
   "string_pln" ":" json_abstract
   {
       $$ = new DataMap();
       $$->insert(driver.removeQuotes($1), $3);
   }
|
   json_object_content "," "string" ":" json_abstract
   {
       $1->insert(driver.removeQuotes($3), $5);
       $$ = $1;
   }
|
   "string" ":" json_abstract
   {
       $$ = new DataMap();
       $$->insert(driver.removeQuotes($1), $3);
   }

json_array:
   "[" json_array_content "]"
   {
       $$ = $2;
   }
|
   "[" "]"
   {
       $$ = new DataArray();
   }

json_array_content:
   json_array_content "," json_abstract
   {
       $1->append($3);
       $$ = $1;
   }
|
   json_abstract
   {
       $$ = new DataArray();
       $$->append($1);
   }

json_value:
   "number"
   {
       $$ = new DataValue($1);
   }
|
   "float"
   {
       $$ = new DataValue($1);
   }
|
   "string"
   {
       $$ = new DataValue(driver.removeQuotes($1));
   }
|
   "true"
   {
       $$ = new DataValue(true);
   }
|
   "false"
   {
       $$ = new DataValue(false);
   }

%%

void Kitsunemimi::Sakura::SakuraParser::error(const Kitsunemimi::Sakura::location& location,
                                              const std::string& message)
{
    driver.error(location, message, false);
}
