#include <mruby.h>
#include <mruby/compile.h>
#include <mrgss/mrgss.h>
#include <mrgss/mrgss_pixmap.h>


void
mrb_mrgss_pixmap_gem_init(mrb_state *mrb) {
    mrgss_pixmap_init(mrb);
}

void
mrb_mrgss_pixmap_gem_final(mrb_state* mrb) {
    
}
