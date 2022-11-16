declare function print(str:any):void;

class A {
    add(a : number, b : number) : number {
        return a + b;
    }

    select(a : number, b : number) : number {
        return a > b ? a : b;
    }

    foo(a: number) : number {
        return this.add(a*2, a);
    }

    bar(a: number) : number {
        return this.select(a*2, a);
    }
}

let a : A = new A();
print(a.foo(2));
print(a.bar(2));
