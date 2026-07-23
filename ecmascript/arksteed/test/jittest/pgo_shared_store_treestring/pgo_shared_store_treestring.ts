// @ts-nocheck
class SharedStringHolder {
    value: string = "";

    constructor(value: string) {
        "use sendable";
        this.value = value;
    }
}

function storeSharedString(holder: SharedStringHolder, value: string): string {
    holder.value = value;
    return holder.value;
}

let holder = new SharedStringHolder("warmup");
for (let i = 0; i < 20000; i++) {
    storeSharedString(holder, "warmup");
}

ArkTools.arkSteedCompileSync(storeSharedString);
ArkTools.waitJitCompileFinish(storeSharedString);

let left = "aaaaaaaaaaaaaaaaaaaaaa";
let right = "bbbbbbbbbbbbbbbbbbbbbb";
let treeString = left + right;
let result = storeSharedString(holder, treeString);
let ok = ArkTools.isAOTCompiled(storeSharedString);
ok = ok && ArkTools.isTreeString(treeString);
ok = ok && !ArkTools.isTreeString(holder.value);
ok = ok && result === treeString && holder.value === treeString;
print(ok ? "PASS" : "FAIL");
