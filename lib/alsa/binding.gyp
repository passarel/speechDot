{
	'targets': [
		{
			'target_name': 'alsa',
			'sources': [
				'src/alsa.cc',
				'src/alsa.h'
			],
			'include_dirs': [
				"<!(node -e \"require('nan')\")"
			],
			'conditions': [
				['OS=="linux"', {
					'defines': [
						'LIB_EXPAT=expat'
					],
					'cflags': [
						'-std=gnu++0x',
						'<!@(pkg-config --cflags alsa)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other alsa)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other alsa)'
					]
				}]
			]
		}
	]
}
