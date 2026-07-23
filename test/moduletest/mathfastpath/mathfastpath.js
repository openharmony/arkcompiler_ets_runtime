/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


/*
 * @tc.name:mathfastpath
 * @tc.desc:test Math builtins numeric fast paths preserve baseline semantics
 * @tc.type: FUNC
 * @tc.require:
 */

function show(name, v) {
    var s;
    if (typeof v === "number") {
        if (v === 0) s = (1 / v === -Infinity) ? "-0" : "+0";
        else s = String(v);
    } else s = String(v);
    print(name + " = " + s);
}
var fns = ["abs", "ceil", "floor", "round", "sin", "cos", "sqrt"];
var vals = [0, -0, 1, -1, 0.5, -0.5, 0.4999999999999999, -0.4999999999999999,
    1.5, -1.5, 2.5, -2.5, 0.75, -0.75, NaN, Infinity, -Infinity,
    2147483647, -2147483648, 2147483648, -2147483649,
    9007199254740991, -9007199254740991, 4503599627370495.5,
    1e-323, -1e-323, 1.7976931348623157e308, -1.7976931348623157e308];
for (var f of fns) {
    for (var v of vals) {
        var out = Math[f](v);
        if ((f === "sin" || f === "cos") && typeof out === "number" && isFinite(out) && out !== 0) {
            out = Number(out.toFixed(10));
        }
        show("Math." + f + "(" + (Object.is(v, -0) ? "-0" : v) + ")", out);
    }
}
show("Math.abs('-7')", Math.abs("-7"));
show("Math.ceil('3.2')", Math.ceil("3.2"));
show("Math.floor(null)", Math.floor(null));
show("Math.round(true)", Math.round(true));
show("Math.round(undefined)", Math.round(undefined));
show("Math.sqrt('')", Math.sqrt(""));
show("Math.abs([])", Math.abs([]));
show("Math.abs([-3])", Math.abs([-3]));
var effects = [];
var coercer = { valueOf: function () { effects.push("valueOf"); return -2.5; } };
show("Math.round(coercer)", Math.round(coercer));
print("effects=" + effects.join(","));
try {
    Math.ceil({ valueOf: function () { throw new TypeError("coerce boom"); } });
    print("throwing-coerce: NO THROW");
} catch (e) {
    print("throwing-coerce: " + e.name + " " + e.message);
}
show("Math.abs()", Math.abs());
show("Math.round()", Math.round());
var r1 = Math.random(), r2 = Math.random();
print("random-range: " + (r1 >= 0 && r1 < 1 && r2 >= 0 && r2 < 1));
print("SUITE_END");