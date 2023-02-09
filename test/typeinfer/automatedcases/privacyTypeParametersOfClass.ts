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

// === tests/cases/compiler/privacyTypeParametersOfClass.ts ===
declare function AssertType(value:any, type:string):void;
class privateClass {
}

export class publicClass {
}

// TypeParameter_0_of_exported_class_1_has_or_is_using_private_type_2
export class publicClassWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T { // Error
AssertType(val, "T");
        return val;
    }
}

export class publicClassWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T { // No Error
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T { // No Error
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T { // No Error
AssertType(val, "T");
        return val;
    }
}

export class publicClassWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T { // No Error
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T { // No Error
AssertType(val, "T");
        return val;
    }
}


