pixelpunch
==========

Pixelpunch let's you rotate, sheer and scale pixel graphics and tweak the result by picking from a wide range of different upscaling, transformation and sampling algorithms.

It's a discontinued experiment but due to popular request I uploaded the source code. If you care about software licenses consider it released under MIT.

It's C++ code and requires the Cinder Framework to run.
http://libcinder.org/releases/cinder_0.8.5_vc2012.zip

The easiest way to compile it is to treat it as another of Cinder's many sample apps.
Here's what you need to do if you happen to use VS 2012.

* In "cinder\samples" add another folder called "PixelPunch"
* Copy the sourcecode there.
* Open "AllSamples.sln"
* Add Existing Project and chose "cinder\samples\PixelPunch\vc11\PixelPunch.vcxproj"
* Compile & Profit

References:

http://wayofthepixel.net/index.php?topic=12502.0
http://www.alonsomartin.mx/hfa/2013/10/30/spriting-tips/
