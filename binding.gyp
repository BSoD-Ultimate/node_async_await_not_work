{
  "targets": [
    {
      "target_name": "uv_async_hang",
      "sources": [
        "async_hang.cpp",
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")"]
    }
  ]
}
