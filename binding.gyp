{
  "targets": [
    {
      "target_name": "asyncprog",
      "sources": [
        "usbs.h",
        "cond_var.h",
        "async_prog.cpp"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
