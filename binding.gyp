{
  "targets": [
    {
      "target_name": "asyncprog",
      "sources": [
        "async_prog.cpp"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      'conditions': [
        ['OS=="win"',
          {
            'sources': [
              "async_prog.cpp"
            ],
            'include_dirs+':
            [
              # Not needed now
            ]
          }
        ]
      ]
    }
  ]
}
