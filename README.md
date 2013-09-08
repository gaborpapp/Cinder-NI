
Cinder-Oni
----------

OpenNI2 Cinder block

###Updating the OpenNI2 lib
In OpenNI2 2.3 the install name of the library is modified to make it easier to install.

Original install name
<pre>
$ otool -D libOpenNI2.dylib 
libOpenNI2.dylib:
libOpenNI2.dylib
</pre>

Changed to relative path
<pre>
$ install\_name\_tool -id @loader\_path/libOpenNI2.dylib libOpenNI2.dylib 
</pre>

