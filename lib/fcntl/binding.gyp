{
	'targets': [
		{
			'target_name': 'hfpio',
			'sources': [
				'src/hfpio.cc'
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
						'<!@(pkg-config --cflags sbc)',
						'<!@(pkg-config --cflags alsa)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other sbc)',
						'<!@(pkg-config  --libs-only-L --libs-only-other alsa)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other sbc)',
						'<!@(pkg-config  --libs-only-l --libs-only-other alsa)'
					]
				}]
			]
		}
	]
}
