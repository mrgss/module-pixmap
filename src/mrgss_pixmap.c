#define STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <stb/stb_image.h>
#include <mruby.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mrgss.h>
#include <mrgss/mrgss-types.h>
#include <mrgss/mrgss-pixmap.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void
pixmap_free(mrb_state *mrb, void *data) {
    mrgss_pixmap *pixmap = data;
    if (pixmap) {
        if (pixmap->data) {
            stbi_image_free(pixmap->data);
        }
        free(pixmap);
    };
}

struct mrb_data_type const mrbal_pixmap_data_type = {"Pixmap", pixmap_free};

mrb_value initialize(mrb_state *mrb, mrb_value self) {
    mrb_int iwidth = 0, iheight = 0, icomp;
    mrb_int param_count, height, size;
    mrb_value first_param;
    mrgss_pixmap* pixmap;
    const char* filedir;
    param_count = mrb_get_args(mrb, "o|i", &first_param, &height);
    pixmap = mrb_malloc(mrb, sizeof (mrgss_pixmap));
    switch (param_count) {
        case 1:
            if (mrb_string_p(first_param)) {
                filedir = mrb_string_value_ptr(mrb, first_param);
#ifdef __EMSCRIPTEN__
                void* buff;
                int pnum, perror;
                emscripten_wget_data(filedir, &buff, &pnum, &perror);
                pixmap->data = stbi_load_from_memory(buff, pnum, &iwidth, &iheight, &icomp, 4);
                pixmap->width = iwidth;
                pixmap->height = iheight;
#else
                pixmap->data = stbi_load(filedir, &iwidth, &iheight, &icomp, 4);
                pixmap->width = iwidth;
                pixmap->height = iheight;
#endif
            } else {
                mrgss_raise(mrb, E_ARGUMENT_ERROR, "Expecting a String");
                return mrb_nil_value();
            }
            if (!pixmap->data) {
                
                mrgss_raise(mrb, E_RUNTIME_ERROR, "Cannot create Pixmap. File not found");
                return mrb_nil_value();
            }
            break;
        case 2:
            iwidth = mrb_int(mrb, first_param);
            if (iwidth < 1 || iheight < 1) {
                mrgss_raise(mrb, E_ARGUMENT_ERROR, "Pixmap size must be at least 1");
                return mrb_nil_value();
            }
            pixmap->width = iwidth;
            pixmap->height = iheight;
            size = iwidth * iheight * 4;
            pixmap->data = mrb_malloc_simple(mrb, size);
            if (pixmap->data) {
                mrb_int i;
                for (i = 0; i < size; ++i) {
                    pixmap->data[i] = 0;
                }
            } else {
                mrgss_raise(mrb, E_RUNTIME_ERROR, "Cannot create Pixmap. Out of Memory");
                return mrb_nil_value();
            }
            break;
        default:
            mrgss_raise(mrb, E_RUNTIME_ERROR, "Unexpected error");
            return mrb_nil_value();
            break;
    }
    DATA_TYPE(self) = &mrbal_pixmap_data_type;
    DATA_PTR(self) = pixmap;
    return self;
}

static mrb_value
dispose(mrb_state *mrb, mrb_value self) {
    mrgss_pixmap *pixmap;
    pixmap = DATA_PTR(self);
    if (mrgss_is_disposable(mrb, self)) {
        pixmap_free(mrb, pixmap);
    } else {
        mrgss_raise(mrb, E_RUNTIME_ERROR, "Pixmap Already Disposed");
    }
    DATA_PTR(self) = NULL;
    return mrb_nil_value();
};

static mrb_value disposed(mrb_state *mrb, mrb_value self) {
    return DATA_PTR(self) ? mrb_false_value() : mrb_true_value();
};

static mrb_value width(mrb_state *mrb, mrb_value self) {
    mrgss_pixmap *pixmap = DATA_PTR(self);
    return mrb_fixnum_value(pixmap->width);
}

static mrb_value height(mrb_state *mrb, mrb_value self) {
    mrgss_pixmap *pixmap = DATA_PTR(self);
    return mrb_fixnum_value(pixmap->height);
}

static mrb_value get_pixel(mrb_state *mrb, mrb_value self) {
    mrb_int x, y;
    mrb_int r, g, b, a;
    uint32_t *pixels, pixel;
    mrgss_pixmap* pixmap;
    pixmap = DATA_PTR(self);
    if (mrgss_is_disposable(mrb, self)) {
        mrb_get_args(mrb, "ii", &x, &y);
        if (x < 0 || y < 0 || x >= pixmap->width || y >= pixmap->height) {
            return mrgss_color_new(mrb, 0, 0, 0, 255);
        }
        pixels = (uint32_t*) pixmap->data;
        pixel = pixels[x % pixmap->width + y * pixmap->width];
        a = (pixel) & 0xff;
        b = (pixel >> 8) & 0xff;
        g = (pixel >> 16) & 0xff;
        r = (pixel >> 24) & 0xff;
        return mrgss_color_new(mrb, r, g, b, a);
    } else {
        mrgss_raise(mrb, E_RUNTIME_ERROR, "Pixmap Disposed");
        return mrb_nil_value();
    }
}

static mrb_value
set_pixel(mrb_state *mrb, mrb_value self) {
    mrb_int x, y;
    uint32_t *all_pixels, pixel;
    unsigned char r, g, b, a;
    mrb_value color;
    mrgss_pixmap *pixmap;
    pixmap = DATA_PTR(self);
    if (mrgss_is_disposable(mrb, self)) {
        mrb_get_args(mrb, "iio", &x, &y, &color);
        if (x < 0 || y < 0 || x >= pixmap->width || y >= pixmap->height) {
            return mrgss_color_new(mrb, 0, 0, 0, 255);
        }
        if (mrgss_object_is_a(mrb, color, "Color")) {
            all_pixels = (uint32_t*) pixmap->data;
            r = mrb_int(mrb, mrgss_iv_get(mrb, color, "@r"));
            g = mrb_int(mrb, mrgss_iv_get(mrb, color, "@g"));
            b = mrb_int(mrb, mrgss_iv_get(mrb, color, "@b"));
            a = mrb_int(mrb, mrgss_iv_get(mrb, color, "@a"));
            pixel = r | g << 8 | b << 16 | a << 24;
            all_pixels[x % pixmap->width + y * pixmap->width] = pixel;
            return mrgss_color_new(mrb, r, g, b, a);
        } else {
            mrgss_raise(mrb, E_ARGUMENT_ERROR, "Expecting a MRGSS::Color");
            return mrb_nil_value();
        }
    } else {
        mrgss_raise(mrb, E_RUNTIME_ERROR, "Pixmap Disposed");
        return mrb_nil_value();
    }
};

/**
 * Type initializer
 * @param mrb
 */
void mrgss_init_pixmap(mrb_state * mrb) {
    struct RClass *type = mrgss_create_class(mrb, "Pixmap");
    mrb_define_method(mrb, type, "initialize", initialize, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, type, "dispose", dispose, MRB_ARGS_NONE());
    mrb_define_method(mrb, type, "disposed?", disposed, MRB_ARGS_NONE());
    mrb_define_method(mrb, type, "width", width, MRB_ARGS_NONE());
    mrb_define_method(mrb, type, "height", height, MRB_ARGS_NONE());
    mrb_define_method(mrb, type, "get_pixel", get_pixel, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, type, "set_pixel", set_pixel, MRB_ARGS_REQ(3));
}
