declare function AssertType(value:any, type:string):void;
{
    class A {
        v:number;
        constructor(v:number) {
            this.v = v;
        }
    }

    function processResults(v: number) {
        for (let i = 0; i < 10; i++) {
            v += i;
        }
        for (let i = 0; i < 10; i++) {
            v += i;
        }
        let s = new A(v);
        AssertType(s, "A");
    }
}
