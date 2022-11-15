function emptycompare(x, y) {
    if (x > y);
    return 1;
}

function emptyiftrue() {
    if (true);
    return 2;
}

print(emptycompare(2, 3));
print(emptyiftrue());
