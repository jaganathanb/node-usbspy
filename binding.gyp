{
  "targets": [
    {
      "target_name": "usbspy",
      "sources": [
        "src/usbspy.cpp",
        "src/usbspy.h",
        "src/usbs.cpp",
        "src/usbs.h"
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
