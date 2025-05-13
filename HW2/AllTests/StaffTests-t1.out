└──Funcs
    ├──FuncDecl
    │   ├──ID: main
    │   ├──PrimitiveType: void
    │   ├──Formals
    │   └──Statements
    │       ├──VarDecl
    │       │   ├──ID: x
    │       │   ├──PrimitiveType: int
    │       │   └──BinOp: *
    │       │       ├──BinOp: +
    │       │       │   ├──Num: 4
    │       │       │   └──Num: 7
    │       │       └──Num: 12
    │       ├──VarDecl
    │       │   ├──ID: y
    │       │   ├──PrimitiveType: byte
    │       │   └──NumB: 100
    │       ├──Call
    │       │   ├──ID: printOk
    │       │   └──ExpList
    │       │       └──And
    │       │           ├──RelOp: >
    │       │           │   ├──ID: x
    │       │           │   └──ID: y
    │       │           └──RelOp: <
    │       │               ├──ID: x
    │       │               └──Num: 100
    │       └──VarDecl
    │           ├──ID: z
    │           └──PrimitiveType: int
    └──FuncDecl
        ├──ID: printOk
        ├──PrimitiveType: void
        ├──Formals
        │   └──Formal
        │       ├──ID: isOk
        │       └──PrimitiveType: bool
        └──Statements
            ├──VarDecl
            │   ├──ID: i
            │   ├──PrimitiveType: int
            │   └──Num: 0
            ├──While
            │   ├──RelOp: <
            │   │   ├──ID: i
            │   │   └──Num: 10
            │   └──Statements
            │       ├──Call
            │       │   ├──ID: print
            │       │   └──ExpList
            │       │       └──String: i = 
            │       ├──Call
            │       │   ├──ID: printi
            │       │   └──ExpList
            │       │       └──ID: i
            │       └──Assign
            │           ├──ID: i
            │           └──BinOp: +
            │               ├──ID: i
            │               └──Num: 1
            └──If
                ├──ID: isOk
                ├──Statements
                │   ├──VarDecl
                │   │   ├──ID: x
                │   │   ├──PrimitiveType: int
                │   │   └──Num: 0
                │   └──Call
                │       ├──ID: print
                │       └──ExpList
                │           └──String: ok
                └──Call
                    ├──ID: print
                    └──ExpList
                        └──String: not ok
