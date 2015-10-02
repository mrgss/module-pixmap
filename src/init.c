#include <mruby.h>
#include <mruby/compile.h>
#include <mrgss.h>
#include <mrgss/mrgss-pixmap.h>


void
mrb_mrgss_pixmap_gem_init(mrb_state *mrb) {
    mrgss_init_pixmap(mrb);
}

void
mrb_mrgss_pixmap_gem_final(mrb_state* mrb) {
    
}