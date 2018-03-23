/*
 *
 * Copyright 2013 IPOL Image Processing On Line http://www.ipol.im/
 *
 * This file implements an algorithm possibly linked to the patents:
 *
 *  - US 5991456, "Method of improving a digital image," Issued Nov 23, 1999
 *  - US 6834125, "Method of improving a digital image as a function of its
 *  dynamic range," Issued Dec 21, 2004
 *  - US 6842543 B2, "Method of improving a digital image having white
 *  zones," Issued Jan 11, 2005
 *  - US 8111943, "Smart Image Enhancement Process," Issued Feb 7, 2012
 *  - EP 0901671, "Method of improving a digital image,"
 *  Issued September 3, 2003
 *  - AUS 713076, "Method of improving a digital image,"
 *  Issued February 26, 1998
 *  - WO 1997045809 A1, "Method of improving a digital image," July 4, 2006
 *  - JPO 4036391 B2, "Method of improving a digital image"
 *
 * This file is made available for the exclusive aim of serving as
 * scientific tool to verify the soundness and completeness of the
 * algorithm description. Compilation, execution and redistribution of
 * this file may violate patents rights in certain countries. The
 * situation being different for every country and changing
 * over time, it is your responsibility to determine which patent rights
 * restrictions apply to you before you compile, use, modify, or
 * redistribute this file. A patent lawyer is qualified to make this
 * determination. If and only if they don't conflict with any patent
 * terms, you can benefit from the following license terms attached to this
 * file.
 *
 */



/**
 * @file MSR_original.cpp
 * @brief Multiscale Retinex with color restoration
 *
 * Algorithm based on the original work of Jobson et al.
 * "A multiscale Retinex for bridging the gap between color images
 * and the human observations of scenes"
 *
 * Read/write operations (png format) make use
 * of io_png.c and io_png.h, by Nicolas Limare
 *
 * @author Catalina Sbert <catalina.sbert@uib.es/>
 * @author Ana Belén Petro <anabelen.petro@uib.es/>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "io_png.h"
#include "MSR_original_lib.h"
#include "parser.h"
#include "auxiliary_lib.h"

#define min(a,b) ((a) < (b) ? (a):(b))

using namespace std;

void print_min_max_single_ch(double *data, size_t image_size, char text[20])
{
    float minch, maxch;
    double *sortdata;

    sortdata = (double*) malloc(image_size*sizeof(double));

    memcpy(sortdata, data, image_size*sizeof(double));
    qsort(sortdata, image_size, sizeof sortdata[0], &myComparisonFunction);
    minch=sortdata[0];
    maxch= sortdata[image_size-1];
    std::cout << text << " range: (" << minch << ", " << maxch << ")\n";
}

void print_min_max_3ch_indep(double *R, double *G, double *B, size_t image_size, char text[50])
{
    float minch[3], maxch[3];
    double *sortdata;
    double *data;
    int c;

    sortdata = (double*) malloc(image_size*sizeof(double));
    
    for(c=0; c<3; c++){
        if (c == 0)
            data = R;
        if (c == 1)
            data = G;
        if (c == 2)
            data = B;
        
        memcpy(sortdata, data, image_size*sizeof(double));
        qsort(sortdata, image_size, sizeof sortdata[0], &myComparisonFunction);
        minch[c] = sortdata[0];
        maxch[c] = sortdata[image_size-1];
    }
    std::cout << text << " ranges: R(" << minch[0] << ", " << maxch[0] << ") "<< "G(" << minch[1] << ", " << maxch[1] << ")" << " B(" << minch[2] << ", " << maxch[2] << ")\n";
}

int main(int argc,  char **argv)
{


    int nscales, channels;
    double w, scale[3];
    unsigned char *data_in, *data_outC, *data_outG;
    size_t nx,ny, image_size, nc;
    double *R, *G,*B;
    double *Rout, *Gout, *Bout;
    double *gray, *grayout;
    float s1, s2;
    unsigned char option;
    double gamma;
    int wb_method;
    //float A[3] = {256.0, 256.0, 256.0};
    float A[3];


    std::vector <OptStruct *> options;
    OptStruct oS  = {"S:", 0, "3", NULL, "number of scales. if scales number is 1 then low scale is the scale to use with the value you want. If the number of scales is 2 then low and medium scales are used"};
    options.push_back(&oS);
    OptStruct oL  = {"L:", 0, "15", NULL, "Low scale"};
    options.push_back(&oL);
    OptStruct oM  = {"M:", 0, "80", NULL, "Medium scale"};
    options.push_back(&oM);
    OptStruct oH  = {"H:", 0, "250",NULL, "High scale"};
    options.push_back(&oH);
    OptStruct oN  = {"N:", 0, "1", NULL, "If 0 final 'canonical' gain/offset; if 1 with final simplest color balance; if 2 Moore's translation to display domain"};
    options.push_back(&oN);
    OptStruct ol  = {"l:", 0, "1", NULL, "percentage of saturation on the left (simplest color balance)"};
    options.push_back(&ol);
    OptStruct oR  = {"R:", 0, "1", NULL, "percentage  of saturation on the right (simplest color balance)"};
    options.push_back(&oR);
    OptStruct og  = {"g:", 0, "2.2", NULL, "Reference value of gamma. Will be applied as I^(g) (linearize) after loading and as I^(1/g) (unlinearize, lighten) before saving the image. g=1.0 means no change."};
    options.push_back(&og);
    OptStruct oW  = {"W:", 0, "1", NULL, "White balance method applied to input before inversion and after gamma linearization. 0: None, 1: SCB_l=0.0_R=0.0"};
    options.push_back(&oW);
    OptStruct oA  = {"A:", 0, "255", NULL, "R component of A in [0, 255] range"};
    options.push_back(&oA);
    OptStruct oB  = {"B:", 0, "255", NULL, "G component of A in [0, 255] range"};
    options.push_back(&oB);
    OptStruct oC  = {"C:", 0, "255", NULL, "B component of A in [0, 255] range"};
    options.push_back(&oC);

    std::vector<ParStruct *> pparameters;
    ParStruct pinput = {"input", NULL, "input file"};
    pparameters.push_back(&pinput);
    ParStruct pMSR_channels= {"MSR_channels", NULL,
                              "Multiscale retinex on rgb channels"
                             };
    pparameters.push_back(&pMSR_channels);
    ParStruct pMSR_gray = {"MSR_gray", NULL,
                           "Multiscale retinex on the gray "
                          };
    pparameters.push_back(&pMSR_gray);

    if (!parsecmdline("MSR_original","Multiscale Retinex with color restoration",
                      argc, argv, options, pparameters))
        return EXIT_FAILURE;
    

    /* read the PNG input image and the options parameters*/

    nscales=atoi(oS.value);

    if(nscales <= 0 || nscales > 3)
    {
        printf("nscales must be 1, 2 or 3\n");
        return EXIT_FAILURE;
    }
    else if(nscales == 1) scale[0]=atof(oL.value);
    else if(nscales == 2)
    {
        scale[0]=atof(oL.value);
        scale[1]=atof(oM.value);
    }
    else
    {
        scale[0]=atof(oL.value);
        scale[1]=atof(oM.value);
        scale[2]=atof(oH.value);
    }

    option=(unsigned char) atoi(oN.value);

    s1=atof(ol.value);
    s2=atof(oR.value);
    
    gamma = atof(og.value);
    wb_method = atoi(oW.value);
    A[0] = atof(oA.value);
    A[1] = atof(oB.value);
    A[2] = atof(oC.value);
    printf("A:(%f, %f, %f)\n", A[0], A[1], A[2]);
    data_in=io_png_read_u8(pinput.value, &nx, &ny,&nc);

    if(nc >= 3) channels=3;
    else channels=1;

    image_size=nx*ny;
    
    
    /* memory for the output*/
    
    R=(double*) malloc(image_size*sizeof(double));
    G=(double*) malloc(image_size*sizeof(double));
    B=(double*) malloc(image_size*sizeof(double));
    Rout=(double*) malloc(image_size*sizeof(double));
    Gout=(double*) malloc(image_size*sizeof(double));
    Bout=(double*) malloc(image_size*sizeof(double));
    gray=(double*) malloc(image_size*sizeof(double));
    grayout=(double*) malloc(image_size*sizeof(double));
    data_outC=(unsigned char*)malloc(3*image_size*sizeof(unsigned char));
    data_outG=(unsigned char*)malloc(3*image_size*sizeof(unsigned char));

    
    /* define the three color channels and intensity as double arrays with a translation of 1 for the log function*/
    /*  aitor: data_in is u8 (unsigned char) here */
    input_rgb(data_in, R, G, B, image_size);
    /* aitor: R, G, B are double here  */
    
    printf("Applying offset, rescaling and gamma_linearize_darken to input image with gamma_ref=%f\n", gamma);
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_offset_scale_to_0-1");
    offset_and_scale_float_img(R, R, image_size, -1, 1.0/255);
    offset_and_scale_float_img(G, G, image_size, -1, 1.0/255);
    offset_and_scale_float_img(B, B, image_size, -1, 1.0/255);
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_post_offset_scale_to_0-1");
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_gamma_linearize");
    gamma_linearize_darken_for_processing(R, R, image_size, gamma); //we use 2.2 here and not 1/2.2 because the inversion is done internally before calling apply_gamma_to_img()
    gamma_linearize_darken_for_processing(G, G, image_size, gamma); 
    gamma_linearize_darken_for_processing(B, B, image_size, gamma); 
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_post_gamma_linearize");
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_offset_scale_to_1-256");
    scale_and_offset_float_img(R, R, image_size, 255, 1);
    scale_and_offset_float_img(G, G, image_size, 255, 1);
    scale_and_offset_float_img(B, B, image_size, 255, 1);
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_post_offset_scale_to_1-256");
    
    
//#if 0
    /*  aitor */
    int A_offset_input; // offset to be added to A just before inversion
    switch(wb_method) {
        case 0: //no WB, useful if A != (255, 255, 255)
        {
            A_offset_input = 2;  // so that we then invert by doing 255+2-[1,256] = [1,256]
            break;
        }
        case 1: // SCB_l0.0_R0.0
        {
            float s0=0;
            print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_SCB_takes_1-256");
            printf("Applying the Simplest Color Balance BEFORE MSR with saturation values (%.1f, %.1f)\n", s0, s0);
            simplest_color_balance(R, R, image_size, s0, s0);
            simplest_color_balance(G, G, image_size, s0, s0);
            simplest_color_balance(B, B, image_size, s0, s0);
            print_min_max_3ch_indep(R, G, B, image_size, "RGB_post_SCB_produces_0-255");
           
            A_offset_input = 1;  // so that we then invert by doing 255+1-[0,255] = [1,256]
            break;
        }
        default:
        {
            exit(-1);
        }
    }
    /*  \aitor */
//#endif

    print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_input_inversion_takes_0-255");
    /* In the white case, A=255, 255, 255. and we keep it like this. Modify it dynamically when inverting, depending on what we need. We now need [1,256] */
    printf("Inverting the input image with A=(%.1f, %.1f, %.1f)\n", A[0]+A_offset_input, A[1]+A_offset_input, A[2]+A_offset_input);
    for(unsigned int p=0;p<image_size;p++){
    	R[p] = A[0]+A_offset_input - R[p];
    	G[p] = A[1]+A_offset_input - G[p];
    	B[p] = A[2]+A_offset_input - B[p];
    	//R[p] = max(0.0, A[0]+A_offset_input - R[p]);
    	//G[p] = max(0.0, A[1]+A_offset_input - G[p]);
    	//B[p] = max(0.0, A[2]+A_offset_input - B[p]);
	}   
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_post_input_inversion_produces_1-256");
    gray_intensity(gray, R, G, B, image_size);
    /*  now R g B in [1,256] */ 
    
    /* weight of each scale*/

    w=(double)(1./nscales);



    /* MSR on RGB channels*/
    /* inputs are double, outputs are double */
    print_min_max_3ch_indep(R, G, B, image_size, "RGB_pre_MSR_takes_1-256");
    printf("Applying the MSRetinex to the 3ch independently \n");
    MSRetinex( Rout,  R,  scale, nscales, w, nx,ny);
    MSRetinex( Gout,  G,  scale, nscales, w, nx,ny);
    MSRetinex( Bout,  B,  scale, nscales, w, nx,ny);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_MSR_produces_unnormalized_range");

    /* Color restoration  */

    /* inputs are double, outputs are double */
#if 0
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_CR");
    printf("Applying the Color Restoration step\n");
    Color_Restoration(Rout, R, gray, image_size);
    Color_Restoration(Gout, G, gray, image_size);
    Color_Restoration(Bout, B, gray, image_size);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_CR");
:w
#endif

//#if 0

    int A_offset_output; // to be set differently depending on option (N)
    
    double * RGBout;
    RGBout = (double*)malloc(3*image_size*sizeof(double));
    switch(option){
        case 0:/* Gain/Offset*/
        {
            /* inputs are double, outputs are double */
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_gain_offset");
            printf("Applying the Gain/Offset approach, NOT the Simplest Color Balance\n");
            Gain_offset(Rout, Rout, 30, -6, image_size);
            Gain_offset(Gout, Gout, 30, -6, image_size);
            Gain_offset(Bout, Bout, 30, -6, image_size);
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_gain_offset");
            A_offset_output = 0; //???
            break;
        }
        case 1:
            /* Simplest color balance with s1% and s2% of saturation*/
        {
            /* inputs are double, outputs are double */
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_SCB");
            printf("Applying the Simplest Color Balance AFTER MSR with saturation values (%.1f, %.1f), NOT the Gain/Offset approach\n", s1, s2);
            simplest_color_balance(Rout, Rout, image_size, s1, s2);
            simplest_color_balance(Gout, Gout, image_size, s1, s2);
            simplest_color_balance(Bout, Bout, image_size, s1, s2);
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_SCB");
            A_offset_output = 0; //???
            break;
        }
//#endif
        case 2:
        {
        //#if 0
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_moore");
            printf("Applying the Moore's Retinex's Output to Display Domain translation (Moore) with saturation values (%.1f, %.1f)\n", s1, s2);
            merge_rgb(Rout, Gout, Bout, RGBout, image_size);
            retinex_output_to_display_domain_moore(RGBout, RGBout, 3*image_size, s1, s2);
            split_rgb(RGBout, Rout, Gout, Bout, image_size);
            print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_moore");
            A_offset_output = 1; // because moore keaves result in [1, 256]
            break;
    //#endif
        }
    default:
        printf("ERROR: NO VALID TO DISPLAY DOMAIN METHOD PROVIDED");
        exit(-1);
    }
    /*write the output*/

    //rgb_output(Rout, Gout, Bout, data_outC, image_size);
    //io_png_write_u8(pMSR_channels.value, data_outC, nx,ny, nc);
    

    /* MSR on the gray*/
    //MSRetinex(grayout, gray, scale, nscales, w, nx, ny);
    //simplest_color_balance(grayout, grayout, image_size, s1, s2);
    //compute_color_from_grayscale(Rout, Gout, Bout, R, G, B, gray, grayout, image_size);
    
    /* invert the image back to the "non-negative" space */
    // Rout, Gout, Bout are now in [0, 255]
    //for(int i=0; i<=2; i++){
     //   A[i] -= 1;
    //}
    printf("Inverting the output image with A=(%.1f, %.1f, %.1f)\n", A[0]+A_offset_output, A[1]+A_offset_output, A[2]+A_offset_output);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_output_inversion");
    for(unsigned int p=0; p<image_size; p++){
    	//Rout[p] = max(0.0, A[0]+A_offset_output - Rout[p]);
    	//Gout[p] = max(0.0, A[1]+A_offset_output - Gout[p]);
    	//Bout[p] = max(0.0, A[2]+A_offset_output - Bout[p]);
	    Rout[p] = A[0]+A_offset_output - Rout[p];
	    Gout[p] = A[1]+A_offset_output - Gout[p];
	    Bout[p] = A[2]+A_offset_output - Bout[p];
	}   
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_output_inversion");
    
//#if 0
    /* unlinearize gamma */
    printf("Applying offset, rescaling and gamma_unlinearize_lighten to output image with gamma_ref=%f\n", gamma);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_offset_scale_to_0-1");
    offset_and_scale_float_img(Rout, Rout, image_size, 0, 1.0/255);
    offset_and_scale_float_img(Gout, Gout, image_size, 0, 1.0/255);
    offset_and_scale_float_img(Bout, Bout, image_size, 0, 1.0/255);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_offset_scale_to_0-1");
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_gamma_unlinearize");
    gamma_unlinearize_lighten_for_storage_and_display(Rout, Rout, image_size, gamma);
    gamma_unlinearize_lighten_for_storage_and_display(Gout, Gout, image_size, gamma); 
    gamma_unlinearize_lighten_for_storage_and_display(Bout, Bout, image_size, gamma); 
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_gamma_unlinearize");
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_pre_scale_offset_to_0-255");
    scale_and_offset_float_img(Rout, Rout, image_size, 255, 0);
    scale_and_offset_float_img(Gout, Gout, image_size, 255, 0);
    scale_and_offset_float_img(Bout, Bout, image_size, 255, 0);
    print_min_max_3ch_indep(Rout, Gout, Bout, image_size, "RGB_post_scale_offset_to_0-255");
//#endif    
    
    /*write the rgb output*/
    /* inputs are double, outputs unsigned char */
    rgb_output(Rout, Gout, Bout, data_outC, image_size);
    io_png_write_u8(pMSR_channels.value, data_outC, nx,ny, channels);  // channels used to be nc instead, but did not handle 4-channel pngs correctly (alpha was affected! workaround: write just three channels)
    
    /*write the gray output*/
	//rgb_output(Rout, Gout, Bout, data_outG, image_size);
    //io_png_write_u8(pMSR_gray.value, data_outG, nx,ny, nc);
    

    /*Free the memory*/
    
    free(R); free(G); free(B);
    free(Rout); free(Gout); free(Bout);
    free(gray); free(grayout);
    free(data_outG); free(data_outC);
    free(RGBout); 
    return EXIT_SUCCESS;

}

