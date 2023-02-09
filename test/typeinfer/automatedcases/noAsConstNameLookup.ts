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

// === tests/cases/compiler/noAsConstNameLookup.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #44292

type Store = { a: 123 }
export type Cleaner = <W extends Store>(runner: FeatureRunner<W>) => Promise<any>

export class FeatureRunner<W extends Store> {
    private readonly cleaners: Cleaner[] = []

    async runFeature(): Promise<any> {
        const objectWhichShouldBeConst = {
AssertType(objectWhichShouldBeConst, "{ readonly flags: {}; readonly settings: {}; }");
AssertType({            flags: {},            settings: {},        } as const, "{ readonly flags: {}; readonly settings: {}; }");
AssertType({            flags: {},            settings: {},        }, "{ readonly flags: {}; readonly settings: {}; }");

            flags: {},
AssertType(flags, "{}");
AssertType({}, "{}");

            settings: {},
AssertType(settings, "{}");
AssertType({}, "{}");

        } as const;
AssertType(objectWhichShouldBeConst, "{ readonly flags: {}; readonly settings: {}; }");
        return objectWhichShouldBeConst
    }

    async run(): Promise<any> {
        const result = {
AssertType(result, "{}");

AssertType({}, "{}");
}

        this.cleaners.forEach(c => c(this))
AssertType(this.cleaners.forEach(c => c(this)), "void");
AssertType(this.cleaners.forEach, "((Cleaner, number, Cleaner[]) => void, ?any) => void");
AssertType(this.cleaners, "Cleaner[]");
AssertType(this, "this");
AssertType(c => c(this), "(Cleaner) => Promise<any>");
AssertType(c, "Cleaner");
AssertType(c(this), "Promise<any>");
AssertType(c, "Cleaner");
AssertType(this, "this");

AssertType(result, "{}");
        return result
    }
}

export class C<T> {
    f(): void {
        let one = 1 as const;
AssertType(one, "int");
AssertType(1 as const, "int");
AssertType(1, "int");
    }
}
new C<string>().f();
AssertType(new C<string>().f(), "void");
AssertType(new C<string>().f, "() => void");


