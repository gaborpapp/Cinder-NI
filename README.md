
Cinder-Oni
----------

OpenNI2 Cinder block

###Updating the OpenNI2 lib on OS X
In OpenNI2 2.3 the install name of the library is modified to make it easier to install.

Original install name
<pre>
$ otool -D libOpenNI2.dylib 
libOpenNI2.dylib:
libOpenNI2.dylib
</pre>

Changed to relative path
<pre>
$ install_name_tool -id @loader_path/libOpenNI2.dylib libOpenNI2.dylib
</pre>

