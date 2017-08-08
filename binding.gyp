{
	'targets': [{
		'target_name': 'microphone-node',
		'sources': [
			'src/microphone.cpp'
		],
		'conditions': [
			['OS=="win"',
				{
					'sources': [ 'src/microphone_windows.cpp' ],
					"cflags" : [ "-lole32", "-loleaut32"]
				}
			],
			['OS=="mac"',
				{
					'sources': [
						'src/CFAMicrophone/CFAMicrophone/CFAAudioObject.cpp',
						'src/microphone_darwin.cpp'
					],
					"xcode_settings": { 
						"OTHER_CPLUSPLUSFLAGS" : ["-std=c++11","-stdlib=libc++", "-x objective-c++"], 
						"OTHER_LDFLAGS": [
							"-stdlib=libc++ -framework CoreFoundation -framework Foundation -framework IOKit -framework CoreAudio"
						], 
						"MACOSX_DEPLOYMENT_TARGET": "10.7"
					} 
				}
			],
		]
	}],
}