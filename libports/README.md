ZeroVM ported libraries
=====

lua-5.2.1
----
Compiled automatically with ZRT

sqlite3
----
Compiled automatically with ZRT

tar-1.11.8
----
Compiled automatically with ZRT

----------

To compile all other libraries first add `$ZVM_SDK_ROOT/bin` to the `PATH`
We assume that `PREFIX` environment variable points to sysroot.
Sysroot is set to `$ZRT_ROOT/zerovm-dev` by default in ZRT make files.
Therefore <b>do</b> `export PREFIX=$ZRT_ROOT/zerovm-dev` or something similar.
Please try to compile things in order we did in this manual, there are dependencies between various libraries.

zlib-1.2.3.4
----
    cd zlib-1.2.3.4.dfsg
    CC="x86_64-nacl-gcc" ./configure --static --prefix=$PREFIX
    make
    make install
    
pkg-config-0.27.1
---
We do not need to port pkg-config, only set it up with correct prefix.
    
    cd pkg-config-0.27.1
    ./configure --prefix=$PREFIX
    make install
    
bzip2-1.0.6
---
    cd bzip2-1.0.6
    make
    make install PREFIX=$PREFIX
    
xz-utils-5.1.1 (lzma)
---
    cd xz-utils-5.1.1alpha+20110809
    autoreconf -vif
    ./configure --host=x86_64-nacl --enable-static --disable-threads --prefix=$PREFIX
    make
    make install
    
libpng-1.2.46
---
    cd libpng-1.2.46
    CFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
libjpeg6b-6b1
---
    cd libjpeg6b-6b1
    ./configure --host=x86_64-nacl --enable-static --disable-shared --without-pic --prefix=$PREFIX
    make
    make install
    
tiff-3.9.5
---
    cd tiff-3.9.5
    CFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
libxml2-2.7.8
---
    cd libxml2-2.7.8.dfsg
    CPPFLAGS="-I$PREFIX/include" CFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib" ./configure --host=x86_64-nacl --enable-static --disable-shared --without-threads --without-python --prefix=$PREFIX
    make
    make install
    
pcre-8.32
---
    cd pcre-8.32
    CPPFLAGS="-I$PREFIX/include" CFLAGS="-I$PREFIX/include" LDFLAGS="-L$PREFIX/lib" ./configure --host=x86_64-nacl --enable-utf --with-sysroot=$PREFIX --prefix=$PREFIX
    make
    make install
    
libwebp-0.1.3
---
    cd libwebp-0.1.3
    PKG_CONFIG="${PREFIX}/bin/pkg-config" \
    CFLAGS="-I${PREFIX}/include" \
    CPPFLAGS="-I${PREFIX}/include" \
    LDFLAGS="-L${PREFIX}/lib" \
    LIBS="-lz" \
    ./configure \
    --host=x86_64-nacl \
    --enable-static \
    --prefix=$PREFIX
    make
    make install
    
jasper-1.900.1
---
    cd jasper-1.900.1
    CFLAGS="-I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
lcms-1.19
---
    cd lcms-1.19.dfsg
    CFLAGS="-I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
lcms2-2.2
---
    cd lcms2-2.2+git20110628
    CFLAGS="-I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install

freetype-2.4.8
---
    cd freetype-2.4.8/freetype-2.4.8
    CFLAGS="-I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
fontconfig-2.8.0
---
    cd fontconfig-2.8.0
    CFLAGS="-I${PREFIX}/include" \
    CPPFLAGS="-I${PREFIX}/include" \
    LDFLAGS="-L${PREFIX}/lib" \
    LIBXML2_CFLAGS="-I${PREFIX}/include/libxml2" \
    LIBXML2_LIBS="-L${PREFIX}/lib -lxml2" \
    ./configure \
    --host=x86_64-nacl \
    --enable-static \
    --disable-shared \
    --with-arch=x86_64 \
    --with-freetype-config=${PREFIX}/bin/freetype-config \
    --prefix=${PREFIX}
    make
    make install
    
imagemagick-6.6.9.7
---
    cd imagemagick-6.6.9.7
    CFLAGS="-I${PREFIX}/include -I${PREFIX}/include/libxml2" \
    LDFLAGS="-L${PREFIX}/lib" \
    FREETYPE_CFLAGS="-I${PREFIX}/include/freetype2" \
    FREETYPE_LIBS="-L${PREFIX}/lib -lfreetype" \
    FONTCONFIG_CFLAGS="-I${PREFIX}/include/fontconfig" \
    FONTCONFIG_LIBS="-L${PREFIX}/lib -lfontconfig" \
    PKG_CONFIG="${PREFIX}/bin/pkg-config" \
    ac_cv_prog_freetype_config="${PREFIX}/bin/freetype-config" \
    ac_cv_sys_file_offset_bits=64 \
    ./configure \
    --host=x86_64-nacl \
    --enable-static \
    --without-threads \
    --enable-largefile \
    --with-fontconfig \
    --prefix=${PREFIX}
    make
    make install

