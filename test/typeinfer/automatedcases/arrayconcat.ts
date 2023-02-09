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

// === tests/cases/compiler/arrayconcat.ts ===
declare function AssertType(value:any, type:string):void;
interface IOptions {
    name?: string;
    flag?: boolean;
    short?: string;
    usage?: string;
    set?: (s: string) => void;
    type?: string;
    experimental?: boolean;
}

class parser {
	public options: IOptions[];

	public m() {
		this.options = this.options.sort(function(a, b) {
AssertType(this.options, "IOptions[]");
AssertType(this, "this");
AssertType(this.options.sort(function(a, b) {            let aName = a.name.toLowerCase();            let bName = b.name.toLowerCase();            if (aName > bName) {                return 1;            } else if (aName < bName) {                return -1;            } else {                return 0;            }        }), "IOptions[]");
AssertType(this.options.sort, "(?(IOptions, IOptions) => number) => IOptions[]");
AssertType(this.options, "IOptions[]");
AssertType(this, "this");
AssertType(function(a, b) {            let aName = a.name.toLowerCase();            let bName = b.name.toLowerCase();            if (aName > bName) {                return 1;            } else if (aName < bName) {                return -1;            } else {                return 0;            }        }, "(IOptions, IOptions) => union");
AssertType(a, "IOptions");
AssertType(b, "IOptions");
AssertType(this.options = this.options.sort(function(a, b) {            let aName = a.name.toLowerCase();            let bName = b.name.toLowerCase();            if (aName > bName) {                return 1;            } else if (aName < bName) {                return -1;            } else {                return 0;            }        }), "IOptions[]");

            let aName = a.name.toLowerCase();
AssertType(aName, "string");
AssertType(a.name.toLowerCase(), "string");
AssertType(a.name.toLowerCase, "() => string");
AssertType(a.name, "string");

            let bName = b.name.toLowerCase();
AssertType(bName, "string");
AssertType(b.name.toLowerCase(), "string");
AssertType(b.name.toLowerCase, "() => string");
AssertType(b.name, "string");

            if (aName > bName) {
AssertType(aName > bName, "boolean");
AssertType(aName, "string");
AssertType(bName, "string");

AssertType(1, "int");
                return 1;

            } else if (aName < bName) {
AssertType(aName < bName, "boolean");
AssertType(aName, "string");
AssertType(bName, "string");

AssertType(-1, "int");
AssertType(1, "int");
                return -1;

            } else {
AssertType(0, "int");
                return 0;
            }
        });
	}
}

