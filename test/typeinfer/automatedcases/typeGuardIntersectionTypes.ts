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

// === tests/cases/conformance/expressions/typeGuards/typeGuardIntersectionTypes.ts ===
declare function AssertType(value:any, type:string):void;
interface X {
    x: string;
}

interface Y {
    y: string;
}

interface Z {
    z: string;
}

declare function isX(obj: any): obj is X;
declare function isY(obj: any): obj is Y;
declare function isZ(obj: any): obj is Z;

function f1(obj: Object) {
    if (isX(obj) || isY(obj) || isZ(obj)) {
AssertType(isX(obj) || isY(obj) || isZ(obj), "boolean");
AssertType(isX(obj) || isY(obj), "boolean");
AssertType(isX(obj), "boolean");
AssertType(isX, "(any) => obj is X");
AssertType(obj, "Object");
AssertType(isY(obj), "boolean");
AssertType(isY, "(any) => obj is Y");
AssertType(obj, "Object");
AssertType(isZ(obj), "boolean");
AssertType(isZ, "(any) => obj is Z");
AssertType(obj, "Object");

        obj;
AssertType(obj, "union");
    }
    if (isX(obj) && isY(obj) && isZ(obj)) {
AssertType(isX(obj) && isY(obj) && isZ(obj), "boolean");
AssertType(isX(obj) && isY(obj), "boolean");
AssertType(isX(obj), "boolean");
AssertType(isX, "(any) => obj is X");
AssertType(obj, "Object");
AssertType(isY(obj), "boolean");
AssertType(isY, "(any) => obj is Y");
AssertType(obj, "X");
AssertType(isZ(obj), "boolean");
AssertType(isZ, "(any) => obj is Z");
AssertType(obj, "X & Y");

        obj;
AssertType(obj, "X & Y & Z");
    }
}

// Repro from #8911

// two interfaces
interface A {
  a: string;
}

interface B {
  b: string;
}

// a type guard for B
function isB(toTest: any): toTest is B {
AssertType(toTest && toTest.b, "any");
AssertType(toTest, "any");
AssertType(toTest.b, "any");
  return toTest && toTest.b;
}

// a function that turns an A into an A & B
function union(a: A): A & B | null {
  if (isB(a)) {
AssertType(isB(a), "boolean");
AssertType(isB, "(any) => toTest is B");
AssertType(a, "A");

AssertType(a, "A & B");
    return a;

  } else {
AssertType(null, "null");
    return null;
  }
}

// Repro from #9016

declare function log(s: string): void;

// Supported beast features
interface Beast     { wings?: boolean; legs?: number }
interface Legged    { legs: number; }
interface Winged    { wings: boolean; }

// Beast feature detection via user-defined type guards
function hasLegs(x: Beast): x is Legged { 
AssertType(x && typeof x.legs === 'number', "boolean");
return x && typeof x.legs === 'number'; 

AssertType(x, "Beast");

AssertType(typeof x.legs === 'number', "boolean");

AssertType(typeof x.legs, "union");

AssertType(x.legs, "union");

AssertType('number', "string");
}

function hasWings(x: Beast): x is Winged { 
AssertType(x && !!x.wings, "boolean");
return x && !!x.wings; 

AssertType(x, "Beast");

AssertType(!!x.wings, "boolean");

AssertType(!x.wings, "boolean");

AssertType(x.wings, "union");
}

// Function to identify a given beast by detecting its features
function identifyBeast(beast: Beast) {

    // All beasts with legs
    if (hasLegs(beast)) {
AssertType(hasLegs(beast), "boolean");
AssertType(hasLegs, "(Beast) => x is Legged");
AssertType(beast, "Beast");

        // All winged beasts with legs
        if (hasWings(beast)) {
AssertType(hasWings(beast), "boolean");
AssertType(hasWings, "(Beast) => x is Winged");
AssertType(beast, "Legged");

            if (beast.legs === 4) {
AssertType(beast.legs === 4, "boolean");
AssertType(beast.legs, "number");
AssertType(4, "int");

                log(`pegasus - 4 legs, wings`);
AssertType(log(`pegasus - 4 legs, wings`), "void");
AssertType(log, "(string) => void");
AssertType(`pegasus - 4 legs, wings`, "string");
            }
            else if (beast.legs === 2) {
AssertType(beast.legs === 2, "boolean");
AssertType(beast.legs, "number");
AssertType(2, "int");

                log(`bird - 2 legs, wings`);
AssertType(log(`bird - 2 legs, wings`), "void");
AssertType(log, "(string) => void");
AssertType(`bird - 2 legs, wings`, "string");
            }
            else {
                log(`unknown - ${beast.legs} legs, wings`);
AssertType(log(`unknown - ${beast.legs} legs, wings`), "void");
AssertType(log, "(string) => void");
AssertType(`unknown - ${beast.legs} legs, wings`, "string");
AssertType(beast.legs, "number");
            }
        }

        // All non-winged beasts with legs
        else {
            log(`manbearpig - ${beast.legs} legs, no wings`);
AssertType(log(`manbearpig - ${beast.legs} legs, no wings`), "void");
AssertType(log, "(string) => void");
AssertType(`manbearpig - ${beast.legs} legs, no wings`, "string");
AssertType(beast.legs, "number");
        }
    }

    // All beasts without legs    
    else {
        if (hasWings(beast)) {
AssertType(hasWings(beast), "boolean");
AssertType(hasWings, "(Beast) => x is Winged");
AssertType(beast, "Beast");

            log(`quetzalcoatl - no legs, wings`)
AssertType(log(`quetzalcoatl - no legs, wings`), "void");
AssertType(log, "(string) => void");
AssertType(`quetzalcoatl - no legs, wings`, "string");
        }
        else {
            log(`snake - no legs, no wings`)
AssertType(log(`snake - no legs, no wings`), "void");
AssertType(log, "(string) => void");
AssertType(`snake - no legs, no wings`, "string");
        }
    }
}

function beastFoo(beast: Object) {
    if (hasWings(beast) && hasLegs(beast)) {
AssertType(hasWings(beast) && hasLegs(beast), "boolean");
AssertType(hasWings(beast), "boolean");
AssertType(hasWings, "(Beast) => x is Winged");
AssertType(beast, "Object");
AssertType(hasLegs(beast), "boolean");
AssertType(hasLegs, "(Beast) => x is Legged");
AssertType(beast, "Winged");

        beast;  // Winged & Legged
AssertType(beast, "Winged & Legged");
    }
    else {
        beast;
AssertType(beast, "Object");
    }

    if (hasLegs(beast) && hasWings(beast)) {
AssertType(hasLegs(beast) && hasWings(beast), "boolean");
AssertType(hasLegs(beast), "boolean");
AssertType(hasLegs, "(Beast) => x is Legged");
AssertType(beast, "Object");
AssertType(hasWings(beast), "boolean");
AssertType(hasWings, "(Beast) => x is Winged");
AssertType(beast, "Legged");

        beast;  // Legged & Winged
AssertType(beast, "Legged & Winged");
    }
}

