import sys

def getSources(relpath):
	_SOURCES = ['CiNI.cpp', 'CiNIUserTracker.cpp']
	return [relpath + '../src/' + s for s in _SOURCES]

def getIncludes(relpath):
	_INCLUDES = [relpath + '../src',
			'/usr/include/ni/', '/usr/include/nite/']
	return _INCLUDES

def getLibs(relpath):
	return ['OpenNI', 'usb-1.0']

def getLibPath(relpath):
	if sys.platform == 'darwin':
		return ['/usr/lib/', '/opt/local/lib/']
	else:
		return [] # TODO

