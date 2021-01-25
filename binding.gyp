{
    'targets': [
	{
	    'target_name': 'hello',
	    'type': 'executable',
	    'sources': [
		'src/native/hello.cc',
	    ],
	},
	{
	    'target_name': 'copy',
	    'type': 'none',
	    'dependencies': ['hello'],
	    'copies': [
		{
		    'destination': 'native_build',
		    'files': ['<(PRODUCT_DIR)/hello']
		},
	    ],
	},
    ],
}
