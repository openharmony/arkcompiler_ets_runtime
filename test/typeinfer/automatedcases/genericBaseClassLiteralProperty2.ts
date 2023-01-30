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

// === tests/cases/compiler/genericBaseClassLiteralProperty2.ts ===
declare function AssertType(value:any, type:string):void;
class CollectionItem2 { }

class BaseCollection2<TItem extends CollectionItem2> {
    _itemsByKey: { [key: string]: TItem; };
    constructor() {
        this._itemsByKey = {};
AssertType(this._itemsByKey = {}, "{}");
AssertType(this._itemsByKey, "{ [string]: TItem; }");
AssertType(this, "this");
AssertType({}, "{}");
    }
}

class DataView2 extends BaseCollection2<CollectionItem2> {
    fillItems(item: CollectionItem2) {
        this._itemsByKey['dummy'] = item;
AssertType(this._itemsByKey['dummy'] = item, "CollectionItem2");
AssertType(this._itemsByKey['dummy'], "CollectionItem2");
AssertType(this._itemsByKey, "{ [string]: CollectionItem2; }");
AssertType(this, "this");
AssertType('dummy', "string");
AssertType(item, "CollectionItem2");
    }
}


