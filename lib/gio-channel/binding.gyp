{
	'targets': [
		{
			'target_name': 'gio-channel',
			'sources': [
				'src/gio_channel.cc',
			],
			'include_dirs': [
				"<!(node -e \"require('nan')\")",
				"/usr/include/glib-2.0",
				"/usr/lib/arm-linux-gnueabihf/glib-2.0/include"
			],
			'conditions': [
				['OS=="linux"', {
					'defines': [
					],
					'cflags': [
						'-std=gnu++0x',
						'<!@(pkg-config  --libs-only-L --libs-only-other glib-2.0)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other glib-2.0)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other glib-2.0)'
					]
				}]
			]
		}
	]
}
