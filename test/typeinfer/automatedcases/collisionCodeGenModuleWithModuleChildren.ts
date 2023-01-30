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

// === tests/cases/compiler/collisionCodeGenModuleWithModuleChildren.ts ===
declare function AssertType(value:any, type:string):void;

module M {
    export let x = 3;
    module m1 {
        let M = 10;
        let p = x;
    }
}

module M {
    module m2 {
        class M {
        }
        let p = x;
        let p2 = new M();
    }
}

module M {
    module m3 {
        function M() {
        }
        let p = x;
        let p2 = M();
    }
}

module M { // shouldnt be _M
    module m3 {
        interface M {
        }
        let p = x;
        let p2: M;
    }
}

module M {
    module m4 {
        module M {
            let p = x;
        }
    }
}

