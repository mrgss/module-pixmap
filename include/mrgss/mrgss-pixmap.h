/* 
 * File:   mrgss-pixmap.h
 * Author: manuel
 *
 * Created on 27 de septiembre de 2015, 17:23
 */

#ifndef MRGSS_PIXMAP_H
#define	MRGSS_PIXMAP_H

#ifdef	__cplusplus
extern "C" {
#endif
    void mrb_mrgss_pixmap_gem_init(mrb_state *mrb);
    typedef struct pixmap {
        mrb_int width;
        mrb_int height;
        unsigned char *data;
    } mrgss_pixmap;

    void mrgss_init_pixmap(mrb_state * mrb);
#ifdef	__cplusplus
}
#endif

#endif	/* MRGSS_PIXMAP_H */

