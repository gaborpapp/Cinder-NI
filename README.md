
Cinder-Oni
----------

OpenNI2 Cinder block

###Updating the OpenNI2/NiTE2 lib on OS X
In OpenNI2 2.3/NiTE 2.2 the install name of the librares are modified to make
it easier to install.

Original install name
<pre>
$ otool -D libOpenNI2.dylib
libOpenNI2.dylib:
libOpenNI2.dylib
$ otool -D libNiTE2.dylib
libNiTE2.dylib:
libNiTE2.dylib
</pre>

Changed to relative path
<pre>
$ install_name_tool -id @loader_path/libOpenNI2.dylib libOpenNI2.dylib
$ install_name_tool -id @loader_path/libNiTE2.dylib libNiTE2.dylib
$ install_name_tool -change libOpenNI2.dylib @executable_path/libOpenNI2.dylib libNiTE2.dylib
</pre>

