#include <stdio.h>
#include <stdlib.h>

#include "pngquant/rwpng.h"
#include "pngquant/lib/libimagequant.h"
#include "pngquant/lib/pam.h"

#define GAMMA 0.45455
#define FAST_COMPRESSION 1

void
read_image(liq_attr *attr, FILE *source, png24_image *png, liq_image **image)
{
    rwpng_read_image24(source, png);
    *image = liq_image_create_rgba_rows(attr, (void **) png->row_pointers,
                                        png->width, png->height, png->gamma);
}

void
prepare_output_image(liq_result *rv, liq_image *image, png8_image *png)
{
    unsigned int i;
    const liq_palette *palette;
    
    png->width = liq_image_get_width(image);
    png->height = liq_image_get_height(image);

    png->indexed_data = malloc(png->width * png->height);
    png->row_pointers = malloc(png->height * sizeof(png->row_pointers[0]));

    for (i = 0;i < png->height;i++) {
        png->row_pointers[i] = png->indexed_data + i * png->width;
    }
    
    palette = liq_get_palette(rv);
    png->num_palette = palette->count;
    png->num_trans = 0;
    for (i = 0;i < palette->count;i++) {
        if (palette->entries[i].a < 255) {
            png->num_trans = i + 1;
        }
    }
}

void
set_palette(liq_result *rv, png8_image *png)
{
    unsigned int i;
    liq_color px;
    const liq_palette *palette;

    palette = liq_get_palette(rv);
    png->num_palette = palette->count;
    png->num_trans = 0;
    for (i = 0;i < palette->count;i++) {
        px = palette->entries[i];
        if (px.a < 255) {
            png->num_trans = i + 1;
        }
        png->palette[i] = (png_color) {.red=px.r, .green=px.g, .blue=px.b};
        png->trans[i] = px.a;
    }
}

void
png8_image_destroy(png8_image *png)
{
    free(png->indexed_data);
    png->indexed_data = NULL;

    free(png->row_pointers);
    png->row_pointers = NULL;
}

void
png24_image_destroy(png24_image *png)
{
    free(png->rgba_data);
    free(png->row_pointers);
}

void
pngquant_tiny(FILE *src, FILE *dest)
{
    liq_attr *attr;
    liq_result *rv;
    liq_image *input_image = NULL;
    png24_image input_png = {};
    png8_image output_png = {};

    attr = liq_attr_create();
    read_image(attr, src, &input_png, &input_image);

    rv = liq_quantize_image(attr, input_image);

    liq_set_output_gamma(rv, GAMMA);
    prepare_output_image(rv, input_image, &output_png);
    liq_write_remapped_image_rows(rv, input_image, output_png.row_pointers);
    set_palette(rv, &output_png);
    output_png.fast_compression = FAST_COMPRESSION;

    rwpng_write_image8(dest, &output_png);

    liq_result_destroy(rv);
    liq_image_destroy(input_image);
    png24_image_destroy(&input_png);
    png8_image_destroy(&output_png);
}
