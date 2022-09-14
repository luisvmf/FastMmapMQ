{
	"target_defaults": {
    "cflags": ["-O3","-fpermissive"],
    "default_configuration": "Release",
    "defines": [],
    "include_dirs": [],
    "libraries": []
  },
  "targets": [
    {
      "target_name": "fastmmapmq",
      "sources": [ "fastmmapmq-node.cc" ],
      "include_dirs" : ["<!(node -e \"require('nan')\")"]
    }
  ],
}
