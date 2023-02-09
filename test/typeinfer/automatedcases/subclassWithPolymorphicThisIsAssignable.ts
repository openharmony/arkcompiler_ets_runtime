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

// === tests/cases/compiler/subclassWithPolymorphicThisIsAssignable.ts ===
declare function AssertType(value:any, type:string):void;
/* taken from mongoose.Document */
interface Document {
    increment(): this;
}

/* our custom model extends the mongoose document */
interface CustomDocument extends Document { }

export class Example<Z extends CustomDocument> {
    constructor() {
        // types of increment not compatible??
        this.test<Z>();
AssertType(this.test<Z>(), "void");
AssertType(this.test, "<Z extends Document>() => void");
AssertType(this, "this");
    }

    public test<Z extends Document>() { }
}


