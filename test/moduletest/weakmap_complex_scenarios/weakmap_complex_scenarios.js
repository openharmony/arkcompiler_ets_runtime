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

/**
 * @tc.name:weakmap_complex_scenarios
 * @tc.desc:test weakmap function with complex scenarios including cross references
 * @tc.type: FUNC
 */

print("=== All tests started ===");

// Test 1: Single WeakMap with V referencing K
// Expected: K and V should be collected
Promise.resolve().then(() => print("=== Test 1: Single WeakMap, V references K ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f1() {
        let k1 = {};
        let v1 = {};
        v1.ref = k1;  // V references K
        registry.register(k1, "test1_k1_collected");
        registry.register(v1, "test1_v1_collected");
        wm.set(k1, v1);
    }
    f1();
    ArkTools.forceFullGC();
}

// Test 2: Single WeakMap with multiple K-V pairs forming a cycle
// Expected: All K and V should be collected
Promise.resolve().then(() => print("=== Test 2: Single WeakMap, cyclic references V1->K2, V2->K3, V3->K1 ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f2() {
        let k1 = {};
        let k2 = {};
        let k3 = {};
        let v1 = {};
        let v2 = {};
        let v3 = {};

        // V1 references K2, V2 references K3, V3 references K1
        v1.ref = k2;
        v2.ref = k3;
        v3.ref = k1;

        registry.register(k1, "test2_k1_collected");
        registry.register(k2, "test2_k2_collected");
        registry.register(k3, "test2_k3_collected");
        registry.register(v1, "test2_v1_collected");
        registry.register(v2, "test2_v2_collected");
        registry.register(v3, "test2_v3_collected");

        wm.set(k1, v1);
        wm.set(k2, v2);
        wm.set(k3, v3);
    }
    f2();
    ArkTools.forceFullGC();
}

// Test 3: Two WeakMaps with cross references
// Map1: {K1: V1}, V1 references K2
// Map2: {K2: V2}, V2 references K1
// Expected: All K and V should be collected
Promise.resolve().then(() => print("=== Test 3: Two WeakMaps, cross-map cycle ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f3() {
        let k1 = {};
        let k2 = {};
        let v1 = {};
        let v2 = {};

        // V1 references K2 (in different map)
        v1.ref = k2;
        // V2 references K1 (in different map)
        v2.ref = k1;

        registry.register(k1, "test3_k1_collected");
        registry.register(k2, "test3_k2_collected");
        registry.register(v1, "test3_v1_collected");
        registry.register(v2, "test3_v2_collected");

        wm1.set(k1, v1);
        wm2.set(k2, v2);
    }
    f3();
    ArkTools.forceFullGC();
}

// Test 4: Three WeakMaps with complex cross references
// Map1: {K1: V1}, V1->K2
// Map2: {K2: V2}, V2->K3
// Map3: {K3: V3}, V3->K1
// Expected: All K and V should be collected
Promise.resolve().then(() => print("=== Test 4: Three WeakMaps, multi-map cycle ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f4() {
        let k1 = {};
        let k2 = {};
        let k3 = {};
        let v1 = {};
        let v2 = {};
        let v3 = {};

        v1.ref = k2;
        v2.ref = k3;
        v3.ref = k1;

        registry.register(k1, "test4_k1_collected");
        registry.register(k2, "test4_k2_collected");
        registry.register(k3, "test4_k3_collected");
        registry.register(v1, "test4_v1_collected");
        registry.register(v2, "test4_v2_collected");
        registry.register(v3, "test4_v3_collected");

        wm1.set(k1, v1);
        wm2.set(k2, v2);
        wm3.set(k3, v3);
    }
    f4();
    ArkTools.forceFullGC();
}

// Test 5: V references K, but K has external strong reference
// Expected: First GC: K and V should NOT be collected (K is alive via strongK)
//           Second GC (after strongK=null): K and V should be collected
Promise.resolve().then(() => print("=== Test 5: V references K, K has external reference ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f() {
        let strongK = {};

        let vToken = {};
        function f5() {
            let k = strongK;  // K has strong reference from outer scope
            let v = {};
            v.ref = k;

            // Register V to verify it's NOT collected (no message should appear)
            registry.register(v, "test5_v_collected", vToken);

            wm.set(k, v);
        }
        f5();
        ArkTools.forceFullGC();

        // K is alive because strongK exists
        // V is alive because K is alive (entry is preserved)
        // Unregister V since we've verified it survives the first GC
        registry.unregister(vToken);

        // Clean up strong reference
        registry.register(strongK, "test5_k_eventually_collected");
    }
    f();
    ArkTools.forceFullGC();
}

// Test 6: V has external strong reference, and V references K
// Expected: K should NOT be collected (V is alive and V->K), V should NOT be collected (has external reference)
Promise.resolve().then(() => print("=== Test 6: V keeps K alive (V has external ref, V->K) ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    let strongV = {};
    let kToken = {};
    let vToken = {};

    function f6() {
        let k = {};
        let v = strongV;  // V has strong reference
        v.ref = k;

        // Register K and V to verify they are NOT collected
        registry.register(k, "test6_k_collected", kToken);
        registry.register(v, "test6_v_collected", vToken);

        wm.set(k, v);
    }
    f6();
    ArkTools.forceFullGC();

    // K and V should both survive the GC
    // Unregister to verify no callbacks were called (no output expected)
    registry.unregister(kToken);
    registry.unregister(vToken);
}

// Test 7: No reference cycle (V does not reference K)
// Expected: Both should be collected
Promise.resolve().then(() => print("=== Test 7: V does not reference K ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f7() {
        let k = {};
        let v = {};
        // V does NOT reference K

        registry.register(k, "test7_k_collected");
        registry.register(v, "test7_v_collected");

        wm.set(k, v);
    }
    f7();
    ArkTools.forceFullGC();
}

// Test 8: Mixed scenario - WeakMap strongly holds values when keys are alive
// Expected: First GC: K3, K4 collected (no external refs); V3, V4 collected (their keys were collected)
//                     K1, K2 survive (have external refs)
//                     V1, V2 survive (their keys are alive, WeakMap holds strong reference to values)
//           Second GC (after strongK1/K2=null): K1, K2 collected, then V1, V2 collected
Promise.resolve().then(() => print("=== Test 8: Mixed scenario ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f() {
        let strongK1 = {};
        let strongK2 = {};
        let k1Token = {};
        let k2Token = {};
        let v1Token = {};
        let v2Token = {};

        function f8() {
            let k3 = {};
            let k4 = {};
            let k2 = strongK2;  // Local reference to strongK2 for V2 to use

            let v1 = {};
            let v2 = {};
            let v3 = {};
            let v4 = {};

            // K1 has external ref, V1 does not reference K1
            // K2 has external ref, V2 references K2
            // K3 no external ref, V3 references K4
            // K4 no external ref, V4 does not reference K4

            v2.ref = k2;        // V2 references K2 (which has external ref)
            v3.ref = k4;        // V3 references K4

            registry.register(k3, "test8_k3_collected");   // Should be collected (no external ref)
            registry.register(k4, "test8_k4_collected");   // Should be collected (no external ref)
            registry.register(strongK1, "test8_k1_collected", k1Token);  // Should survive (has external ref)
            registry.register(strongK2, "test8_k2_collected", k2Token);  // Should survive (has external ref)
            registry.register(v1, "test8_v1_collected", v1Token);   // Should survive (K1 is alive, WeakMap holds V1)
            registry.register(v2, "test8_v2_collected", v2Token);   // Should survive (K2 is alive, WeakMap holds V2)
            registry.register(v3, "test8_v3_collected");   // Should be collected
            registry.register(v4, "test8_v4_collected");   // Should be collected

            wm.set(strongK1, v1);
            wm.set(strongK2, v2);
            wm.set(k3, v3);
            wm.set(k4, v4);
        }
        f8();
        ArkTools.forceFullGC();

        // First GC results:
        // - K3, K4 collected (no external refs)
        // - V3, V4 collected (their keys were collected)
        // - K1, K2 survive (have external refs via strongK1/strongK2)
        // - V1, V2 survive (their keys are alive, WeakMap strongly references values)

        // Verify K1, K2, V1, V2 survived first GC by unregistering their tokens
        registry.unregister(k1Token);
        registry.unregister(k2Token);
        registry.unregister(v1Token);
        registry.unregister(v2Token);

        // Clean up strong references
        registry.register(strongK1, "test8_k1_eventually_collected");
        registry.register(strongK2, "test8_k2_eventually_collected");
    }
    f();
    ArkTools.forceFullGC();
}

// Test 9: Deep nesting - V1->V2->V3->...->K
// Expected: All should be collected
Promise.resolve().then(() => print("=== Test 9: Deep reference chain ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f9() {
        let k = {};
        let v1 = {};
        let v2 = {};
        let v3 = {};

        // Create chain: V1->V2->V3->K
        v1.ref = v2;
        v2.ref = v3;
        v3.ref = k;

        registry.register(k, "test9_k_collected");
        registry.register(v1, "test9_v1_collected");
        registry.register(v2, "test9_v2_collected");
        registry.register(v3, "test9_v3_collected");

        wm.set(k, v1);
    }
    f9();
    ArkTools.forceFullGC();
}

// Test 10: Multiple Values referencing same Key, only one in WeakMap
// Expected: All should be collected
//           - k has no external ref, so k is collected
//           - when k is collected, weakmap entry is removed, v1 loses the strong reference
//           - v1 and v2, v3 are then collected (no other references)
Promise.resolve().then(() => print("=== Test 10: Multiple Values, same Key ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f10() {
        let k = {};
        let v1 = {};
        let v2 = {};
        let v3 = {};

        // All V's reference the same K, but only v1 is in WeakMap
        v1.ref = k;
        v2.ref = k;
        v3.ref = k;

        registry.register(k, "test10_k_collected");
        registry.register(v1, "test10_v1_collected");
        registry.register(v2, "test10_v2_collected");
        registry.register(v3, "test10_v3_collected");

        // Only v1 is set as the value in WeakMap
        // v2 and v3 are not in WeakMap and have no external refs
        wm.set(k, v1);
    }
    f10();
    ArkTools.forceFullGC();
}

// Test 11: Same Key in multiple WeakMaps
// Expected: When key is collected, all entries are removed from all maps
Promise.resolve().then(() => print("=== Test 11: Same Key in multiple WeakMaps ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f11() {
        let k = {};
        let v1 = {};
        let v2 = {};

        v1.ref = k;
        v2.ref = k;

        registry.register(k, "test11_k_collected");
        registry.register(v1, "test11_v1_collected");
        registry.register(v2, "test11_v2_collected");

        // Same key in two different WeakMaps
        wm1.set(k, v1);
        wm2.set(k, v2);
    }
    f11();
    ArkTools.forceFullGC();
}

// Test 12: WeakMap value is undefined or null
// Expected: Key and value should be collected (no external references)
Promise.resolve().then(() => print("=== Test 12: WeakMap value is undefined ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f12() {
        let k = {};
        // Value is undefined (not referencing anything)

        registry.register(k, "test12_k_collected");

        wm.set(k, undefined);
    }
    f12();
    ArkTools.forceFullGC();
}

// Test 13: Non-WeakMap value references Key (edge case)
// Expected: Key should be collected (WeakMap value v1 does NOT reference k)
//           v1 should be collected (its key k was collected)
//           v2 should be collected (no external ref, not in WeakMap)
// Note: This verifies that only WeakMap values referencing their key can keep the key alive
Promise.resolve().then(() => print("=== Test 13: Non-WeakMap value references Key ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f13() {
        let k = {};
        let v1 = {};  // This will be the WeakMap value
        let v2 = {};  // This is NOT in WeakMap but references K

        // v1 does NOT reference k (so k can be collected)
        v2.ref = k;  // v2 references k but v2 is NOT in WeakMap

        registry.register(k, "test13_k_collected");
        registry.register(v1, "test13_v1_collected");
        registry.register(v2, "test13_v2_collected");

        // Only v1 is in WeakMap
        wm.set(k, v1);
    }
    f13();
    ArkTools.forceFullGC();
}

// Test 14: Verify entry deletion after key collection
// Expected: After key is collected, entry is removed from WeakMap
Promise.resolve().then(() => print("=== Test 14: Verify entry deletion ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f14() {
        let k = {};
        let v = {};
        v.ref = k;

        registry.register(k, "test14_k_collected");
        registry.register(v, "test14_v_collected");

        wm.set(k, v);
    }

    f14();
    ArkTools.forceFullGC();

    // Key and value should be collected (messages printed above)
}

// Test 15: Single WeakMap with very long reference chain (200+ K-V pairs)
// Expected: All should be collected
// Structure: {k1:v1, k2:v2, ..., k200:v200} where v[i] references k[i+1]
Promise.resolve().then(() => print("=== Test 15: Long reference chain (200+), single WeakMap ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f15() {
        let keys = [];
        let values = [];

        // Create 200 K-V pairs
        for (let i = 0; i < 200; i++) {
            keys.push({});
            values.push({});
        }

        // Create chain: v1->k2, v2->k3, ..., v199->k200, v200->k1 (cycle)
        for (let i = 0; i < 200; i++) {
            let nextIndex = (i + 1) % 200;
            values[i].ref = keys[nextIndex];
        }

        // Add all entries to WeakMap
        for (let i = 0; i < 200; i++) {
            wm.set(keys[i], values[i]);
        }

        // Register ALL keys for verification
        for (let i = 0; i < 200; i++) {
            registry.register(keys[i], "test15_k" + i + "_collected");
        }
    }
    f15();
    ArkTools.forceFullGC();
}

// Test 16: Multiple WeakMaps (5 maps) with long cross-reference chains
// Expected: All should be collected
// Structure: Each map has 40 K-V pairs, total 200 entries
// v[i] references k[i+1], wrapping around across all 5 maps
Promise.resolve().then(() => print("=== Test 16: 5 WeakMaps with long cross-reference chains ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let wm4 = new WeakMap();
    let wm5 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f16() {
        let keys = [];
        let values = [];
        let maps = [wm1, wm2, wm3, wm4, wm5];

        // Create 200 K-V pairs (40 per map)
        for (let i = 0; i < 200; i++) {
            keys.push({});
            values.push({});
        }

        // Create chain: v[i] references k[i+1] (wraps around)
        for (let i = 0; i < 200; i++) {
            let nextIndex = (i + 1) % 200;
            values[i].ref = keys[nextIndex];
        }

        // Distribute across 5 WeakMaps (40 entries each)
        for (let i = 0; i < 200; i++) {
            let mapIndex = Math.floor(i / 40);
            maps[mapIndex].set(keys[i], values[i]);
        }

        // Register ALL keys for verification
        for (let i = 0; i < 200; i++) {
            registry.register(keys[i], "test16_k" + i + "_collected");
        }
    }
    f16();
    ArkTools.forceFullGC();
}

// Test 17: 5 WeakMaps with deep nesting and cross references
// Expected: All should be collected
// Structure: 200 K-V pairs, each value has a 10-level deep chain before referencing next key
Promise.resolve().then(() => print("=== Test 17: 5 WeakMaps with deep nesting and cross refs ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let wm4 = new WeakMap();
    let wm5 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f17() {
        let keys = [];
        let values = [];
        let maps = [wm1, wm2, wm3, wm4, wm5];

        // Create 200 K-V pairs
        for (let i = 0; i < 200; i++) {
            keys.push({});
            values.push({});
        }

        // Create deep chains (10 levels) for each value, then reference next key
        for (let i = 0; i < 200; i++) {
            let nextIndex = (i + 1) % 200;
            let chain = values[i];

            // Add 10 intermediate objects
            for (let j = 0; j < 10; j++) {
                let temp = {};
                temp.ref = chain;
                chain = temp;
            }

            // Last object in chain references next key
            chain.ref = keys[nextIndex];
            values[i] = chain;
        }

        // Distribute across 5 WeakMaps
        for (let i = 0; i < 200; i++) {
            let mapIndex = Math.floor(i / 40);
            maps[mapIndex].set(keys[i], values[i]);
        }

        // Register ALL keys for verification
        for (let i = 0; i < 200; i++) {
            registry.register(keys[i], "test17_k" + i + "_collected");
        }
    }
    f17();
    ArkTools.forceFullGC();
}

// Test 18: Very long cycle with 5 WeakMaps
// Expected: All should be collected
// Structure: 250 K-V pairs (50 per map), each value has 5-level chain, forming long cycle
Promise.resolve().then(() => print("=== Test 18: Very long cycle across 5 WeakMaps ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let wm4 = new WeakMap();
    let wm5 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    function f18() {
        let keys = [];
        let values = [];
        let maps = [wm1, wm2, wm3, wm4, wm5];

        // Create 250 K-V pairs (50 per map)
        for (let i = 0; i < 250; i++) {
            keys.push({});
            values.push({});
        }

        // Create chain: each value has 5 intermediate objects, then references next key
        for (let i = 0; i < 250; i++) {
            let nextIndex = (i + 1) % 250;
            let chain = values[i];

            // Add 5 intermediate objects
            for (let j = 0; j < 5; j++) {
                let temp = {};
                temp.ref = chain;
                chain = temp;
            }

            chain.ref = keys[nextIndex];
            values[i] = chain;
        }

        // Distribute across 5 WeakMaps (50 entries each)
        for (let i = 0; i < 250; i++) {
            let mapIndex = Math.floor(i / 50);
            maps[mapIndex].set(keys[i], values[i]);
        }

        // Register ALL keys for verification
        for (let i = 0; i < 250; i++) {
            registry.register(keys[i], "test18_k" + i + "_collected");
        }
    }
    f18();
    ArkTools.forceFullGC();
}

// Test 19: Long chain with one key having external reference
// Expected: All K-V pairs should survive (NOT be collected)
// Structure: 200 K-V pairs in a cycle, but k100 has external strong reference
// This should keep all 200 K and V alive due to the chain dependency
Promise.resolve().then(() => print("=== Test 19: Long chain (200) with external reference, all survive ==="));
{
    let wm = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    let strongK = {};  // External reference to keep the chain alive
    let tokens = [];    // Keep tokens accessible after function returns

    function f19() {
        let keys = [];
        let values = [];

        // Create 200 K-V pairs
        for (let i = 0; i < 200; i++) {
            keys.push({});
            values.push({});
        }

        // Set one key to have external reference (k100)
        keys[100] = strongK;

        // Create chain: v[i] references k[i+1]
        for (let i = 0; i < 200; i++) {
            let nextIndex = (i + 1) % 200;
            values[i].ref = keys[nextIndex];
        }

        // Add all entries to WeakMap
        for (let i = 0; i < 200; i++) {
            wm.set(keys[i], values[i]);
        }

        // Register ALL keys with tokens to verify they are NOT collected
        for (let i = 0; i < 200; i++) {
            let token = {};
            tokens.push(token);
            registry.register(keys[i], "test19_k" + i + "_collected", token);
        }
    }
    f19();
    // GC happens AFTER function returns, keys[] array is out of scope
    // Only strongK keeps the chain alive through k100
    ArkTools.forceFullGC();

    // Unregister all to verify no callbacks were called (no output expected)
    for (let i = 0; i < 200; i++) {
        registry.unregister(tokens[i]);
    }
}

// Test 20: 5 WeakMaps with one key having external reference
// Expected: All 250 K-V pairs should survive (NOT be collected)
// Structure: 250 K-V pairs across 5 maps in a cycle, k125 has external reference
Promise.resolve().then(() => print("=== Test 20: 5 WeakMaps with external reference, all survive ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let wm4 = new WeakMap();
    let wm5 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    let strongK = {};  // External reference to keep everything alive
    let tokens = [];    // Keep tokens accessible after function returns

    function f20() {
        let keys = [];
        let values = [];
        let maps = [wm1, wm2, wm3, wm4, wm5];

        // Create 250 K-V pairs (50 per map)
        for (let i = 0; i < 250; i++) {
            keys.push({});
            values.push({});
        }

        // Set one key to have external reference (k125)
        keys[125] = strongK;

        // Create chain: v[i] references k[i+1]
        for (let i = 0; i < 250; i++) {
            let nextIndex = (i + 1) % 250;
            values[i].ref = keys[nextIndex];
        }

        // Distribute across 5 WeakMaps (50 entries each)
        for (let i = 0; i < 250; i++) {
            let mapIndex = Math.floor(i / 50);
            maps[mapIndex].set(keys[i], values[i]);
        }

        // Register ALL keys with tokens to verify they are NOT collected
        for (let i = 0; i < 250; i++) {
            let token = {};
            tokens.push(token);
            registry.register(keys[i], "test20_k" + i + "_collected", token);
        }
    }
    f20();
    // GC happens AFTER function returns, keys[] array is out of scope
    // Only strongK keeps the chain alive through k125
    ArkTools.forceFullGC();

    // Unregister all to verify no callbacks were called (no output expected)
    for (let i = 0; i < 250; i++) {
        registry.unregister(tokens[i]);
    }
}

// Test 21: Long chain with deep nesting and one external reference
// Expected: All should survive
// Structure: 200 K-V pairs, each value has 10-level chain, k50 has external reference
Promise.resolve().then(() => print("=== Test 21: Deep nesting with external reference, all survive ==="));
{
    let wm1 = new WeakMap();
    let wm2 = new WeakMap();
    let wm3 = new WeakMap();
    let wm4 = new WeakMap();
    let wm5 = new WeakMap();
    let registry = new FinalizationRegistry((result) => {
        print(result);
    });

    let strongK = {};  // External reference
    let tokens = [];    // Keep tokens accessible after function returns

    function f21() {
        let keys = [];
        let values = [];
        let maps = [wm1, wm2, wm3, wm4, wm5];

        // Create 200 K-V pairs
        for (let i = 0; i < 200; i++) {
            keys.push({});
            values.push({});
        }

        // Set one key to have external reference (k50)
        keys[50] = strongK;

        // Create deep chains (10 levels) for each value, then reference next key
        for (let i = 0; i < 200; i++) {
            let nextIndex = (i + 1) % 200;
            let chain = values[i];

            // Add 10 intermediate objects
            for (let j = 0; j < 10; j++) {
                let temp = {};
                temp.ref = chain;
                chain = temp;
            }

            // Last object in chain references next key
            chain.ref = keys[nextIndex];
            values[i] = chain;
        }

        // Distribute across 5 WeakMaps
        for (let i = 0; i < 200; i++) {
            let mapIndex = Math.floor(i / 40);
            maps[mapIndex].set(keys[i], values[i]);
        }

        // Register ALL keys with tokens to verify they are NOT collected
        for (let i = 0; i < 200; i++) {
            let token = {};
            tokens.push(token);
            registry.register(keys[i], "test21_k" + i + "_collected", token);
        }
    }
    f21();
    // GC happens AFTER function returns, keys[] array is out of scope
    // Only strongK keeps the chain alive through k50
    ArkTools.forceFullGC();

    // Unregister all to verify no callbacks were called (no output expected)
    for (let i = 0; i < 200; i++) {
        registry.unregister(tokens[i]);
    }
}

Promise.resolve().then(() => print("=== All tests completed ==="));
