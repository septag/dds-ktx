## dds-ktx: Portable single header DDS/KTX reader for C/C++
[@septag](https://twitter.com/septagh)

- Parses from memory blob. No allocations
- No dependencies
- Single-header for easy integration
- Overridable libc functions 

### Build Example (ctexview)
[**ctexview**](ctexview/ctexview.c) is a tiny ddx-ktx viewer that can be built on linux/mac and windows. To build it just compile the single file with your compiler.

**Windows:**
```
cl ctexview.c /O2
```

**Linuxx**
```
gcc ctexview.c -O2 -lGL -ldl -lX11 -lXi -lXcursor -lm -o ctexview
```

**MacOS**
```
clang -framework Foundation -framework CoreServices -framework CoreFoundation -O2 -fobjc-arc -x objective-c -fmodules -x objective-c ctexview.c -o ./ctexview
```

to view images just provide the image path as an argument:

```
ctexview [dds_or_ktx_image_file_path]
```

Used open-source libraries for app creation/graphics: [**Sokol**](https://github.com/floooh/sokol)

**Keys:**

- UP/DOWN: change current mipmap
- Apostrophe: change text color
- F: Next cube-map face
- R: Toggle Red channel
- G: Toggle Green channel
- B: Toggle Blue channel
- A: Toggle Alpha channel

### Usage
In this example, a simple 2D texture is parsed and created using OpenGL

```c
#define DDSKTX_IMPLEMENT
#include "dds-ktx.h"

int size;
void* dds_data = load_file("test.dds", &size);
assert(dds_data);
ddsktx_texture_info tc = {0};
GLuint tex = 0;
if (ddsktx_parse(&tc, dds_data, size, NULL)) {
    assert(tc.depth == 1);
    assert(!(tc.flags & STC_TEXTURE_FLAG_CUBEMAP));
    assert(tc.num_layers == 1);

    //Create GPU texture from tc data
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(img->gl_target, tex);

    for (int mip = 0; mip < tc->num_mips; mip++) {
        ddsktx_sub_data sub_data;
        ddsktx_get_sub(&tc, &sub_data, dds_data, size, 0, 0, mip);
        // Fill/Set texture sub resource data (mips in this case)
        if (ddsktx_format_compressed(tc.format))
            glCompressedTexImage2D(..);
        else
            glTexImage2D(..);
    }

    // Now we can delete file data
    free(dds_data);
}
```

### Links
- [DdsKtxSharp](https://github.com/rds1983/DdsKtxSharp): C# port of dds-ktx by [Roman Shapiro](https://github.com/rds1983)

### TODO

- Write KTX/DDS
- Read KTX metadata. currently it just stores the offset/size to the metadata block
  
### Others

- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) - Single header library that loads images (.png, .jpg, .bmp, etc)
- [bimg](https://github.com/bkaradzic/bimg) - Extensive C++ image library

**NOTE**: Many parts of the code is taken from _bimg_ library.

  
[License (BSD 2-clause)](https://github.com/septag/dds-ktx/blob/master/LICENSE)
--------------------------------------------------------------------------

<a href="http://opensource.org/licenses/BSD-2-Clause" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

    Copyright 2018 Sepehr Taghdisian. All rights reserved.
    
    https://github.com/septag/dds-ktx
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
       1. Redistributions of source code must retain the above copyright notice,
          this list of conditions and the following disclaimer.
    
       2. Redistributions in binary form must reproduce the above copyright notice,
          this list of conditions and the following disclaimer in the documentation
          and/or other materials provided with the distribution.
    
    THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
    EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
