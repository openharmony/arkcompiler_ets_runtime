/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/discriminatedUnionJsxElement.tsx ===
declare function AssertType(value:any, type:string):void;
// Repro from #46021

interface IData<MenuItemVariant extends ListItemVariant = ListItemVariant.OneLine> {
    menuItemsVariant?: MenuItemVariant;
}

function Menu<MenuItemVariant extends ListItemVariant = ListItemVariant.OneLine>(data: IData<MenuItemVariant>) {
    const listItemVariant = data.menuItemsVariant ?? ListItemVariant.OneLine;
AssertType(listItemVariant, "union");
AssertType(data.menuItemsVariant ?? ListItemVariant.OneLine, "union");
AssertType(data.menuItemsVariant, "union");
AssertType(ListItemVariant.OneLine, "ListItemVariant.OneLine");

AssertType(<ListItem variant={listItemVariant} />, "error");
AssertType(ListItem, "(IListItemData) => null");
AssertType(variant, "ListItemVariant");
AssertType(listItemVariant, "ListItemVariant");
    return <ListItem variant={listItemVariant} />;
}

type IListItemData = { variant: ListItemVariant.Avatar; } | { variant: ListItemVariant.OneLine; };

enum ListItemVariant {
    OneLine,
    Avatar,
}

function ListItem(_data: IListItemData) {
AssertType(null, "null");
    return null; 
}


