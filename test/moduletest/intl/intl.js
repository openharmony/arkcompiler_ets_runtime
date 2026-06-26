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

/*
 * @tc.name:intl
 * @tc.desc:test Internationalization API
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
try {
    const proto = this.Intl.NumberFormat.prototype;
    proto.resolvedOptions();
} catch (err) {
    print(err instanceof TypeError);
}

try {
    const proto = this.Intl.DateTimeFormat.prototype;
    proto.resolvedOptions();
} catch (err) {
    print(err instanceof TypeError);
}

{
    let noThrow = true;
    try {
        new Intl.DateTimeFormat("en", { numberingSystem: "invalid" });
    } catch (e) {
        noThrow = false;
    }
    print(noThrow);
}

{
    let constructors = [
        {c: Intl.DateTimeFormat, f: "format"},
        {c: Intl.NumberFormat, f: "format"},
    ];

    for (let {c, f} of constructors) {
        let o = Object.create(c.prototype);
        print(o instanceof c);
        print(o == c.call(o));
        print(o[f] == o[f]);
        print(o instanceof c);
    }
}

{
    try {
        const proto = this.Intl.Collator.prototype;
        proto.resolvedOptions();
    } catch (err) {
        print(err instanceof TypeError);
    }
}

{
    const nf = new Intl.NumberFormat([], {
        style: "unit",
        unit: "microsecond",
    });

    const nf2 = new Intl.NumberFormat([], {
        style: "unit",
        unit: "nanosecond",
    });
    print("new Intl.NumberFormat success");
}

{
    let holder = Object.create(Intl.NumberFormat.prototype);
    Intl.NumberFormat.call(holder);
    let hiddenSym = Object.getOwnPropertySymbols(holder)[0];
    let fake = {};
    let wrapper = Object.create(Intl.NumberFormat.prototype);
    wrapper[hiddenSym] = fake;
    try {
        wrapper.format;
    } catch (e) {
        print(e instanceof TypeError);
    }

    wrapper = Object.create(Intl.NumberFormat.prototype);
    wrapper[hiddenSym] = fake;
    try {
        Intl.NumberFormat.prototype.formatToParts.call(wrapper, 1);
    } catch (e) {
        print(e instanceof TypeError);
    }

    wrapper = Object.create(Intl.DateTimeFormat.prototype);
    wrapper[hiddenSym] = fake;
    try {
        wrapper.format;
    } catch (e) {
        print(e instanceof TypeError);
    }

    try {
        Intl.DateTimeFormat.prototype.formatToParts.call(wrapper);
    } catch (e) {
        print(e instanceof TypeError);
    }

    wrapper = Object.create(Intl.RelativeTimeFormat.prototype);
    wrapper[hiddenSym] = fake;
    try {
        Intl.RelativeTimeFormat.prototype.format.call(wrapper, 1, "day");
    } catch (e) {
        print(e instanceof TypeError);
    }
}
