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

// === tests/cases/compiler/privacyTypeParametersOfClassDeclFile.ts ===
declare function AssertType(value:any, type:string):void;
class privateClass {
}

export class publicClass {
}

export class publicClassWithPrivateTypeParameters<T extends privateClass> { // Error
    myMethod(val: T): T {
AssertType(val, "T");
        return val;
    }
}

export class publicClassWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T {
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T {
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T { 
AssertType(val, "T");
        return val;
    }
}

export class publicClassWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T { 
AssertType(val, "T");
        return val;
    }
}

class privateClassWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T { 
AssertType(val, "T");
        return val;
    }
}

export class publicClassWithTypeParametersFromPrivateModule<T extends privateModule.publicClassInPrivateModule> { // Error
    myMethod(val: T): T {
AssertType(val, "T");
        return val;
    }
}

class privateClassWithTypeParametersFromPrivateModule<T extends privateModule.publicClassInPrivateModule> {
    myMethod(val: T): T {
AssertType(val, "T");
        return val;
    }
}

export module publicModule {
    class privateClassInPublicModule {
    }

    export class publicClassInPublicModule {
    }

    export class publicClassWithPrivateTypeParameters<T extends privateClassInPublicModule> { // Error
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    export class publicClassWithPublicTypeParameters<T extends publicClassInPublicModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPrivateTypeParameters<T extends privateClassInPublicModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPublicTypeParameters<T extends publicClassInPublicModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    export class publicClassWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    export class publicClassWithTypeParametersFromPrivateModule<T extends privateModule.publicClassInPrivateModule> { // Error
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithTypeParametersFromPrivateModule<T extends privateModule.publicClassInPrivateModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }
}

module privateModule {
    class privateClassInPrivateModule {
    }

    export class publicClassInPrivateModule {
    }

    export class publicClassWithPrivateTypeParameters<T extends privateClassInPrivateModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    export class publicClassWithPublicTypeParameters<T extends publicClassInPrivateModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPrivateTypeParameters<T extends privateClassInPrivateModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPublicTypeParameters<T extends publicClassInPrivateModule> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    export class publicClassWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }

    class privateClassWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T {
AssertType(val, "T");
            return val;
        }
    }
}


