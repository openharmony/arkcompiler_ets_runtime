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
function test() {
	function f0() {
		const o18 = {
			proto: "p",
			n(a5) {
				let v6;
				try { v6 = a5(this); } catch (e) { }
				function f7() {
					return v6;
				}
				return this;
			},
			set a(a9) {
			},
		};
		return o18;
	}
	const v19 = f0();
	const v21 = f0();
	function f22(a23) {
		try { a23.n(f22); } catch (e) { }
		const o30 = {
			m() {
				return a23;
			},
			proto: a23,
			"d": f0,
		};
		return o30;
	}
	f22(v21);
	const v33 = f22(v19);
	const v45 = new Set();
	for (let [i71, i72] = (() => {
		return [0, 5];
	})();
		i71 < i72;
	) {
		i72--;
	}
}

test()
print(ArkTools.isAOTCompiled(test));