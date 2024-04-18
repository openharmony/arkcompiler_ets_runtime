/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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


// This test need set ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC to 1
// update build.gn to enable elements_kind_generic test, and disable elements_kind test


let GENERIC = 31;



function test1() {
    let a = [1, 2, 34];
    if (a[0] == 1 && a[1] == 2 && a[2] == 34 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test1 - success");
    } else {
        print("test1 - failed");
    }
}



function test2() {
    let a = [1, 2, 34];
    a[1] = 10;
    if (a[0] == 1 && a[1] == 10 && a[2] == 34 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test2 - success");
    } else {
        print("test2 - failed");
    }
}




function test3() {
    let a = [1, 2, 34];
    print(a[10]); 
    if (a[0] == 1 && a[1] == 2 && a[2] == 34 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test3 - success");
    } else {
        print("test3 - failed");
    }
}




function test4() {
    let a = [1, 2, 34];
    print(a[10]); 
    a[1] = 10;
    if (a[0] == 1 && a[1] == 10 && a[2] == 34 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test4 - success");
    } else {
        print("test4 - failed");
    }
}



function test5() {
    let a = [1.5, 3.5, 6.7];
    if (a[0] == 1.5 && a[1] == 3.5 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test5 - success");
    } else {
        print("test5 - failed");
    }
}



function test6() {
    let a = [1.5, 3.5, 6.7];
    a[1] = 8.8;
    if (a[0] == 1.5 && a[1] == 8.8 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test6 - success");
    } else {
        print("test6 - failed");
    }
}




function test7() {
    let a = [1.5, 3.5, 6.7];
    print(a[10]); 
    if (a[0] == 1.5 && a[1] == 3.5 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test7 - success");
    } else {
        print("test7 - failed");
    }
}




function test8() {
    let a = [1.5, 3.5, 6.7];
    print(a[10]); 
    a[1] = 8.8;
    if (a[0] == 1.5 && a[1] == 8.8 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test8 - success");
    } else {
        print("test8 - failed");
    }
}



function test9() {
    let a = [1.5, 3, 6.7];
    if (a[0] == 1.5 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test9 - success");
    } else {
        print("test9 - failed");
    }
}



function test10() {
    let a = [1.5, 3, 6.7];
    a[0] = 2;
    if (a[0] == 2 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test10 - success");
    } else {
        print("test10 - failed");
    }
}




function test11() {
    let a = [1.5, 3, 6.7];
    print(a[10]);
    if (a[0] == 1.5 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test11 - success");
    } else {
        print("test11 - failed");
    }
}




function test12() {
    let a = [1.5, 3, 6.7];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == 3 && a[2] == 6.7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test12 - success");
    } else {
        print("test12 - failed");
    }
}



function test13() {
    let a = ["o", "p", "q"];
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test13 - success");
    } else {
        print("test13 - failed");
    }
}



function test14() {
    let a = ["o", "p", "q"];
    a[0] = "x";
    if (a[0] == "x" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test14 - success");
    } else {
        print("test14 - failed");
    }
}




function test15() {
    let a = ["o", "p", "q"];
    print(a[10]);
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test15 - success");
    } else {
        print("test15 - failed");
    }
}




function test16() {
    let a = ["o", "p", "q"];
    print(a[10]);
    a[0] = "x";
    if (a[0] == "x" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test16 - success");
    } else {
        print("test16 - failed");
    }
}



function test17() {
    let a = [1, , 5];
    if (a[0] == 1 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test17 - success");
    } else {
        print("test17 - failed");
    }
}



function test18() {
    let a = [1.5, , 5.6];
    if (a[0] == 1.5 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test18 - success");
    } else {
        print("test18 - failed");
    }
}




function test19() {
    let a = [1, , 5];
    print(a[10]);
    if (a[0] == 1 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test19 - success");
    } else {
        print("test19 - failed");
    }
}




function test20() {
    let a = [1.5, , 5.6];
    print(a[10]);
    if (a[0] == 1.5 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test20 - success");
    } else {
        print("test20 - failed");
    }
}



function test21() {
    let a = [1, , 5];
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test21 - success");
    } else {
        print("test21 - failed");
    }
}



function test22() {
    let a = [1.5, , 5.6];
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test22 - success");
    } else {
        print("test22 - failed");
    }
}




function test23() {
    let a = [1, , 5];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test23 - success");
    } else {
        print("test23 - failed");
    }
}




function test24() {
    let a = [1.5, , 5.6];
    print(a[10]);
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test24 - success");
    } else {
        print("test24 - failed");
    }
}



function test25() {
    let a = [1, , 5];
    if (a[1] != undefined) {
        print("test25 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    if (a[1] != 2) {
        print("test25 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test25 - success");
    } else {
        print("test25 - failed");
    }
}



function test26() {
    let a = [1.5, , 5.6];
    if (a[1] != undefined) {
        print("test26 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2.2;
    if (a[1] != 2.2) {
        print("test26 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1.5 && a[1] == 2.2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test26 - success");
    } else {
        print("test26 - failed");
    }
}




function test27() {
    let a = [1, , 5];
    if (a[1] != undefined) {
        print("test27 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    print(a[10]);
    if (a[1] != 2) {
        print("test27 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test27 - success");
    } else {
        print("test27 - failed");
    }
}




function test28() {
    let a = [1.5, , 5.6];
    if (a[1] != undefined) {
        print("test28 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2.2;
    print(a[10]);
    if (a[1] != 2.2) {
        print("test28 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1.5 && a[1] == 2.2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test28 - success");
    } else {
        print("test28 - failed");
    }
}



function test29() {
    let a = [1, , 5.1];
    if (a[0] == 1 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test29 - success");
    } else {
        print("test29 - failed");
    }
}



function test30() {
    let a = ["o", , "q"];
    if (a[0] == "o" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test30 - success");
    } else {
        print("test30 - failed");
    }
}




function test31() {
    let a = [1, , 5.1];
    print(a[10]);
    if (a[0] == 1 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test31 - success");
    } else {
        print("test31 - failed");
    }
}




function test32() {
    let a = ["o", , "q"];
    print(a[10]);
    if (a[0] == "o" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test32 - success");
    } else {
        print("test32 - failed");
    }
}



function test33() {
    let a = [1, , 5.1];
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test33 - success");
    } else {
        print("test33 - failed");
    }
}



function test34() {
    let a = ["o", , "q"];
    a[0] = "x";
    if (a[0] == "x" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test34 - success");
    } else {
        print("test34 - failed");
    }
}




function test35() {
    let a = [1, , 5.1];
    print(a[10]);
    a[0] = 2;
    if (a[0] == 2 && a[1] == undefined && a[2] == 5.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test35 - success");
    } else {
        print("test35 - failed");
    }
}




function test36() {
    let a = ["o", , "q"];
    print(a[10]);
    a[0] = "x";
    if (a[0] == "x" && a[1] == undefined && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test36 - success");
    } else {
        print("test36 - failed");
    }
}



function test37() {
    let a = [1, , 5.6];
    if (a[1] != undefined) {
        print("test37 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    if (a[1] != 2) {
        print("test37 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5.6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test37 - success");
    } else {
        print("test37 - failed");
    }
}



function test38() {
    let a = ["o", , "q"];
    if (a[1] != undefined) {
        print("test38 - failed: a[1] should be undefined");
        return;
    }
    a[1] = "p";
    if (a[1] != "p") {
        print("test38 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test38 - success");
    } else {
        print("test38 - failed");
    }
}




function test39() {
    let a = [1, , 5.1];
    if (a[1] != undefined) {
        print("test39 - failed: a[1] should be undefined");
        return;
    }
    a[1] = 2;
    print(a[10]);
    if (a[1] != 2) {
        print("test39 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == 1 && a[1] == 2 && a[2] == 5.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test39 - success");
    } else {
        print("test39 - failed");
    }
}




function test40() {
    let a = ["o", , "q"];
    if (a[1] != undefined) {
        print("test40 - failed: a[1] should be undefined");
        return;
    }
    a[1] = "p";
    print(a[10]);
    if (a[1] != "p") {
        print("test40 - failed: store value to GENERIC spot failed");
    }
    if (a[0] == "o" && a[1] == "p" && a[2] == "q" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test40 - success");
    } else {
        print("test40 - failed");
    }
}



function test41() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test41 - failed");
    }
    a[1] = 2.5;
    if (a[0] == 1 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test41 - success");
    } else {
        print("test41 - failed");
    }
}


function test42() {
    let a = [1, 2, 3];
    a[1] = 2.5;
    if (a[0] == 1 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test42 - success");
    } else {
        print("test42 - failed");
    }
}



function test43() {
    let a = [1, 2, 3];
    a[0] = 4;
    a[1] = 2.5;
    if (a[0] == 4 && a[1] == 2.5 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test43 - success");
    } else {
        print("test43 - failed");
    }
}



function test44() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test44 - failed");
    }
    a[1] = "1";
    if (a[0] == 1 && a[1] == "1" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test44 - success");
    } else {
        print("test44 - failed");
    }
}


function test45() {
    let a = [1, 2.2, 3];
    if (a[0] != 1 || a[1] != 2.2 || a[2] != 3) {
        print("test45 - failed");
    }
    a[1] = 2;
    if (a[0] == 1 && a[1] == 2 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test45 - success");
    } else {
        print("test45 - failed");
    }
}


function test46() {
    let a = [1, 2.2, 3];
    if (a[0] != 1 || a[1] != 2.2 || a[2] != 3) {
        print("test46 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test46 - success");
    } else {
        print("test46 - failed");
    }
}


function test47() {
    let a = [1, 2, 3];
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) {
        print("test47 - failed");
    }
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test47 - success");
    } else {
        print("test47 - failed");
    }
}




function test48() {
    let a = [1, "2"];
    if (a[0] != 1 || a[1] != "2") {
        print("test48 - failed");
    }
    a[1] = 2;
    if (a[0] == 1 && a[1] == 2 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test48 - success");
    } else {
        print("test48 - failed");
    }
}


function test49() {
    let a = [1, "2"];
    if (a[0] != 1 || a[1] != "2") {
        print("test49 - failed");
    }
    a[0] = "1";
    if (a[0] == "1" && a[1] == "2" && ArkTools.getElementsKind(a) == GENERIC) {
        print("test49 - success");
    } else {
        print("test49 - failed");
    }
}



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



function test51() {
    let a = [1, , 3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test51 - success");
    } else {
        print("test51 - failed");
    }
}



function test52() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test52 - failed");
    }
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test52 - success");
    } else {
        print("test52 - failed");
    }
}



function test53() {
    let a = [1, , 3] 
    a[0] = 2.2;
    if (a[0] == 2.2 && a[1] == undefined && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test53 - success");
    } else {
        print("test53 - failed");
    }
}



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



function test55() {
    let a = [1.1, , 3.3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test55 - success");
    } else {
        print("test55 - failed");
    }
}



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



function test57() {
    let a = [1, , 3.3] 
    a[0] = "1";
    if (a[0] == "1" && a[1] == undefined && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test57 - success");
    } else {
        print("test57 - failed");
    }
}




function test58() {
    let a = [1, , 3] 
    if (a[0] != 1 || a[1] != undefined || a[2] != 3) {
        print("test58 - failed");
    }
    a[1] = 2.2;
    if (a[0] == 1 && a[1] == 2.2 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test58 - success");
    } else {
        print("test58 - failed");
    }
}




function test59() {
    let a = [1, , 3] 
    a[1] = 2.2;
    if (a[0] == 1 && a[1] == 2.2 && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test59 - success");
    } else {
        print("test59 - failed");
    }
}




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




function test61() {
    let a = [1, , 3] 
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test61 - success");
    } else {
        print("test61 - failed");
    }
}




function test62() {
    let a = [1.1, , 3.3] 
    if (a[0] != 1.1 || a[1] != undefined || a[2] != 3.3) {
        print("test62 - failed");
    }
    a[1] = 2;
    if (a[0] == 1.1 && a[1] == 2 && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test62 - success");
    } else {
        print("test62 - failed");
    }
}




function test63() {
    let a = [1.1, , 3.3] 
    a[1] = 2;
    if (a[0] == 1.1 && a[1] == 2 && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test63 - success");
    } else {
        print("test63 - failed");
    }
}




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




function test65() {
    let a = [1.1, , 3.3] 
    a[1] = "2";
    if (a[0] == 1.1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test65 - success");
    } else {
        print("test65 - failed");
    }
}




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




function test67() {
    let a = [1, , 3.3] 
    a[1] = "2";
    if (a[0] == 1 && a[1] == "2" && a[2] == 3.3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test67 - success");
    } else {
        print("test67 - failed");
    }
}




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


function test84() {
    let a = new Array(5);
    if (ArkTools.getElementsKind(a) == GENERIC) {
        print("test84 - success");
    } else {
        print("test84 - failed");
    }
}

function test85() {
    let a = new Array(1, 2, 3);
    if (ArkTools.getElementsKind(a) == GENERIC) {
        print("test85 - success");
    } else {
        print("test85 - failed");
    }
}

function test86() {
    let a = new Array(1, 1.5, 3);
    if (ArkTools.getElementsKind(a) == GENERIC) {
        print("test86 - success");
    } else {
        print("test86 - failed");
    }
}

function test87() {
    let a = new Array("1", 1.5, 3);
    if (ArkTools.getElementsKind(a) == GENERIC) {
        print("test87 - success");
    } else {
        print("test87 - failed");
    }
}

function test88() {
    let a = new Array("1", "2", "3");
    if (ArkTools.getElementsKind(a) == GENERIC) {
        print("test88 - success");
    } else {
        print("test88 - failed");
    }
}

function test89() {
    let a = Array.of(1, 2, 3);
    if (a.length == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test89 - success");
    } else {
        print("test89 - failed");
    }
}

function test90() {
    let a = Array.of(3);
    if (a.length == 1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test90 - success");
    } else {
        print("test90 - failed");
    }
}



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


function test92() {
    const num1 = [1, 2, 3];
    const num2 = [4, 5, 6];

    const a = num1.concat(num2);
    if (a.length == 6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test92 - success");
    } else {
        print("test92 - failed");
    }
}

function test93() {
    const num1 = [1, 2, 3];
    const num2 = [4.4, 5.5, 6.6];

    const a = num1.concat(num2);
    if (a.length == 6 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test93 - success");
    } else {
        print("test93 - failed");
    }
}

function test94() {
    let a = [1, , 3].concat([4, 5]); 
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test94 - success");
    } else {
        print("test94 - failed");
    }
}

function test95() {
    let a = [1, , 3].concat([4, 5]); 
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test95 - success");
    } else {
        print("test95 - failed");
    }
}

function test96() {
    const obj1 = { 0: 1, 1: 2, 2: 3, length: 3 };
    const obj2 = { 0: 1, 1: 2, 2: 3, length: 3, [Symbol.isConcatSpreadable]: true };
    let a = [0].concat(obj1, obj2);
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test96 - success");
    } else {
        print("test96 - failed");
    }
}


function test97() {
    let a = [1, 2, 3, 4, 5].copyWithin(0, 3); 
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test97 - success");
    } else {
        print("test97 - failed");
    }
}

function test98() {
    let a = [1, , 3].copyWithin(2, 1, 2); 
    if (a.length == 3 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test98 - success");
    } else {
        print("test98 - failed");
    }
}


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


function test100() {
    const isBelowThreshold = (currentValue) => currentValue < 40;
    const array1 = [1, 30, 39, 29, 10, 13];
    
    if (array1.every(isBelowThreshold)) {
        print("test100 - success");
    } else {
        print("test100 - failed");
    }
}


function test101() {
    const array1 = [1, 2, 3, 4];
    
    array1.fill(0, 2, 4);
    
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == GENERIC){
        print("test101 - success");
    } else {
        print("test101 - failed");
    }
}

function test102() {
    const array1 = [1, 2, 3, 4];
    
    array1.fill(5.5, 1);
    
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == GENERIC){
        print("test102 - success");
    } else {
        print("test102 - failed");
    }
}

function test103() {
    const array1 = [1, 2, 3, 4];
    array1.fill("6");
    
    if (array1.length == 4 && ArkTools.getElementsKind(array1) == GENERIC){
        print("test103 - success");
    } else {
        print("test103 - failed");
    }
}


function test104() {
    const words = ['spray', 'elite', 'exuberant', 'destruction', 'present'];
    const result = words.filter((word) => word.length > 6);
    
    if (result.length == 3 && ArkTools.getElementsKind(result) == GENERIC){
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
      3: "a", 
    };
    let result = Array.prototype.filter.call(arrayLike, (x) => x <= "b");
    
    if (result.length == 2 && ArkTools.getElementsKind(result) == GENERIC){
        print("test105 - success");
    } else {
        print("test105 - failed");
    }
}

function test106() {
    let result = [1, , undefined].filter((x) => x === undefined); 
    if (result.length == 1 && ArkTools.getElementsKind(result) == GENERIC){
        print("test106 - success");
    } else {
        print("test106 - failed");
    }
}


function test107() {
    const array1 = [5, 12, 8, 130, 44];
    const found = array1.find((element) => element > 10);
    
    if (found == 12) {
        print("test107 - success");
    } else {
        print("test107 - failed");
    }
}


function test108() {
    const array1 = [5, 12, 8, 130, 44];
    const isLargeNumber = (element) => element > 13;
    let result = array1.findIndex(isLargeNumber);
    
    if (result == 3) {
        print("test108 - success");
    } else {
        print("test108 - failed");
    }
}


function test109() {
    const array1 = [5, 12, 50, 130, 44];
    const found = array1.findLast((element) => element > 45);
    
    if (found == 130) {
        print("test109 - success");
    } else {
        print("test109 - failed");
    }
}


function test110() {
    const array1 = [5, 12, 50, 130, 44];
    const isLargeNumber = (element) => element > 45;
    let result = array1.findLastIndex(isLargeNumber);
    
    
    if (result == 3) {
        print("test110 - success");
    } else {
        print("test110 - failed");
    }
}


function test111() {
    const arr1 = [0, 1, 2, [3, 4]];
    let result = arr1.flat();
    

    if (result.length == 5 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test111 - success");
    } else {
        print("test111 - failed");
    }
}

function test112() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat();
    

    if (result.length == 4 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test112 - success");
    } else {
        print("test112 - failed");
    }
}

function test113() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat(2);
    

    if (result.length == 5 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test113 - success");
    } else {
        print("test113 - failed");
    }
}

function test114() {
    const arr2 = [0, 1, [2, [3, [4, 5]]]];
    let result = arr2.flat(Infinity);
    

    if (result.length == 6 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test114 - success");
    } else {
        print("test114 - failed");
    }
}


function test115() {
    const arr1 = [1, 2, 1];
    const result = arr1.flatMap((num) => (num === 2 ? [2, 2] : 1));
    
    if (result.length == 4 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test115 - success");
    } else {
        print("test115 - failed");
    }
}

function test116() {
    const arr1 = [1, 2, 1];
    const result = arr1.flatMap((num) => (num === 2 ? 2.2 : 1));
    
    if (result.length == 3 && ArkTools.getElementsKind(result) == GENERIC) {
        print("test116 - success");
    } else {
        print("test116 - failed");
    }
}


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


function test118() {
    const array1 = [1, 2, 3];
    let result = true;
    
    if (!array1.includes(2) && ArkTools.getElementsKind(array1) == GENERIC) {
        result = false;
    }

    const pets = ['cat', 'dog', 'bat'];
    
    if (!pets.includes('cat')) {
        result = false;
    }
    
    if (pets.includes('at')) {
        result = false;
    }

    if (result) {
        print("test118 - success");
    } else {
        print("test118 - failed");
    }
}


function test119() {
    const beasts = ['ant', 'bison', 'camel', 'duck', 'bison'];
    let result = true;
    let index1 = beasts.indexOf('bison');
    
    if (index1 != 1) {
        result = false;
    }

    
    let index2 = beasts.indexOf('bison', 2);
    
    if (index2 != 4) {
        result = false;
    }

    let index3 = beasts.indexOf('giraffe');
    
    if (index3 != -1) {
        result = false;
    }
    if (result) {
        print("test119 - success");
    } else {
        print("test119 - failed");
    }
}


function test120() {
    const animals = ['Dodo', 'Tiger', 'Penguin', 'Dodo'];
    let result = true;
    let index1 = animals.lastIndexOf('Dodo');
    
    if (index1 != 3) {
        result = false;
    }

    let index2 = animals.lastIndexOf('Tiger');
    
    if (index2 != 1) {
        result = false;
    }
    if (result) {
        print("test120 - success");
    } else {
        print("test120 - failed");
    }
}


function test121() {
    const elements = ['Fire', 'Air', 'Water'];
    let result = true;
    let joined1 = elements.join();
    
    if (joined1 != "Fire,Air,Water") {
        result = false;
    }

    let joined2 = elements.join('');
    
    if (joined2 != "FireAirWater") {
        result = false;
    }

    let joined3 = elements.join('-');
    
    if (joined3 != "Fire-Air-Water") {
        result = false;
    }
    if (result) {
        print("test121 - success");
    } else {
        print("test121 - failed");
    }
}


function test122() {
    const array1 = [1, 4, 9, 16];

    
    const map1 = array1.map((x) => x * 2);
    

    if (map1.length == 4 && ArkTools.getElementsKind(map1) == GENERIC) {
        print("test122 - success");
    } else {
        print("test122 - failed");
    }
}

function test123() {
    const array1 = [1, 4, 9, 16];

    
    const map1 = array1.map((x) => x * 1.1);
    

    if (map1.length == 4 && ArkTools.getElementsKind(map1) == GENERIC) {
        print("test123 - success");
    } else {
        print("test123 - failed");
    }
}


function test124() {
    const plants = ['broccoli', 'cauliflower', 'cabbage', 'kale', 'tomato'];

    let popResult = plants.pop();
    
    if (popResult == "tomato" && ArkTools.getElementsKind(plants) == GENERIC) {
        print("test124 - success");
    } else {
        print("test124 - failed");
    }
}

function test125() {
    const a = [1, 2, 3, 4, 5.5];
    let popResult = a.pop();
    
    if (popResult == 5.5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test125 - success");
    } else {
        print("test125 - failed");
    }
}


function test126() {
    let a = [6, 5, 4, 3, 2, 1];
    let b = a.toSorted();
    if (b.length == 6 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test126 - success");
    } else {
        print("test126 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}


function test127() {
    let a = [0, 1, 2];
    a.unshift(-1);
    if (a.length == 4 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test127 - success");
    } else {
        print("test127 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(a));
    }
}

function test128() {
    let a = [0, 1, 2];
    a.unshift(-2, -1);
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test128 - success");
    } else {
        print("test128 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(a));
    }
}

function test129() {
    
    let a = [0, 1, 2, 3, 4, 5, 6];
    let b = a.toSpliced(2, 2, 3, 3);
    if (b.length == 7 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test129 - success");
    } else {
        print("test129 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test130() {
    
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.toSpliced(2, 1, 3);
    if (b.length == 7 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test130 - success");
    } else {
        print("test130 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test131() {
    
    let a = [0, 1, 2, 3, 4, 5, 6];
    let b = a.toSpliced(2, 1, 3.1);
    if (b.length == 7 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test131 - success");
    } else {
        print("test131 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test132() {
    
    let a = [0, 1, 2, 4, 5, 6];
    let b = a.toReversed();
    if (b.length == 6 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test132 - success");
    } else {
        print("test132 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test133() {
    
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.toReversed();
    if (b.length == 7 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test133 - success");
    } else {
        print("test133 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test134() {
    
    let a = [0, 1, 2, ,4, 5, 6];
    if (a.length == 7 && a.at(2) == 2 && a.at(3) == undefined) {
        print("test134 - success");
    } else {
        print("test134 - failed, a.At(2): " + a[2] + " , but get: a.At(3): " + a[3]);
    }
}

function test135() {
    
    let a = [0, 1, 2, ,4, 5, 6];
    let b = a.with(3, 6);
    if (b.length == 7 && b[3] == 6 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test135 - success");
    } else {
        print("test135 - failed, expected: " + GENERIC + " , but get: " + ArkTools.getElementsKind(b));
    }
}

function test136() {
    let a = [1, 2, 3, 4, 5];
    a.push(5.5);
    if (a.length == 6 && a[5] == 5.5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test136 - success");
    } else {
        print("test136 - failed");
    }
}

function test137() {
    let a = [1, 2, 3, 4, 5];
    a.push(undefined);
    if (a.length == 6 && a[5] == undefined && ArkTools.getElementsKind(a) == GENERIC) {
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
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test140 - success");
    } else {
        print("test140 - failed");
    }
}

function test141() {
    let a = [1, 2, , 4, 5];
    a.reverse();
    if (a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test141 - success");
    } else {
        print("test141 - failed");
    }
}

function test142() {
    
    let a = [0.1, 1, 2, 3, 4, 5, 6];
    let result = a.shift();
    if (a.length == 6 && result == 0.1 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test142 - success");
    } else {
        print("test142 - failed");
    }
}

function test143() {
    
    let a = [0, 1, 2, , 4, 5, 6];
    let result = a.shift();
    if (a.length == 6 && result == 0 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test143 - success");
    } else {
        print("test143 - failed");
    }
}

function test144() {
    
    let a = [0, 1, 2, , 4, 5, 6];
    let b = a.slice();
    if (b.length == 7 && ArkTools.getElementsKind(b) == GENERIC) {
        print("test144 - success");
    } else {
        print("test144 - failed");
    }
}

function test145() {
    let a = [1, , 4, 21, 100000];
    a.sort();
    if (a[0] == 1 && a[1] == 100000 && a.length == 5 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test145 - success");
    } else {
        print("test145 - failed");
    }
}

function test146() {
    
    let a = [0, 1, 2, 3, 4, 5, 6];
    a.splice(2, 2, 3, 3);
    if (a.length == 7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test146 - success");
    } else {
        print("test146 - failed");
    }
}

function test147() {
    
    let a = [0, 1, 2, , 4, 5, 6];
    a.splice(2, 1, 3);
    if (a.length == 7 && ArkTools.getElementsKind(a) == GENERIC) {
        print("test147 - success");
    } else {
        print("test147 - failed");
    }
}


test1();
test2();
test3();
test4();


test5();
test6();
test7();
test8();


test9();
test10();
test11();
test12();


test13();
test14();
test15();
test16();



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


test41();
test42();
test43();
test44();
test45();
test46();
test47();
test48();
test49();


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



test84();
test85();
test86();
test87();
test88();


test89();
test90();
test91();


test92();
test93();
test94();
test95();
test96();


test97();
test98();


test99();


test100();


test101();
test102();
test103();


test104();
test105();
test106();





test107();
test108();
test109();
test110();


test111();
test112();
test113();
test114();


test115();
test116();


test117();


test118();


test119();


test120();


test121();


test122();
test123();


test124();
test125();


test126();


test127();
test128();


test129();
test130();
test131();


test132();
test133();


test134();


test135();


test136();
test137();



test138();
test139();


test140();
test141();


test142();
test143();


test144(); 


test145();


test146();
test147();


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
