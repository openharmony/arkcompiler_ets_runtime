# ecmascript/string

String implementation with multiple string representations (tree, line, sliced, external) and efficient Unicode handling.
Primary Language: C++

## Design

Multiple string representations for memory efficiency:

```
  String creation
       │
       ├──▶ TreeString: Binary tree for concatenation
       ├──▶ LineString: Single-line string
       ├──▶ SlicedString: View into parent string
       └──▶ ExternalString: External memory buffer
       │
       ▼
  BaseString (common interface)
       ├── Length, charAt, substring operations
       ├── Hash computation
       └── Comparison and equality
```

**Key abstractions:**
- **BaseString** — Common string interface and base class
- **TreeString** — Binary tree node with left and right substrings for efficient concatenation
- **LineString** — Contiguous string data
- **SlicedString** — Zero-copy substring view
- **ExternalString** — External memory reference
- **BaseStringTable** — String interning and deduplication
- **IntegerCache** — Cached integer string representations

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/string/base_string.h` (1-300行) - Base string interface
- `arkcompiler/ets_runtime/ecmascript/string/tree_string.h` - Tree string implementation

## Key Files

| File | Role |
|------|------|
| `base_string.h/.cpp` | Base string class and interface |
| `tree_string.h` | Tree string for concatenation |
| `line_string.h` | Contiguous string data |
| `sliced_string.h` | Substring view |
| `external_string.h` | External memory string |
| `base_string_table.h/.cpp` | String interning |
| `integer_cache.h` | Integer string cache |
| `hashtriemap.h` | String hash table implementation |

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
```

## Tests

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug unittest   # includes string tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/string/tests:host_unittest
```

## Boundaries

- String representations are immutable; mutations create new strings.
- TreeString is a binary tree, not balanced; character access traverses left/right subtrees.
- TreeString is flat if right substring is empty; enables flattening optimization.
- ExternalString lifetime management is caller's responsibility.
- Unicode handling follows ECMAScript spec (UTF-16 code units).
