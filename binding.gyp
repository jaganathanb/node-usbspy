{
  "targets": [
    {
      "target_name": "usbspy",
      "sources": [
        "src/usbs.h",
        "src/usbspy.h",
        "src/usbs.cpp",
        "src/usbspy.cpp"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      'conditions': [
        ['OS=="win"',
          {
            'sources': [
              "src/usbspy_win.cpp"
            ],
            'include_dirs+': []
          }
        ]
      ]
    }
  ]
}
