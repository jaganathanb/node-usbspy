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
      ]
    }
  ]
}
