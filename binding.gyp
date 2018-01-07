{
  "targets": [
    {
      "target_name": "asyncprog",
      "sources": [
        "usbs.h",
        "cond_var.h",
        "usbs.cpp",
        "async_prog.cpp"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
