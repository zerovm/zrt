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

Prerequisites
----

Install ZeroVM SDK.

You need to set up `ZVM_PREFIX` to the ZeroVM SDK installtion dir.
Example: `export ZVM_PREFIX=/opt/zerovm`

    export PREFIX="$ZVM_PREFIX/x86_64-nacl"
    export PATH="$PATH:$ZVM_PREFIX/bin"

You will need to install the following packages

    sudo apt-get install autoconf autopoint libtool build-essential

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
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
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
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
libxml2-2.7.8
---
    cd libxml2-2.7.8.dfsg
    ./configure --host=x86_64-nacl --enable-static --disable-shared --without-threads --without-python --prefix=$PREFIX
    make
    make install
    
pcre-8.32
---
    cd pcre-8.32
    ./configure --host=x86_64-nacl --enable-utf --with-sysroot=$PREFIX --prefix=$PREFIX
    make
    make install
    
libwebp-0.1.3
---
    cd libwebp-0.1.3
    PKG_CONFIG="${PREFIX}/bin/pkg-config" \
    ./configure \
    --host=x86_64-nacl \
    --enable-static \
    --prefix=$PREFIX
    make
    make install
    
jasper-1.900.1
---
    cd jasper-1.900.1
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
lcms-1.19
---
    cd lcms-1.19.dfsg
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
lcms2-2.2
---
    cd lcms2-2.2+git20110628
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install

freetype-2.4.8
---
    cd freetype-2.4.8/freetype-2.4.8
    ./configure --host=x86_64-nacl --enable-static --prefix=$PREFIX
    make
    make install
    
fontconfig-2.8.0
---
    cd fontconfig-2.8.0
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

