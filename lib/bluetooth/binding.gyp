{
	'targets': [
	
		{
			'target_name': 'hfpio',
			'sources': [
				'src/hfpio.cc',
				'src/hfpio.h',
				'src/utils.c',
				'src/utils.h'
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
						'<!@(pkg-config --cflags alsa)',
						'<!@(pkg-config --cflags dbus-1)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other sbc)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other sbc)'
					]
				}]
			]
		},
		
		{
			'target_name': 'a2dp',
			'sources': [
				'src/a2dp.cc'
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
						'<!@(pkg-config --cflags alsa)',
						'<!@(pkg-config --cflags dbus-1)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other sbc)',
						'<!@(pkg-config  --libs-only-L --libs-only-other dbus-1)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other sbc)',
						'<!@(pkg-config  --libs-only-L --libs-only-other dbus-1)'
					]
				}]
			]
		}
		
	]
}
