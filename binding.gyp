{
	"targets": [
		{
			"target_name" : "libtracer",
			"type" : "static_library",
			"sources" : [
				"./libtracer/basic.observer.cpp",
				"./libtracer/simple.tracer/simple.tracer.cpp",
        "./libtracer/annotated.tracer/TrackingExecutor.cpp",
        "./libtracer/annotated.tracer/BitMap.cpp",
        "./libtracer/annotated.tracer/annotated.tracer.cpp"
			],
			"include_dirs": [
				"./include",
        "./river.format/include",
        "<!(echo $RIVER_SDK_DIR)/include/"
			],
      "conditions": [
				["OS==\"linux\"", {
					"cflags": [
						"-g",
						"-m32",
						"-std=c++11",
						"-D__cdecl=''",
						"-D__stdcall=''"
						]
					}
				]
			]
		}
	]
}
