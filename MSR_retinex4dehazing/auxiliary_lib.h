/*
 * auxiliary_lib.h
 *  
 *
 *  Created by Catalina Sbert Juan on 12/07/13.
 *
 * Copyright 2013 IPOL Image Processing On Line http://www.ipol.im/
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _AUXILIARY_LIB_H
#define _AUXILIARY_LIB_H

#ifdef __cplusplus
extern "C" {
#endif
    
    double *input_rgb(unsigned char *data_in, double *R, double *G, double *B, size_t size);
    
    unsigned char *rgb_output(double *R, double *G, double *B, unsigned char *output, size_t size);
    
    double *gray_intensity(double *gray, double *R, double *G, double *B, size_t dim);
    
    double *simplest_color_balance(double *data_out, double *data, size_t dim, float s1,float s2);
    
    double *compute_color_from_grayscale(double *R, double *G, double *B, double *RO, double *GO , double *BO,double *gray, double *gray1, size_t dim);

    int myComparisonFunction(const void *x, const void *y);

    double *retinex_output_to_display_domain_moore(double *data_out, double *data, size_t image_size, float s1,float s2);
    double *merge_rgb(double *R, double *G, double *B, double *output, size_t image_size);
    double *split_rgb(double *input, double *R, double *G, double *B, size_t size);
    double *apply_gamma_to_img(double *data_out, double *data_in, size_t image_size, double gamma);
    double *gamma_unlinearize_lighten_for_storage_and_display(double *data_out, double *data_in, size_t image_size, double gamma_ref);
    double *gamma_linearize_darken_for_processing(double *data_out, double *data_in, size_t image_size, double gamma_ref);
    double *offset_and_scale_float_img(double *data_out, double *data_in, size_t image_size, double offset, double scale);
    double *scale_and_offset_float_img(double *data_out, double *data_in, size_t image_size, double scale, double offset);
    
    
#ifdef __cplusplus
}
#endif

#endif /* _AUXILIARY_LIB_H */
