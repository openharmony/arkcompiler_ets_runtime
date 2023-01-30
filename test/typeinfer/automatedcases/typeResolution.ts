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

// === tests/cases/compiler/typeResolution.ts ===
declare function AssertType(value:any, type:string):void;
export module TopLevelModule1 {
    export module SubModule1 {
        export module SubSubModule1 {
            export class ClassA {
                public AisIn1_1_1() {
                    // Try all qualified names of this type
                    let a1: ClassA; a1.AisIn1_1_1();
AssertType(a1, "ClassA");
AssertType(a1.AisIn1_1_1(), "void");
AssertType(a1.AisIn1_1_1, "() => void");

                    let a2: SubSubModule1.ClassA; a2.AisIn1_1_1();
AssertType(a2, "ClassA");
AssertType(SubSubModule1, "any");
AssertType(a2.AisIn1_1_1(), "void");
AssertType(a2.AisIn1_1_1, "() => void");

                    let a3: SubModule1.SubSubModule1.ClassA; a3.AisIn1_1_1();
AssertType(a3, "ClassA");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a3.AisIn1_1_1(), "void");
AssertType(a3.AisIn1_1_1, "() => void");

                    let a4: TopLevelModule1.SubModule1.SubSubModule1.ClassA; a4.AisIn1_1_1();
AssertType(a4, "ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a4.AisIn1_1_1(), "void");
AssertType(a4.AisIn1_1_1, "() => void");
                    
                    // Two letiants of qualifying a peer type
                    let b1: ClassB; b1.BisIn1_1_1();
AssertType(b1, "ClassB");
AssertType(b1.BisIn1_1_1(), "void");
AssertType(b1.BisIn1_1_1, "() => void");

                    let b2: TopLevelModule1.SubModule1.SubSubModule1.ClassB; b2.BisIn1_1_1();
AssertType(b2, "ClassB");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(b2.BisIn1_1_1(), "void");
AssertType(b2.BisIn1_1_1, "() => void");
                    
                    // Type only accessible from the root
                    let c1: TopLevelModule1.SubModule2.SubSubModule2.ClassA; c1.AisIn1_2_2();
AssertType(c1, "SubModule2.SubSubModule2.ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule2, "any");
AssertType(SubSubModule2, "any");
AssertType(c1.AisIn1_2_2(), "void");
AssertType(c1.AisIn1_2_2, "() => void");
                    
                    // Interface reference
                    let d1: InterfaceX; d1.XisIn1_1_1();
AssertType(d1, "InterfaceX");
AssertType(d1.XisIn1_1_1(), "any");
AssertType(d1.XisIn1_1_1, "() => any");

                    let d2: SubSubModule1.InterfaceX; d2.XisIn1_1_1();
AssertType(d2, "InterfaceX");
AssertType(SubSubModule1, "any");
AssertType(d2.XisIn1_1_1(), "any");
AssertType(d2.XisIn1_1_1, "() => any");
                }
            }
            export class ClassB {
                public BisIn1_1_1() {
                    /** Exactly the same as above in AisIn1_1_1 **/
                    
                    // Try all qualified names of this type
                    let a1: ClassA; a1.AisIn1_1_1();
AssertType(a1, "ClassA");
AssertType(a1.AisIn1_1_1(), "void");
AssertType(a1.AisIn1_1_1, "() => void");

                    let a2: SubSubModule1.ClassA; a2.AisIn1_1_1();
AssertType(a2, "ClassA");
AssertType(SubSubModule1, "any");
AssertType(a2.AisIn1_1_1(), "void");
AssertType(a2.AisIn1_1_1, "() => void");

                    let a3: SubModule1.SubSubModule1.ClassA; a3.AisIn1_1_1();
AssertType(a3, "ClassA");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a3.AisIn1_1_1(), "void");
AssertType(a3.AisIn1_1_1, "() => void");

                    let a4: TopLevelModule1.SubModule1.SubSubModule1.ClassA; a4.AisIn1_1_1();
AssertType(a4, "ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a4.AisIn1_1_1(), "void");
AssertType(a4.AisIn1_1_1, "() => void");
                    
                    // Two letiants of qualifying a peer type
                    let b1: ClassB; b1.BisIn1_1_1();
AssertType(b1, "ClassB");
AssertType(b1.BisIn1_1_1(), "void");
AssertType(b1.BisIn1_1_1, "() => void");

                    let b2: TopLevelModule1.SubModule1.SubSubModule1.ClassB; b2.BisIn1_1_1();
AssertType(b2, "ClassB");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(b2.BisIn1_1_1(), "void");
AssertType(b2.BisIn1_1_1, "() => void");
                    
                    // Type only accessible from the root
                    let c1: TopLevelModule1.SubModule2.SubSubModule2.ClassA; c1.AisIn1_2_2();
AssertType(c1, "SubModule2.SubSubModule2.ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule2, "any");
AssertType(SubSubModule2, "any");
AssertType(c1.AisIn1_2_2(), "void");
AssertType(c1.AisIn1_2_2, "() => void");

                    let c2: TopLevelModule2.SubModule3.ClassA; c2.AisIn2_3();
AssertType(c2, "TopLevelModule2.SubModule3.ClassA");
AssertType(TopLevelModule2, "any");
AssertType(SubModule3, "any");
AssertType(c2.AisIn2_3(), "void");
AssertType(c2.AisIn2_3, "() => void");
                    
                    // Interface reference
                    let d1: InterfaceX; d1.XisIn1_1_1();
AssertType(d1, "InterfaceX");
AssertType(d1.XisIn1_1_1(), "any");
AssertType(d1.XisIn1_1_1, "() => any");

                    let d2: SubSubModule1.InterfaceX; d2.XisIn1_1_1();
AssertType(d2, "InterfaceX");
AssertType(SubSubModule1, "any");
AssertType(d2.XisIn1_1_1(), "any");
AssertType(d2.XisIn1_1_1, "() => any");
                }
            }
            export interface InterfaceX { XisIn1_1_1(); }
            class NonExportedClassQ {
                constructor() {
                    function QQ() {
AssertType(QQ, "() => void");

                        /* Sampling of stuff from AisIn1_1_1 */
                        let a4: TopLevelModule1.SubModule1.SubSubModule1.ClassA; a4.AisIn1_1_1();
AssertType(a4, "ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a4.AisIn1_1_1(), "void");
AssertType(a4.AisIn1_1_1, "() => void");

                        let c1: TopLevelModule1.SubModule2.SubSubModule2.ClassA; c1.AisIn1_2_2();
AssertType(c1, "SubModule2.SubSubModule2.ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule2, "any");
AssertType(SubSubModule2, "any");
AssertType(c1.AisIn1_2_2(), "void");
AssertType(c1.AisIn1_2_2, "() => void");

                        let d1: InterfaceX; d1.XisIn1_1_1();
AssertType(d1, "InterfaceX");
AssertType(d1.XisIn1_1_1(), "any");
AssertType(d1.XisIn1_1_1, "() => any");

                        let c2: TopLevelModule2.SubModule3.ClassA; c2.AisIn2_3();
AssertType(c2, "TopLevelModule2.SubModule3.ClassA");
AssertType(TopLevelModule2, "any");
AssertType(SubModule3, "any");
AssertType(c2.AisIn2_3(), "void");
AssertType(c2.AisIn2_3, "() => void");
                    }
                }
            }
        }
        
        // Should have no effect on S1.SS1.ClassA above because it is not exported
        class ClassA {
            constructor() {
                function AA() {
AssertType(AA, "() => void");

                    let a2: SubSubModule1.ClassA; a2.AisIn1_1_1();
AssertType(a2, "SubSubModule1.ClassA");
AssertType(SubSubModule1, "any");
AssertType(a2.AisIn1_1_1(), "void");
AssertType(a2.AisIn1_1_1, "() => void");

                    let a3: SubModule1.SubSubModule1.ClassA; a3.AisIn1_1_1();
AssertType(a3, "SubSubModule1.ClassA");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a3.AisIn1_1_1(), "void");
AssertType(a3.AisIn1_1_1, "() => void");

                    let a4: TopLevelModule1.SubModule1.SubSubModule1.ClassA; a4.AisIn1_1_1();
AssertType(a4, "SubSubModule1.ClassA");
AssertType(TopLevelModule1, "any");
AssertType(SubModule1, "any");
AssertType(SubSubModule1, "any");
AssertType(a4.AisIn1_1_1(), "void");
AssertType(a4.AisIn1_1_1, "() => void");
                    
                    // Interface reference
                    let d2: SubSubModule1.InterfaceX; d2.XisIn1_1_1();
AssertType(d2, "SubSubModule1.InterfaceX");
AssertType(SubSubModule1, "any");
AssertType(d2.XisIn1_1_1(), "any");
AssertType(d2.XisIn1_1_1, "() => any");
                }
            }
        }
    }

    export module SubModule2 {
        export module SubSubModule2 {
            // No code here since these are the mirror of the above calls
            export class ClassA { public AisIn1_2_2() { } }
            export class ClassB { public BisIn1_2_2() { } }
            export class ClassC { public CisIn1_2_2() { } }
            export interface InterfaceY { YisIn1_2_2(); }
            interface NonExportedInterfaceQ { }
        }
        
        export interface InterfaceY { YisIn1_2(); }
    }
    
    class ClassA {
        public AisIn1() { }
    }

    interface InterfaceY {
        YisIn1();
    }
    
    module NotExportedModule {
        export class ClassA { }
    }
}

module TopLevelModule2 {
    export module SubModule3 {
        export class ClassA {
            public AisIn2_3() { }
        }
    }
}



