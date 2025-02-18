/*
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
 */

let NONE = 0;
let HOLE = 1;
let PACKEDINT = 2;
let HOLEINT = 3;
let PACKEDNUMBER = 6;
let HOLENUMBER = 7;
let PACKEDSTRING = 8;
let HOLESTRING = 9;
let PACKEDOBJECT = 16;
let HOLEOBJECT = 17;
let PACKEDTAGGED = 30;
let GENERIC = 31;
let DICTIONARY = 31;

// ElementdKind == INT
// Test on ldobjbyvalue
function test1() {
    let a = [1, 2, 34];
    if (a[0] == 1 && a[1] == 2 && a[2] == 34 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test1 - success");
    } else {
        print("test1 - failed");
    }
}

// ElementdKind == INT
// Test on ldobjbyvalue and stobjbyvalue
function test2() {
    let a = [1, 2, 34];
    a[1] = 10;
    if (a[0] == 1 && a[1] == 10 && a[2] == 34 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test2 - success");
    } else {
        print("test2 - failed");
    }
}

// ElementdKind == INT
// Test on ldobjbyvalue
// With Deopt
function test3() {
    let a = [1, 2, 34];
    print(a[10]); // deopt point
    if (a[0] == 1 && a[1] == 2 && a[2] == 34 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test3 - success");
    } else {
        print("test3 - failed");
    }
}

// ElementdKind == INT
// Test on ldobjbyvalue and stobjbyvalue
// With Deopt
function test4() {
    let a = [1, 2, 34];
    print(a[10]); // deopt point
    a[1] = 10;
    if (a[0] == 1 && a[1] == 10 && a[2] == 34 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test4 - success");
    } else {
        print("test4 - failed");
    }
}

// ElementdKind == DOUBLE
// Test on ldobjbyvalue
function test5() {
    let a = [1.5, 3.5, 6.7];
    if (a[0] == 1.5 && a[1] == 3.5 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test5 - success");
    } else {
        print("test5 - failed");
    }
}

// ElementdKind == DOUBLE
// Test on ldobjbyvalue and stobjbyvalue
function test6() {
    let a = [1.5, 3.5, 6.7];
    a[1] = 8.8;
    if (a[0] == 1.5 && a[1] == 8.8 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test6 - success");
    } else {
        print("test6 - failed");
    }
}

// ElementdKind == DOUBLE
// Test on ldobjbyvalue
// With Deopt
function test7() {
    let a = [1.5, 3.5, 6.7];
    print(a[10]); // deopt point
    if (a[0] == 1.5 && a[1] == 3.5 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test7 - success");
    } else {
        print("test7 - failed");
    }
}

// ElementdKind == INT
// Test on ldobjbyvalue and stobjbyvalue
// With Deopt
function test8() {
    let a = [1.5, 3.5, 6.7];
    print(a[10]); // deopt point
    a[1] = 8.8;
    if (a[0] == 1.5 && a[1] == 8.8 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test8 - success");
    } else {
        print("test8 - failed");
    }
}

// ElementdKind == NUMBER
// Test on ldobjbyvalue
function test9() {
    let a = [1.5, 3, 6.7];
    if (a[0] == 1.5 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test9 - success");
    } else {
        print("test9 - failed");
    }
}

// ElementdKind == Number
// Test on ldobjbyvalue stobjbyvalue
function test10() {
    let a = [1.5, 3, 6.7];
    a[0] = 2;
    if (a[0] == 2 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test10 - success");
    } else {
        print("test10 - failed");
    }
}

// ElementdKind == Number
// Test on ldobjbyvalue
// With deopt
function test11() {
    let a = [1.5, 3, 6.7];
    print(a[10]);
    if (a[0] == 1.5 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test11 - success");
    } else {
        print("test11 - failed");
    }
}

// ElementdKind == Number
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test12() {
    let a = [1.5, 3, 6.7];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test12 - success");
    } else {
        print("test12 - failed");
    }
}

// ElementdKind == String
// Test on ldobjbyvalue
function test13() {
    let a = ["o", "p", "q"];
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == PACKEDSTRING) {
        print("test13 - success");
    } else {
        print("test13 - failed");
    }
}

// ElementdKind == String
// Test on ldobjbyvalue stobjbyvalue
function test14() {
    let a = ["o", "p", "q"];
    a[0] = "x";
    if (a[0] == "x" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == PACKEDSTRING) {
        print("test14 - success");
    } else {
        print("test14 - failed");
    }
}

// ElementdKind == Number
// Test on ldobjbyvalue
// With deopt
function test15() {
    let a = ["o", "p", "q"];
    print(a[10]);
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == PACKEDSTRING) {
        print("test15 - success");
    } else {
        print("test15 - failed");
    }
}

// ElementdKind == Number
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test16() {
    let a = ["o", "p", "q"];
    print(a[10]);
    a[0] = "x";
    if (a[0] == "x" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == PACKEDSTRING) {
        print("test16 - success");
    } else {
        print("test16 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on ldobjbyvalue
function test17() {
    let a = [1, , 5];
    if (a[0] == 1 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test17 - success");
    } else {
        print("test17 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue
function test18() {
    let a = [1.5, , 5.6];
    if (a[0] == 1.5 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test18 - success");
    } else {
        print("test18 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on ldobjbyvalue
// With deopt
function test19() {
    let a = [1, , 5];
    print(a[10]);
    if (a[0] == 1 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test19 - success");
    } else {
        print("test19 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue
// With deopt
function test20() {
    let a = [1.5, , 5.6];
    print(a[10]);
    if (a[0] == 1.5 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test20 - success");
    } else {
        print("test20 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on ldobjbyvalue stobjbyvalue
function test21() {
    let a = [1, , 5];
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test21 - success");
    } else {
        print("test21 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue stobjbyvalue
function test22() {
    let a = [1.5, , 5.6];
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test22 - success");
    } else {
        print("test22 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test23() {
    let a = [1, , 5];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test23 - success");
    } else {
        print("test23 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test24() {
    let a = [1.5, , 5.6];
    print(a[10]);
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test24 - success");
    } else {
        print("test24 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on store value at "hole" place
function test25() {
    let a = [1, , 5];
    if (a[1] != undefined) {
        print("test25 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    if (a[1] != 2) {
        print("test25 - failed: store value to hole spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test25 - success");
    } else {
        print("test25 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue stobjbyvalue
function test26() {
    let a = [1.5, , 5.6];
    if (a[1] != undefined) {
        print("test26 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2.2;
    if (a[1] != 2.2) {
        print("test26 - failed: store value to hole spot failed");
    }
    if (a[0] == 1.5 && a[1] == 2.2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test26 - success");
    } else {
        print("test26 - failed");
    }
}

// ElementdKind == HOLE INT
// Test on store value at "hole" place
// With deopt
function test27() {
    let a = [1, , 5];
    if (a[1] != undefined) {
        print("test27 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    print(a[10]);
    if (a[1] != 2) {
        print("test27 - failed: store value to hole spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test27 - success");
    } else {
        print("test27 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test28() {
    let a = [1.5, , 5.6];
    if (a[1] != undefined) {
        print("test28 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2.2;
    print(a[10]);
    if (a[1] != 2.2) {
        print("test28 - failed: store value to hole spot failed");
    }
    if (a[0] == 1.5 && a[1] == 2.2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test28 - success");
    } else {
        print("test28 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on ldobjbyvalue
function test29() {
    let a = [1, , 5.1];
    if (a[0] == 1 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test29 - success");
    } else {
        print("test29 - failed");
    }
}

// ElementdKind == HOLE Tagged
// Test on ldobjbyvalue
function test30() {
    let a = ["o", , "q"];
    if (a[0] == "o" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test30 - success");
    } else {
        print("test30 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on ldobjbyvalue
// With deopt
function test31() {
    let a = [1, , 5.1];
    print(a[10]);
    if (a[0] == 1 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test31 - success");
    } else {
        print("test31 - failed");
    }
}

// ElementdKind == HOLE Tagged
// Test on ldobjbyvalue
// With deopt
function test32() {
    let a = ["o", , "q"];
    print(a[10]);
    if (a[0] == "o" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test32 - success");
    } else {
        print("test32 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on ldobjbyvalue stobjbyvalue
function test33() {
    let a = [1, , 5.1];
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test33 - success");
    } else {
        print("test33 - failed");
    }
}

// ElementdKind == HOLE Tagged
// Test on ldobjbyvalue stobjbyvalue
function test34() {
    let a = ["o", , "q"];
    a[0] = "x";
    if (a[0] == "x" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test34 - success");
    } else {
        print("test34 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test35() {
    let a = [1, , 5.1];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test35 - success");
    } else {
        print("test35 - failed");
    }
}

// ElementdKind == HOLE Tagged
// Test on ldobjbyvalue stobjbyvalue
// With deopt
function test36() {
    let a = ["o", , "q"];
    print(a[10]);
    a[0] = "x";
    if (a[0] == "x" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test36 - success");
    } else {
        print("test36 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on store value at "hole" place
function test37() {
    let a = [1, , 5.6];
    if (a[1] != undefined) {
        print("test37 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    if (a[1] != 2) {
        print("test37 - failed: store value to hole spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test37 - success");
    } else {
        print("test37 - failed");
    }
}

// ElementdKind == HOLE Tagged
// Test on store value at "hole" place
function test38() {
    let a = ["o", , "q"];
    if (a[1] != undefined) {
        print("test38 - failed: a[1] should be undefined");
        return;
    }
    a[1] = "p";
    if (a[1] != "p") {
        print("test38 - failed: store value to hole spot failed");
    }
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test38 - success");
    } else {
        print("test38 - failed");
    }
}

// ElementdKind == HOLE NUMBER
// Test on store value at "hole" place
// With deopt
function test39() {
    let a = [1, , 5.1];
    if (a[1] != undefined) {
        print("test39 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    print(a[10]);
    if (a[1] != 2) {
        print("test39 - failed: store value to hole spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5.1 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test39 - success");
    } else {
        print("test39 - failed");
    }
}

// ElementdKind == HOLE DOUBLE
// Test on store value at "hole" place
// With deopt
function test40() {
    let a = ["o", , "q"];
    if (a[1] != undefined) {
        print("test40 - failed: a[1] should be undefined");
        return;
    }
    a[1] = "p";
    print(a[10]);
    if (a[1] != "p") {
        print("test40 - failed: store value to hole spot failed");
    }
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == HOLESTRING) {
        print("test40 - success");
    } else {
        print("test40 - failed");
    }
}

// Transition Int -> Number
// Load Stable Array Check deopt
function test41() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test41 - failed");
    }
    a[1] = 2.5;
    if (a[0] == 1 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test41 - success");
    } else {
        print("test41 - failed");
    }
}

// Transition Int -> Number
function test42() {
    let a = [1, 2, 3];
    a[1] = 2.5;
    if (a[0] == 1 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test42 - success");
    } else {
        print("test42 - failed");
    }
}

// Transition Int -> Number
// Store Stable Array Check deopt
function test43() {
    let a = [1, 2, 3];
    a[0] = 4;
    a[1] = 2.5;
    if (a[0] == 4 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test43 - success");
    } else {
        print("test43 - failed");
    }
}

// Transition Int -> TaggedObject
// Load Stable Array Check deopt
function test44() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test44 - failed");
    }
    a[1] = "1";
    if (a[0] == 1 && a[1] == "1" && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test44 - success");
    } else {
        print("test44 - failed");
    }
}

// Transition Number -> Number
function test45() {
    let a = [1, 2.2, 3];
    if (a[0] != 1 || a[1] != 2.2 || a[2] != 3) {
        print("test45 - failed");
    }
    a[1] = 2;
    if (a[0] == 1 && a[1] == 2 && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test45 - success");
    } else {
        print("test45 - failed");
    }
}

// Transition Number -> TaggedObject
function test46() {
    let a = [1, 2.2, 3];
    if (a[0] != 1 || a[1] != 2.2 || a[2] != 3) {
        print("test46 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test46 - success");
    } else {
        print("test46 - failed");
    }
}

// Transition Int -> TaggedObject
function test47() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test47 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test47 - success");
    } else {
        print("test47 - failed");
    }
}

// Transition TaggedObject -> TaggedObject
// Load and Store has type TaggedObject
// Because it uses receiver type in pgo
function test48() {
    let a = [1, "2"];
    if (a[0] != 1 || a[1] != "2") {
        print("test48 - failed");
    }
    a[1] = 2;
    if (a[0] == 1 && a[1] == 2 && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test48 - success");
    } else {
        print("test48 - failed");
    }
}

// Transition TaggedObject -> TaggedObject
function test49() {
    let a = [1, "2"];
    if (a[0] != 1 || a[1] != "2") {
        print("test49 - failed");
    }
    a[0] = "1";
    if (a[0] == "1" && a[1] == "2" && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test49 - success");
    } else {
        print("test49 - failed");
    }
}

// Transition Hole_Int -> Hole_Tagged
// Has deopt at load
function test50() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test50 - failed");
    }
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test50 - success");
    } else {
        print("test50 - failed");
    }
}

// Transition Hole_Int -> Hole_Tagged
// No Deopt
function test51() {
    let a = [1, , 3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test51 - success");
    } else {
        print("test51 - failed");
    }
}

// Transition Hole_Int -> Hole_Number
// Has deopt at load
function test52() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test52 - failed");
    }
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test52 - success");
    } else {
        print("test52 - failed");
    }
}

// Transition Hole_Int -> Hole_Number
// No Deopt
function test53() {
    let a = [1, , 3] 
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test53 - success");
    } else {
        print("test53 - failed");
    }
}

// Transition Hole_Double -> Hole_Tagged
// Has deopt at load
function test54() {
    let a = [1.1, , 3.3] 
    if (a[0] != 1.1 || a[1] != undefined || a[2] != 3.3) {
        print("test54 - failed");
    }
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test54 - success");
    } else {
        print("test54 - failed");
    }
}

// Transition Hole_Double -> Hole_Tagged
// No Deopt
function test55() {
    let a = [1.1, , 3.3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test55 - success");
    } else {
        print("test55 - failed");
    }
}

// Transition Hole_Number -> Hole_Tagged
// Has deopt at load
function test56() {
    let a = [1, , 3.3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3.3) {
        print("test56 - failed");
    }
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test56 - success");
    } else {
        print("test56 - failed");
    }
}

// Transition Hole_Number -> Hole_Tagged
// No Deopt
function test57() {
    let a = [1, , 3.3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test57 - success");
    } else {
        print("test57 - failed");
    }
}

// Transition Hole_Int -> Hole_Number
// Has deopt at load
// Store at hole place
function test58() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test58 - failed");
    }
    a[1] = 2.2;
    if (a[0] == 1 && a[1] == 2.2 && a[2] == 3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test58 - success");
    } else {
        print("test58 - failed");
    }
}

// Transition Hole_Int -> Hole_Number
// No Deopt
// Store at hole place
function test59() {
    let a = [1, , 3] 
    a[1] = 2.2;
    if (a[0] == 1 && a[1] == 2.2 && a[2] == 3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test59 - success");
    } else {
        print("test59 - failed");
    }
}

// Transition Hole_Int -> Hole_Tagged
// Has deopt at load
// Store at hole place
function test60() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test60 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test60 - success");
    } else {
        print("test60 - failed");
    }
}

// Transition Hole_Int -> Hole_Tagged
// No Deopt
// Store at hole place
function test61() {
    let a = [1, , 3] 
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test61 - success");
    } else {
        print("test61 - failed");
    }
}

// Transition Hole_Double -> Hole_Number
// Has deopt at load
// Store at hole place
function test62() {
    let a = [1.1, , 3.3] 
    if (a[0] != 1.1 || a[1] != undefined || a[2] != 3.3) {
        print("test62 - failed");
    }
    a[1] = 2;
    if (a[0] == 1.1 && a[1] == 2 && a[2] == 3.3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test62 - success");
    } else {
        print("test62 - failed");
    }
}

// Transition Hole_Double -> Hole_Number
// No Deopt
// Store at hole place
function test63() {
    let a = [1.1, , 3.3] 
    a[1] = 2;
    if (a[0] == 1.1 && a[1] == 2 && a[2] == 3.3 && ArkTools.getElementsKind(a) == HOLENUMBER) {
        print("test63 - success");
    } else {
        print("test63 - failed");
    }
}

// Transition Hole_Double -> Hole_Tagged
// Has deopt at load
// Store at hole place
function test64() {
    let a = [1.1, , 3.3] 
    if (a[0] != 1.1 || a[1] != undefined || a[2] != 3.3) {
        print("test64 - failed");
    }
    a[1] = "2";
    if (a[0] == 1.1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test64 - success");
    } else {
        print("test64 - failed");
    }
}

// Transition Hole_Double -> Hole_Tagged
// No Deopt
// Store at hole place
function test65() {
    let a = [1.1, , 3.3] 
    a[1] = "2";
    if (a[0] == 1.1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test65 - success");
    } else {
        print("test65 - failed");
    }
}

// Transition Hole_Number -> Hole_Tagged
// Has deopt at load
// Store at hole place
function test66() {
    let a = [1, , 3.3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3.3) {
        print("test66 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test66 - success");
    } else {
        print("test66 - failed");
    }
}

// Transition Hole_Number -> Hole_Tagged
// No Deopt
// Store at hole place
function test67() {
    let a = [1, , 3.3] 
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test67 - success");
    } else {
        print("test67 - failed");
    }
}

// Transition Hole_Tagged -> Hole_Tagged
// no deopt at load
// Store at hole place
function test68() {
    let a = [1, , "3"];
    if (a[0] != 1 || a[1] != undefined || a[2] != "3") {
        print("test68 - failed");
    }
    a[0] = "1";
    a[1] = "2";
    if (a[0] == "1" && a[1] == "2" && a[2] == "3" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test68 - success");
    } else {
        print("test68 - failed");
    }
}

// Transition Hole_Tagged -> Hole_Tagged
// No Deopt
// Store at hole place
function test69() {
    let a = [1, , "3"]; 
    a[0] = "1";
    a[1] = "2";
    if (a[0] == "1" && a[1] == "2" && a[2] == "3" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test69 - success");
    } else {
        print("test69 - failed");
    }
}

// Compatibility test
// Packed Int Array
function test70() {
    let a = [1, 2, 3];
    let result = "Output: " + a;
    let expected = "Output: 1,2,3";
    if (result == expected) {
        print("test70 - success");
    } else {
        print("test70 - failed");
    }
}

// Compatibility test
// Packed Number Array
function test71() {
    let a = [1.1, 2.2, 3.3];
    let result = "Output: " + a;
    let expected = "Output: 1.1,2.2,3.3";
    if (result == expected) {
        print("test71 - success");
    } else {
        print("test71 - failed");
    }
}

// Compatibility test
// Packed Int and Number Array
function test72() {
    let a = [1, 2.2, 3.3];
    let result = "Output: " + a;
    let expected = "Output: 1,2.2,3.3";
    if (result == expected) {
        print("test72 - success");
    } else {
        print("test72 - failed");
    }
}

// Compatibility test
// Holely Int Array
function test73() {
    let a = [1, , 3];
    let result = "Output: " + a;
    let expected = "Output: 1,,3";
    if (result == expected) {
        print("test73 - success");
    } else {
        print("test73 - failed");
    }
}

// Compatibility test
// Holely Number Array
function test74() {
    let a = [1.1, , 3.3];
    let result = "Output: " + a;
    let expected = "Output: 1.1,,3.3";
    if (result == expected) {
        print("test74 - success");
    } else {
        print("test74 - failed");
    }
}

// Compatibility test
// Holely Int and Number Array
function test75() {
    let a = [1, , 3.3];
    let result = "Output: " + a;
    let expected = "Output: 1,,3.3";
    if (result == expected) {
        print("test75 - success");
    } else {
        print("test75 - failed");
    }
}

// Compatibility test
// Add Transition Int -> Number
function test76() {
    let a = [1, 2, 3];
    a[1] = 2.2;
    let result = "Output: " + a;
    let expected = "Output: 1,2.2,3";
    if (result == expected) {
        print("test76 - success");
    } else {
        print("test76 - failed");
    }
}

// Compatibility test
// Add Transition Int -> Tagged
function test77() {
    let a = [1, 2, 3];
    a[1] = "2";
    let result = "Output: " + a;
    let expected = "Output: 1,2,3";
    if (result == expected) {
        print("test77 - success");
    } else {
        print("test77 - failed");
    }
}

// Compatibility test
// Add Transition Number -> Tagged
function test78() {
    let a = [1.1, 2.2, 3.3];
    a[1] = "2.2";
    let result = "Output: " + a;
    let expected = "Output: 1.1,2.2,3.3";
    if (result == expected) {
        print("test78 - success");
    } else {
        print("test78 - failed");
    }
}

// Compatibility test
// Add Transition Int and Number -> Tagged
function test79() {
    let a = [1, 2.2, 3.3];
    a[1] = "2";
    let result = "Output: " + a;
    let expected = "Output: 1,2,3.3";
    if (result == expected) {
        print("test79 - success");
    } else {
        print("test79 - failed");
    }
}

// Compatibility test
// Add Holely Transition HoleNumber -> Tagged
function test80() {
    let a = [1, , 3];
    a[1] = 2;
    let result = "Output: " + a;
    let expected = "Output: 1,2,3";
    if (result == expected) {
        print("test80 - success");
    } else {
        print("test80 - failed");
    }
}

// Compatibility test
// Add Holely Transition HoleIntNumber -> Tagged
function test81() {
    let a = [1, , 3.3];
    a[1] = 2.2;
    let result = "Output: " + a;
    let expected = "Output: 1,2.2,3.3";
    if (result == expected) {
        print("test81 - success");
    } else {
        print("test81 - failed");
    }
}

// Compatibility test
// Packed Tagged Array
function test82() {
    let a = ["1", "2", "3"];
    let result = "Output: " + a;
    let expected = "Output: 1,2,3";
    if (result == expected) {
        print("test82 - success");
    } else {
        print("test82 - failed");
    }
}

// Compatibility test
// Add Holely Transition HoleTagged -> Tagged
function test83() {
    let a = ["1", , "3"];
    a[1] = "2";
    let result = "Output: " + a;
    let expected = "Output: 1,2,3";
    if (result == expected) {
        print("test83 - success");
    } else {
        print("test83 - failed");
    }
}

// BuiltinArray Array.constructor
function test84() {
    let a = new Array(5);
    if (ArkTools.getElementsKind(a) == HOLE) {
        print("test84 - success");
    } else {
        print("test84 - failed");
    }
}

function test85() {
    let a = new Array(1, 2, 3);
    if (ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test85 - success");
    } else {
        print("test85 - failed");
    }
}

function test86() {
    let a = new Array(1, 1.5, 3);
    if (ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test86 - success");
    } else {
        print("test86 - failed");
    }
}

function test87() {
    let a = new Array("1", 1.5, 3);
    if (ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test87 - success");
    } else {
        print("test87 - failed");
    }
}

function test88() {
    let a = new Array("1", "2", "3");
    if (ArkTools.getElementsKind(a) == PACKEDSTRING) {
        print("test88 - success");
    } else {
        print("test88 - failed");
    }
}

function test89() {
    let a = Array.of(1, 2, 3);
    if (a.length == 3 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test89 - success");
    } else {
        print("test89 - failed");
    }
}

function test90() {
    let a = Array.of(3);
    if (a.length == 1 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test90 - success");
    } else {
        print("test90 - failed");
    }
}

// Question? 
// Array is constructed from callback
function test91() {
    let result = 0;
    function NotArray(len) {
      result = len;
    }

    let a = Array.of.call(NotArray, 1, 2, 3);
    if (result == 3 && a.length == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test91 - success");
    } else {
        print("test91 - failed");
    }
}

// Array.concat
function test92() {
    const num1 = [1, 2, 3];
    const num2 = [4, 5, 6];

    const a = num1.concat(num2);
    if (a.length == 6 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test92 - success");
    } else {
        print("test92 - failed");
    }
}

function test93() {
    const num1 = [1, 2, 3];
    const num2 = [4.4, 5.5, 6.6];

    const a = num1.concat(num2);
    if (a.length == 6 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test93 - success");
    } else {
        print("test93 - failed");
    }
}

function test94() {
    let a = [1, , 3].concat([4, 5]); // [1, empty, 3, 4, 5]
    if (a.length == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test94 - success");
    } else {
        print("test94 - failed");
    }
}

function test95() {
    let a = [1, , 3].concat([4, 5]); // [1, empty, 3, 4, 5]
    if (a.length == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test95 - success");
    } else {
        print("test95 - failed");
    }
}

function test96() {
    const obj1 = { 0: 1, 1: 2, 2: 3, length: 3 };
    const obj2 = { 0: 1, 1: 2, 2: 3, length: 3, [Symbol.isConcatSpreadable]: true };
    let a = [0].concat(obj1, obj2);
    if (a.length == 5 && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test96 - success");
    } else {
        print("test96 - failed");
    }
}

// Array.copyWithin
function test97() {
    let a = [1, 2, 3, 4, 5].copyWithin(0, 3); // [4, 5, 3, 4, 5]
    if (a.length == 5 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test97 - success");
    } else {
        print("test97 - failed");
    }
}

function test98() {
    let a = [1, , 3].copyWithin(2, 1, 2); // [1, empty, empty]
    if (a.length == 3 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test98 - success");
    } else {
        print("test98 - failed");
    }
}

// Array.entries
function test99() {
    const a = ["a", "b", "c"];
    let i = 0;
    let flag = true;
    for (const [index, element] of a.entries()) {
        if (element != a[i]) {
            flag = false;
        }
        i++;
    }
    if (flag) {
        print("test99 - success");
    } else {
        print("test99 - failed");
    }
}

// Array.every
function test100() {
    const isBelowThreshold = (currentValue) => currentValue < 40;
    const array1 = [1, 30, 39, 29, 10, 13];
    // Expected output: true
    if (array1.every(isBelowThreshold)) {
        print("test100 - success");
    } else {
        print("test100 - failed");
    }
}

// Array.fill
function test101() {
    const array1 = [1, 2, 3, 4];
    // Fill with 0 from position 2 until position 4
    array1.fill(0, 2, 4);
    // Expected output: Array [1, 2, 0, 0]
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == PACKEDINT){
        print("test101 - success");
    } else {
        print("test101 - failed");
    }
}

function test102() {
    const array1 = [1, 2, 3, 4];
    // Fill with 5 from position 1
    array1.fill(5.5, 1);
    // Expected output: Array [1, 5, 5, 5]
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == PACKEDNUMBER){
        print("test102 - success");
    } else {
        print("test102 - failed");
    }
}

function test103() {
    const array1 = [1, 2, 3, 4];
    array1.fill("6");
    // Expected output: Array [6, 6, 6, 6]
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == PACKEDSTRING){
        print("test103 - success");
    } else {
        print("test103 - failed");
    }
}

// Array.filter
function test104() {
    const words = ['spray', 'elite', 'exuberant', 'destruction', 'present'];
    const result = words.filter((word) => word.length > 6);
    // Expected output: Array ["exuberant", "destruction", "present"]
    if (result.length == 3 && ArkTools.getElementsKind(result) == PACKEDSTRING){
        print("test104 - success");
    } else {
        print("test104 - failed");
    }
}

function test105() {
    const arrayLike = {
      length: 3,
      0: "a",
      1: "b",
      2: "c",
      3: "a", // ignored by filter() since length is 3
    };
    let result = Array.prototype.filter.call(arrayLike, (x) => x <= "b");
    // [ 'a', 'b' ]
    if (result.length == 2 && ArkTools.getElementsKind(result) == PACKEDSTRING){
        print("test105 - success");
    } else {
        print("test105 - failed");
    }
}

function test106() {
    let result = [1, , undefined].filter((x) => x === undefined); // [undefined]
    if (result.length == 1 && ArkTools.getElementsKind(result) == GENERIC){
        print("test106 - success");
    } else {
        print("test106 - failed");
    }
}

// Array.find
function test107() {
    const array1 = [5, 12, 8, 130, 44];
    const found = array1.find((element) => element > 10);
    // Expected output: 12
    if (found == 12) {
        print("test107 - success");
    } else {
        print("test107 - failed");
    }
}

// Array.findIndex
function test108() {
    const array1 = [5, 12, 8, 130, 44];
    const isLargeNumber = (element) => element > 13;
    let result = array1.findIndex(isLargeNumber);
    // Expected output: 3
    if (result == 3) {
        print("test108 - success");
    } else {
        print("test108 - failed");
    }
}

// Array.findLast
function test109() {
    const array1 = [5, 12, 50, 130, 44];
    const found = array1.findLast((element) => element > 45);
    // Expected output: 130
    if (found == 130) {
        print("test109 - success");
    } else {
        print("test109 - failed");
    }
}

// Array.findLastIndex
function test110() {
    const array1 = [5, 12, 50, 130, 44];
    const isLargeNumber = (element) => element > 45;
    let result = array1.findLastIndex(isLargeNumber);
    // Expected output: 3
    // Index of element with value: 130
    if (result == 3) {
        print("test110 - success");
    } else {
        print("test110 - failed");
    }
}

// Array.flat
function test111() {
    const arr1 = [0, 1, 2, [3, 4]];
    let result = arr1.flat();
    // expected output: Array [0, 1, 2, 3, 4]

    if (result.length == 5 && ArkTools.getElementsKind(result) == PACKEDINT) {
        print("test111 - success");
    } else {
        print("test111 - failed");
    }
}

function test112() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat();
    // expected output: Array [0, 1, 2, Array [3, Array [4, 5]]]

    if (result.length == 4 && ArkTools.getElementsKind(result) == PACKEDTAGGED) {
        print("test112 - success");
    } else {
        print("test112 - failed");
    }
}

function test113() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat(2);
    // expected output: Array [0, 1, 2, 3, Array [4, 5]]

    if (result.length == 5 && ArkTools.getElementsKind(result) == PACKEDTAGGED) {
        print("test113 - success");
    } else {
        print("test113 - failed");
    }
}

function test114() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat(Infinity);
    // expected output: Array [0, 1, 2, 3, 4, 5]

    if (result.length == 6 && ArkTools.getElementsKind(result) == PACKEDINT) {
        print("test114 - success");
    } else {
        print("test114 - failed");
    }
}

// Array.flatMap
function test115() {
    const arr1 = [1, 2, 1];
    const result = arr1.flatMap((num) => (num === 2 ? [2, 2] : 1));
    // Expected output: Array [1, 2, 2, 1]
    if (result.length == 4 && ArkTools.getElementsKind(result) == PACKEDINT) {
        print("test115 - success");
    } else {
        print("test115 - failed");
    }
}

function test116() {
    const arr1 = [1, 2, 1];
    const result = arr1.flatMap((num) => (num === 2 ? 2.2 : 1));
    // Expected output: Array [1, 2, 2, 1]
    if (result.length == 3 && ArkTools.getElementsKind(result) == PACKEDNUMBER) {
        print("test116 - success");
    } else {
        print("test116 - failed");
    }
}

// Array.forEach
function test117() {
    const array1 = [1, 2, 3];
    let result = 0;
    array1.forEach((element) => { result += element; });
    if (result == 6) {
        print("test117 - success");
    } else {
        print("test117 - failed");
    }
}

// Array.includes
function test118() {
    const array1 = [1, 2, 3];
    let result = true;
    // Expected output: true
    if (!array1.includes(2) && ArkTools.getElementsKind(array1) == PACKEDINT) {
        result = false;
    }

    const pets = ['cat', 'dog', 'bat'];
    // Expected output: true
    if (!pets.includes('cat')) {
        result = false;
    }
    // Expected output: false
    if (pets.includes('at')) {
        result = false;
    }

    if (result) {
        print("test118 - success");
    } else {
        print("test118 - failed");
    }
}

// Array.Indexof
function test119() {
    const beasts = ['ant', 'bison', 'camel', 'duck', 'bison'];
    let result = true;
    let index1 = beasts.indexOf('bison');
    // Expected output: 1
    if (index1 != 1) {
        result = false;
    }

    // Start from index 2
    let index2 = beasts.indexOf('bison', 2);
    // Expected output: 4
    if (index2 != 4) {
        result = false;
    }

    let index3 = beasts.indexOf('giraffe');
    // Expected output: -1
    if (index3 != -1) {
        result = false;
    }
    if (result) {
        print("test119 - success");
    } else {
        print("test119 - failed");
    }
}

// Array.LastIndexof
function test120() {
    const animals = ['Dodo', 'Tiger', 'Penguin', 'Dodo'];
    let result = true;
    let index1 = animals.lastIndexOf('Dodo');
    // Expected output: 3
    if (index1 != 3) {
        result = false;
    }

    let index2 = animals.lastIndexOf('Tiger');
    // Expected output: 1
    if (index2 != 1) {
        result = false;
    }
    if (result) {
        print("test120 - success");
    } else {
        print("test120 - failed");
    }
}

// Array.join
function test121() {
    const elements = ['Fire', 'Air', 'Water'];
    let result = true;
    let joined1 = elements.join();
    // Expected output: "Fire,Air,Water"
    if (joined1 != "Fire,Air,Water") {
        result = false;
    }

    let joined2 = elements.join('');
    // Expected output: "FireAirWater"
    if (joined2 != "FireAirWater") {
        result = false;
    }

    let joined3 = elements.join('-');
    // Expected output: "Fire-Air-Water"
    if (joined3 != "Fire-Air-Water") {
        result = false;
    }
    if (result) {
        print("test121 - success");
    } else {
        print("test121 - failed");
    }
}

// Array.map
function test122() {
    const array1 = [1, 4, 9, 16];

    // Pass a function to map
    const map1 = array1.map((x) => x * 2);
    // Expected output: Array [2, 8, 18, 32]

    if (map1.length == 4 && ArkTools.getElementsKind(map1) == HOLEINT) {
        print("test122 - success");
    } else {
        print("test122 - failed");
    }
}

function test123() {
    const array1 = [1, 4, 9, 16];

    // Pass a function to map
    const map1 = array1.map((x) => x * 1.1);
    // Expected output: Array [2, 8, 18, 32]

    if (map1.length == 4 && ArkTools.getElementsKind(map1) == HOLENUMBER) {
        print("test123 - success");
    } else {
        print("test123 - failed");
    }
}

// Array.pop
function test124() {
    const plants = ['broccoli', 'cauliflower', 'cabbage', 'kale', 'tomato'];

    let popResult = plants.pop();
    // Expected output: "tomato"
    if (popResult == "tomato" && ArkTools.getElementsKind(plants) == PACKEDSTRING) {
        print("test124 - success");
    } else {
        print("test124 - failed");
    }
}

function test125() {
    const a = [1, 2, 3, 4, 5.5];
    let popResult = a.pop();
    // Expected output: "tomato"
    if (popResult == 5.5 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test125 - success");
    } else {
        print("test125 - failed");
    }
}

// Array.toSort
function test126() {
    let a = [6, 5, 4, 3, 2, 1];
    let b = a.toSorted();
    if (b.length == 6 && ArkTools.getElementsKind(b) == HOLEINT) {
        print("test126 - success");
    } else {
        print("test126 - failed, expected: " + HOLEINT + " , but get: " + ArkTools.getElementsKind(b));
    }
}

// Array.unshift
function test127() {
    let a = [0, 1, 2];
    a.unshift(-1);
    if (a.length == 4 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test127 - success");
    } else {
        print("test127 - failed, expected: " + PACKEDINT + " , but get: " + ArkTools.getElementsKind(a));
    }
}

function test128() {
    let a = [0, 1, 2];
    a.unshift(-2, -1);
    if (a.length == 5 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test128 - success");
    } else {
        print("test128 - failed, expected: " + PACKEDINT + " , but get: " + ArkTools.getElementsKind(a));
    }
}

function test129() {
    // Array.prototype.toSpliced(start, deleteCount, item1, item2, ..., itemN)
    let a = [0, 1, 2, 3, 4, 5, 6];
    let b = a.toSpliced(2, 2, 3, 3);
    if (b.length == 7 && ArkTools.getElementsKind(b) == PACKEDINT) {
        print("test129 - success");
    } else {
        print("test129 - failed, expected: " + PACKEDINT + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test130() {
    // Array.prototype.toSpliced(start, deleteCount, item1, item2, ..., itemN)
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.toSpliced(2, 1, 3);
    if (b.length == 7 && ArkTools.getElementsKind(b) == PACKEDTAGGED) {
        print("test130 - success");
    } else {
        print("test130 - failed, expected: " + PACKEDTAGGED + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test131() {
    // Array.prototype.toSpliced(start, deleteCount, item1, item2, ..., itemN)
    let a = [0, 1, 2, 3, 4, 5, 6];
    let b = a.toSpliced(2, 1, 3.1);
    if (b.length == 7 && ArkTools.getElementsKind(b) == PACKEDNUMBER) {
        print("test131 - success");
    } else {
        print("test131 - failed, expected: " + PACKEDNUMBER + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test132() {
    // Array.prototype.toReversed()
    let a = [0, 1, 2, 4, 5, 6];
    let b = a.toReversed();
    if (b.length == 6 && ArkTools.getElementsKind(b) == PACKEDINT) {
        print("test132 - success");
    } else {
        print("test132 - failed, expected: " + PACKEDINT + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test133() {
    // Array.prototype.toReversed()
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.toReversed();
    if (b.length == 7 && ArkTools.getElementsKind(b) == PACKEDTAGGED) {
        print("test133 - success");
    } else {
        print("test133 - failed, expected: " + PACKEDTAGGED + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test134() {
    // Array.prototype.at(index)
    let a = [0, 1, 2, ,4, 5, 6];
    if (a.length == 7 && a.at(2) == 2 && a.at(3) == undefined) {
        print("test134 - success");
    } else {
        print("test134 - failed, a.At(2): " + a[2] + " , but get: a.At(3): " + a[3]);
    }
}

function test135() {
    // Array.prototype.with(index, value)
    let a = [0, 1, 2, ,4, 5, 6];
    let b = a.with(3, 6);
    if (b.length == 7 && b[3] == 6 && ArkTools.getElementsKind(b) == PACKEDINT) {
        print("test135 - success");
    } else {
        print("test135 - failed, expected: " + PACKEDINT + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test136() {
    let a = [1, 2, 3, 4, 5];
    a.push(5.5);
    if (a.length == 6 && a[5] == 5.5 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test136 - success");
    } else {
        print("test136 - failed");
    }
}

function test137() {
    let a = [1, 2, 3, 4, 5];
    a.push(undefined);
    if (a.length == 6 && a[5] == undefined && ArkTools.getElementsKind(a) == PACKEDTAGGED) {
        print("test137 - success");
    } else {
        print("test137 - failed");
    }
}

function test138() {
    let a = [1, 2, , 4, 5];

    const initialValue = 0;
    const sumWithInitial = a.reduce(
        (accumulator, currentValue) => accumulator + currentValue,
        initialValue,
    );

    if (sumWithInitial == 12) {
        print("test138 - success");
    } else {
        print("test138 - failed");
    }
}

function test139() {
    let a = [1, 2, , 4, 5];

    const initialValue = 0;
    const sumWithInitial = a.reduceRight(
        (accumulator, currentValue) => accumulator + currentValue,
        initialValue,
    );

    if (sumWithInitial == 12) {
        print("test139 - success");
    } else {
        print("test139 - failed");
    }
}

function test140() {
    let a = [1, 2, 3, 4, 5];
    a.reverse();
    if (a.length == 5 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test140 - success");
    } else {
        print("test140 - failed");
    }
}

function test141() {
    let a = [1, 2, , 4, 5];
    a.reverse();
    if (a.length == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test141 - success");
    } else {
        print("test141 - failed");
    }
}

function test142() {
    // Array.prototype.shift()
    let a = [0.1, 1, 2, 3, 4, 5, 6];
    let result = a.shift();
    if (a.length == 6 && result == 0.1 && ArkTools.getElementsKind(a) == PACKEDNUMBER) {
        print("test142 - success");
    } else {
        print("test142 - failed");
    }
}

function test143() {
    // Array.prototype.shift()
    let a = [0, 1, 2, , 4, 5, 6];
    let result = a.shift();
    if (a.length == 6 && result == 0 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test143 - success");
    } else {
        print("test143 - failed");
    }
}

function test144() {
    // Array.prototype.slice()
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.slice();
    if (b.length == 7 && ArkTools.getElementsKind(b) == HOLEINT) {
        print("test144 - success");
    } else {
        print("test144 - failed");
    }
}

function test145() {
    let a = [1, , 4, 21, 100000];
    a.sort();
    if (a[0] == 1 && a[1] == 100000 && a.length == 5 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test145 - success");
    } else {
        print("test145 - failed");
    }
}

function test146() {
    // Array.prototype.splice()
    let a = [0, 1, 2, 3, 4, 5, 6];
    a.splice(2, 2, 3, 3);
    if (a.length == 7 && ArkTools.getElementsKind(a) == PACKEDINT) {
        print("test146 - success");
    } else {
        print("test146 - failed");
    }
}

function test147() {
    // Array.prototype.splice()
    let a = [0, 1, 2, , 4, 5, 6];
    a.splice(2, 1, 3);
    if (a.length == 7 && ArkTools.getElementsKind(a) == HOLEINT) {
        print("test147 - success");
    } else {
        print("test147 - failed");
    }
}

//fast Array.reverse
function test148() {
    let a = [];
    let b = a.reverse();
    let c = [8, 6, 4, 2];
    let d = [2, 4, 6, 8].reverse();
    let e = [1,2,,4];
    let f = e.reverse();
    if (a == b && c[1] == d[1] && f[0] == 4 &&
        f[1] == undefined) {
        print("test148 - success");
    } else {
        print("test148 - failed");
    }
}

//fast Array.reverse
function test149() {
    let a = [0.4, 0.6, 0.8];
    let b = a.reverse();
    let c = ["str2", "str3", "str4"];
    let d = c.reverse();
    let proto = { 0: "foo", 19: "bar" };
    let obj = { length: 20, 5: "foobar", __proto__: proto };
    Array.prototype.reverse.call(obj);

    if (b[0] == 0.8 && c[0] == "str4" && obj[0] == "bar" &&
        obj[19] == "foo") {
        print("test149 - success");
    } else {
        print("test149 - failed");
    }
}

// Int
test1();
test2();
test3();
test4();

// Double
test5();
test6();
test7();
test8();

// Number
test9();
test10();
test11();
test12();

// String
test13();
test14();
test15();
test16();

// Hole related tests
// Double and Int
test17();
test18();
test19();
test20();
test21();
test22();
test23();
test24();
test25();
test26();
test27();
test28();
// Number and Tagged
test29();
test30();
test31();
test32();
test33();
test34();
test35();
test36();
test37();
test38();
test39();
test40();

// Transition - Packed
test41();
test42();
test43();
test44();
test45();
test46();
test47();
test48();
test49();

// Transition - Holey
test50();
test51();
test52();
test53();
test54();
test55();
test56();
test57();
test58();
test59();
test60();
test61();
test62();
test63();
test64();
test65();
test66();
test67();
test68();
test69();

// Compatibility test
test70();
test71();
test72();
test73();
test74();
test75();
test76();
test77();
test78();
test79();
test80();
test81();
test82();
test83();

// Builtins Test
// Array.Constructor
test84();
test85();
test86();
test87();
test88();

// Array.of
test89();
test90();
test91();

// Array.concat
test92();
test93();
test94();
test95();
test96();

// Array.copywithin
test97();
test98();

// Array.entries
test99();

// Array.every
test100();

// Array.fill
test101();
test102();
test103();

// Array.filter
test104();
test105();
test106();

// Array.find
// Array.findIndex
// Array.findLast
// Array.findLastIndex
test107();
test108();
test109();
test110();

// Array.flat
test111();
test112();
test113();
test114();

// Array.flatMap
test115();
test116();

// Array.ForEach
test117();

// Array.Includes
test118();

// Array.indexof
test119();

// Array.LastIndexof
test120();

// Array.join
test121();

// Array.map
test122();
test123();

// Array.pop
test124();
test125();

// Array.toSort
test126();

// Array.unshift
test127();
test128();

// Array.toSplice
test129();
test130();
test131();

// Array.reverse
test132();
test133();

// Array.at
test134();

// Array.with
test135();

// Array.push
test136();
test137();

// Array.reduce
// Array.reduceRight
test138();
test139();

// Array.reverse
test140();
test141();

// Array.shift
test142();
test143();

// Array.slice
test144(); // gc bug

// Array.sort
test145();

// Array.splice
test146();
test147();

test148();
test149();

// Test COW ElementsKind
class Index {
    currentArrays = [
        [0, 0, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 0, 0]
      ]

    changeCurretArrays() {
        let newArrays = [
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0]
        ]

        for (let j = 0; j < 4; j++) {
            for (let i = 0; i < 4; i++) {
                newArrays[j][i] = this.currentArrays[j][i] + 1;
            }
        }
        return newArrays;
    }

    computeScore(array) {
        let total = 0;
        for (let j = 0; j < 4; j++) {
            for (let i = 0; i < 4; i++) {
                total  += array[j][i];
            }
        }
        return total;
    }

    run() {
        let newArray = this.changeCurretArrays();
        print(this.computeScore(newArray));
        print(this.computeScore(this.currentArrays));
        this.currentArrays = newArray;
    }
}

let index = new Index;
for (let i = 0; i < 3; i++) {
    index.run();
}

function testLength() {
    let a = [0];
    print(ArkTools.getElementsKind(a));
    a.length = 3;
    print(ArkTools.getElementsKind(a));
}

testLength();

function testDerivedElementskind() {
    class CustomArray extends Array {
        constructor(...args) {
            super(...args);
        }
    }
    let a = new CustomArray(1,2,3,4,5,6,7,8,9,10);
    print(ArkTools.getElementsKind(a));
}

testDerivedElementskind();