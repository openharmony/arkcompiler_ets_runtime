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

class cl_59 {
    static var_116: number[] = [(-700)];
    static var_159: number = 9;

    static func_42(var_47: number): any {
        let var_162 = -115;
        let var_91: number[] = [(-764)];
        let var_158: any[][][] = [[[(6), (28), (16), (110), (-80)]], [[(-62), (88), (6), (-98), (-82)], [(-96), (71), (13), (117), (38)]], [[(-11), (-18), (-46), (96), (40)], [(-11), (84), (90), (-46), (62)]], [[(52), (-87), (-128), (117), (-101)], [(116), (-84), (5), (91), (-12)]], [[(-81), (114), (5), (91), (-108)], [(-101), (124), (62), (20), (-88)]], [[(-82), (-45), (76), (89), (23)], [(-116), (-77), (21), (-114), (-94)]]];
        if (!(cl_59.var_159 < 75)) {
            try {
                let var_103 = 119;
                try {
                    for (let var_111 = 681; var_111 < 757; var_111 += 19) {
                        var_91[0] = var_47++;
                    }
                } catch (e) {
                    try {
                        for (let var_113 = 116; var_113 > 108; var_113 -= 4) {
                            var_91[0] = var_103++;
                        }
                    } catch (e) {
                        console.log("hello exception 56 !");
                    }
                    console.log("hello exception 55 !");
                }
            } catch (e) {
                if (cl_59.var_116[var_47++]) {
                    // 空块体  
                }
                console.log("hello exception 57 !");
            }
        }

        return var_158[((-var_162--) & 6)][((cl_59.var_159--) & 4)];
    }
}

class MainClass {
    var_311: cl_59 = new cl_59();

    run(): void {
        for (let var_30 = 316; var_30 > 264; var_30 -= 13) {
            cl_59.func_42(~(var_30++));
        }
    }
}

class HtmlTest {
    mainclass: MainClass = new MainClass();
}

function RunTest() {
    const html1 = new HtmlTest();
    html1.mainclass.run();
}

RunTest();
print(ArkTools.isAOTCompiled(RunTest));