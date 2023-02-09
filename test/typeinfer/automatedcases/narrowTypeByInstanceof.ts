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

// === tests/cases/compiler/narrowTypeByInstanceof.ts ===
declare function AssertType(value:any, type:string):void;
    class Match {
        public range(): any {
AssertType(undefined, "undefined");
            return undefined;
        }
    }

    class FileMatch {
        public resource(): any {
AssertType(undefined, "undefined");
            return undefined;
        }
    }

type FileMatchOrMatch = FileMatch | Match;


let elementA: FileMatchOrMatch, elementB: FileMatchOrMatch;
AssertType(elementA, "FileMatchOrMatch");
AssertType(elementB, "FileMatchOrMatch");

if (elementA instanceof FileMatch && elementB instanceof FileMatch) {
    let a = elementA.resource().path;
AssertType(a, "any");
AssertType(elementA.resource().path, "any");
AssertType(elementA.resource(), "any");
AssertType(elementA.resource, "() => any");

    let b = elementB.resource().path;
AssertType(b, "any");
AssertType(elementB.resource().path, "any");
AssertType(elementB.resource(), "any");
AssertType(elementB.resource, "() => any");

} else if (elementA instanceof Match && elementB instanceof Match) {
    let a = elementA.range();
AssertType(a, "any");
AssertType(elementA.range(), "any");
AssertType(elementA.range, "() => any");

    let b = elementB.range();
AssertType(b, "any");
AssertType(elementB.range(), "any");
AssertType(elementB.range, "() => any");
}


