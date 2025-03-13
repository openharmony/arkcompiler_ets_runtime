/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function cl_43() {
    cl_43.var_51 = (123456);
    cl_43.var_53 = (928674011);
    cl_43.var_98 = (1688571889);
}

function Test(var_42, var_43, var_44, var_45, var_46) {
    var var_50 = (113);
    // Ensure that the slot ID of the call statement() within the loop is 0xff.
    var var_300 = (--var_42) & (++var_43) & (--var_44);
    var var_301 = (--var_42) & (++var_43) & (--var_44);
    var var_302 = (--var_42) & (++var_43) & (--var_44);
    var var_303 = (--var_42) & (++var_43) & (--var_44);
    var var_304 = (--var_42) & (++var_43) & (--var_44);
    var var_305 = (--var_42) & (++var_43) & (--var_44);
    var var_306 = (--var_42) & (++var_43) & (--var_44);
    var var_307 = (--var_42) & (++var_43) & (--var_44);
    var var_308 = (--var_42) & (++var_43) & (--var_44);
    var var_309 = (--var_42) & (++var_43) & (--var_44);
    var var_310 = (--var_42) & (++var_43) & (--var_44);
    var var_311 = (--var_42) & (++var_43) & (--var_44);
    var var_312 = (--var_42) & (++var_43) & (--var_44);
    var var_313 = (--var_42) & (++var_43) & (--var_44);
    var var_314 = (--var_42) & (++var_43) & (--var_44);
    var var_315 = (--var_42) & (++var_43) & (--var_44);
    var var_316 = (--var_42) & (++var_43) & (--var_44);
    var var_317 = (--var_42) & (++var_43) & (--var_44);
    var var_318 = (--var_42) & (++var_43) & (--var_44);
    var var_319 = (--var_42) & (++var_43) & (--var_44);
    var var_320 = (--var_42) & (++var_43) & (--var_44);
    var var_321 = (--var_42) & (++var_43) & (--var_44);
    var var_322 = (--var_42) & (++var_43) & (--var_44);
    var var_323 = (--var_42) & (++var_43) & (--var_44);
    var var_324 = (--var_42) & (++var_43) & (--var_44);
    var var_325 = (--var_42) & (++var_43) & (--var_44);
    var var_326 = (--var_42) & (++var_43) & (--var_44);
    var var_327 = (--var_42) & (++var_43) & (--var_44);
    var var_328 = (--var_42) & (++var_43) & (--var_44);
    var var_329 = (--var_42) & (++var_43) & (--var_44);
    var var_330 = (--var_42) & (++var_43) & (--var_44);
    var var_331 = (--var_42) & (++var_43) & (--var_44);
    var var_332 = (--var_42) & (++var_43) & (--var_44);
    var var_333 = (--var_42) & (++var_43) & (--var_44);
    var var_334 = (--var_42) & (++var_43) & (--var_44);
    var var_335 = (--var_42) & (++var_43) & (--var_44);
    var var_336 = (--var_42) & (++var_43) & (--var_44);
    var var_337 = (--var_42) & (++var_43) & (--var_44);
    var var_338 = (--var_42) & (++var_43) & (--var_44);
    var var_339 = (--var_42) & (++var_43) & (--var_44);
    var var_340 = (--var_42) & (++var_43) & (--var_44);
    var var_341 = (--var_42) & (++var_43) & (--var_44);

    var var_64 = [[[(1075652066), (1572543454), (-1373653984), (-677435825), (-1571332717)]],
                  [[(1152426036), (-1272234450), (508469531), (-780112294), (1943940739)]],
                  [[(-187395671), (-43356962), (-163465418), (-1296113430), (-435365389)]],
                  [[(1404215208), (152774491), (100927236), (-1467561855), (-19283496)]],
                  [[(-1287547821), (-970867494), (-1232465612), (-193230996), (-1378255740)]],
                  [[(-609768854), (-348123470), (20316771), (875758301), (-1919674097)]]];
    var var_48 = [[(-1957622862), (1324461290)]];
    print("hello exception 300!");
    if (Number(var_48[((var_50++) & 0)][(((-(++var_42)) | (Number(101))) & 3)])) {
        try {
            cl_43.var_51.var_52 = (var_50--);
            for (cl_43.var_53 = 968; (cl_43.var_53 > 944); cl_43.var_53 = (cl_43.var_53 - 6)) {
                if ((--var_50) > cl_43.var_53) {}
                if (!((var_50--) < cl_43.var_53)) {}
                for (cl_43.var_98 = 337; (cl_43.var_98 < 351); cl_43.var_98 = (cl_43.var_98)) {
                    print("callarg1's slot id is 0xff");
                }
            }
        } catch (e) {
            print("hello exception 30!");
        }
    } else {
        print("hello exception 301!");
    }
    return (-47);
}

new cl_43();

Test((122), (10), (-3));

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test((122), (10), (-3));
