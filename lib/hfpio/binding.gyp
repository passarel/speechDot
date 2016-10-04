{
	'targets': [
		{
			'target_name': 'hfpio',
			'sources': [
				'src/hfpio.cc',
			],
			'include_dirs': [
				"<!(node -e \"require('nan')\")"
			],
			'conditions': [
				['OS=="linux"', {
					'defines': [
					],
					'cflags': [
						'-std=gnu++0x'
					],
					'ldflags': [
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
