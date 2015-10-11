#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <mruby.h>
#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mrgss/mrgss.h>

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static int rmask = 0xff000000;
static int gmask = 0x00ff0000;
static int bmask = 0x0000ff00;
static int amask = 0x000000ff;
#else
static int rmask = 0x000000ff;
static int gmask = 0x0000ff00;
static int bmask = 0x00ff0000;
static int amask = 0xff000000;
#endif

Uint32 mrgss_get_pixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(Uint16 *) p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(Uint32 *) p;
            break;

        default:
            return 0; /* shouldn't happen, but avoids warnings */
    }
}

void mrgss_set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16 *) p = pixel;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *) p = pixel;
            break;
    }
}

static void pixmap_free(mrb_state *mrb, void *p) {
    if (p) {
        SDL_Surface* bmp = (SDL_Surface*) p;
        SDL_FreeSurface(bmp);
    }
}
/**
 * free function structure
 */
struct mrb_data_type const mrbal_bitmap_data_type = {"Pixmap", pixmap_free};

/**
 *  Pixmap mruby Constructor
 */
static mrb_value initialize(mrb_state *mrb, mrb_value self) {
    SDL_Surface* surface;
    mrb_value param1;
    mrb_int height, count;
    DATA_TYPE(self) = &mrbal_bitmap_data_type;
    DATA_PTR(self) = NULL;
    count = mrb_get_args(mrb, "o|i", &param1, &height);
    switch (count) {
        case 1:
            if (mrb_string_p(param1)) {
                const char *str = mrb_string_value_ptr(mrb, param1);
                surface = IMG_Load(str);
                if (surface == NULL) {
                    printf("Oh My Goodness, an error : %s", IMG_GetError());
                }
            } else {
                mrb_raise(mrb, E_ARGUMENT_ERROR, "Wrong type of arguments");
                return mrb_nil_value();
            }

            break;
        case 2:
            surface = SDL_CreateRGBSurface(0, mrb_int(mrb, param1), height, 32, rmask, gmask, bmask, amask);
            break;
        default:
            mrb_raise(mrb, E_ARGUMENT_ERROR, "Wrong number of arguments");
            return mrb_nil_value();
            break;
    }
    
    DATA_PTR(self) = surface;
    return self;
}

static mrb_value width(mrb_state *mrb, mrb_value self) {

    SDL_Surface *this = DATA_PTR(self);
    if (!this) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "The bitmap has been disposed");
        return mrb_nil_value();
    }
    return mrb_fixnum_value(this->w);
}

static mrb_value height(mrb_state *mrb, mrb_value self) {
    SDL_Surface *this = DATA_PTR(self);
    if (!this) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "The bitmap has been disposed");
        return mrb_nil_value();
    }
    return mrb_fixnum_value(this->h);
}

static mrb_value dispose(mrb_state *mrb, mrb_value self) {
    SDL_Surface *this = DATA_PTR(self);
    pixmap_free(mrb, this);
    DATA_PTR(self) = NULL;
    return mrb_nil_value();
}

static mrb_value disposed(mrb_state *mrb, mrb_value self) {
    return DATA_PTR(self) ? mrb_false_value() : mrb_true_value();
}

static mrb_value get_pixel(mrb_state *mrb, mrb_value self) {
    mrb_value params[4];
    mrb_int x, y;
    Uint8 r, g, b, a;
    SDL_Surface* this = DATA_PTR(self);
    if (!this) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "The bitmap has been disposed");
        return mrb_nil_value();
    }
    mrb_get_args(mrb, "ii", &x, &y);
    if (x < 0 || y < 0 || x >= this->w || y >= this->h) {
        r = 0;
        g = 0;
        b = 0;
        a = 255;
    } else {
        Uint32 pix = mrgss_get_pixel(this, x, y);
        SDL_GetRGBA(pix, this->format, &r, &g, &b, &a);
    }
    params[0] = mrb_fixnum_value((mrb_int) r);
    params[1] = mrb_fixnum_value((mrb_int) g);
    params[2] = mrb_fixnum_value((mrb_int) b);
    params[3] = mrb_fixnum_value((mrb_int) a);
    return mrb_obj_new(mrb, mrb_class_get_under(mrb, mrgss_module(), "Color"), 4, params);
}

static mrb_value set_pixel(mrb_state *mrb, mrb_value self) {
    mrb_int x, y;
    mrb_value color;
    SDL_Color *colour;
    Uint32 pixel;
    SDL_Surface *this = DATA_PTR(self);
    mrb_get_args(mrb, "oo", &x, &y, &color);
    colour = DATA_PTR(color);
    pixel = SDL_MapRGBA(this->format, colour->r, colour->g, colour->b, colour->a);
    mrgss_set_pixel(this, x, y, pixel);
    return color;
}

static mrb_value fill_rect(mrb_state *mrb, mrb_value self) {
    mrb_value color, rect;
    Uint32 pixel;
    SDL_Color *colour;
    SDL_Rect *rectc;
    SDL_Surface *this = DATA_PTR(self);
    mrb_get_args(mrb, "oo", &rect, &color);
    colour = DATA_PTR(color);
    rectc = DATA_PTR(rect);
    pixel = SDL_MapRGBA(this->format, colour->r, colour->g, colour->b, colour->a);
    SDL_FillRect(this, rectc, pixel);
    return self;
}

static mrb_value blit(mrb_state *mrb, mrb_value self) {
    SDL_Rect *src_rect;
    SDL_Point *origin;
    SDL_Surface *src_bmp;
    SDL_Rect dst_rect;
    SDL_Surface *this = DATA_PTR(self);
    mrb_value rect, bitmap, point;
    mrb_get_args(mrb, "ooo", &point, &bitmap, &rect);
    src_rect = DATA_PTR(rect);
    origin = DATA_PTR(point);
    dst_rect.x = origin->x;
    dst_rect.y = origin->y;
    dst_rect.w = src_rect->w;
    dst_rect.h = src_rect->h;
    src_bmp = DATA_PTR(bitmap);
    SDL_BlitSurface(src_bmp, src_rect, this, &dst_rect);
    return self;
}

static mrb_value scaled_blit (mrb_state *mrb, mrb_value self)
{
  SDL_Rect *src_rect;
  SDL_Rect *dst_rect;
  SDL_Surface *src_bmp;
  SDL_Surface *this = DATA_PTR (self);
  mrb_value srect, drect, bitmap;
  mrb_get_args (mrb, "ooo", &drect, &bitmap, &srect);
  src_rect = DATA_PTR (srect);
  dst_rect = DATA_PTR (drect);
  src_bmp = DATA_PTR (bitmap);
  SDL_BlitScaled (src_bmp, src_rect, this, dst_rect);
  return self;
}
/**
 * Initialize mruby class
 */
void mrgss_pixmap_init(mrb_state *mrb) {
    struct RClass *bitmap = mrb_define_class_under(mrb, mrgss_module(), "Pixmap", mrb->object_class);
    mrb_define_method(mrb, bitmap, "initialize", initialize, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_method(mrb, bitmap, "width", width, MRB_ARGS_NONE());
    mrb_define_method(mrb, bitmap, "height", height, MRB_ARGS_NONE());
    mrb_define_method(mrb, bitmap, "dispose", dispose, MRB_ARGS_NONE());
    mrb_define_method(mrb, bitmap, "disposed?", disposed, MRB_ARGS_NONE());
    mrb_define_method(mrb, bitmap, "get_pixel", get_pixel, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, bitmap, "set_pixel", set_pixel, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, bitmap, "fill_rect", fill_rect, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, bitmap, "blit", blit, MRB_ARGS_REQ(3));
    mrb_define_method(mrb, bitmap, "scaled_blit", scaled_blit, MRB_ARGS_REQ(3));
    MRB_SET_INSTANCE_TT(bitmap, MRB_TT_DATA);
}