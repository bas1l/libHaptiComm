/*
 * =====================================================================================
 *
 *       Filename:  main.cc
 *
 *    Description:  This is our main file.
 *
 *        Version:  1.0
 *        Created:  Sunday 05 September 2010 09:44:39  IST
 *       Revision:  none
 *       Compiler:  g++/gcc
 *
 *         Author:  Dilawar (nuts), dilawar[AT]ee[dot]iitb[dot]ac[dot]in
 *      Institute:  Indian Institute of Technology, Bombay
 *
 * This material is released under GNU Lesser Public License.
 * You are free to copy, distribute or use it for any non-commercial activity.
 * But you are not allowed to modify it. If you are a student, you can use its
 * part in your work with or without mentioning it.
 *
 * For additional details, please see the GNU Lesser Public license.
 *
 * NOTE : No propriety software is used in this material.
 * Almost all of the code is written and modified in VIM editor with c-support
 * plugin which is more awesome than Kung Fu Panda. Just kidding, no one is more
 * awesome than Kung Fu Panda with or without a light saber.
 *
 * This program is made using a bit for here,  a bit from there under the influence
 *  of a lot of burnt out neurons.
 * Report bugs : dilawar.in@gmail.com
 * =====================================================================================
 */
#define EXIT_SUCCESS    1
#define EXIT_FAILURE    0
#define TRUE            1
#define FALSE           0

#include  <iostream>
#include  <getopt.h>
#include  <stdlib.h>
#include  <stdio.h>
#include  <string.h>

#include  "wav_file.h"

int main(int argc, char** argv)
{
    /* Here, we parse our argument passed to this application from command line*/
    if(argc < 2)
    {
        std::cout<<"\nOh Snap!"
                 <<"\nWassup, Fellow Human.."
                 <<"\nAt least, you need to specify wav file. Use --help to see your options.\n";
        return EXIT_FAILURE;
    }
    int c;
    int do_help = 0;
    int do_verbose = 0;
    bool ifInvalid = FALSE;	/* flag variables. */
    char inputFile[80];
    while(1)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"file", required_argument, NULL, 'f'},
            {"help", no_argument, &do_help, 1},
            {"verbose", no_argument, &do_verbose, 1},
            {0, 0, 0 , 0}
        };

        c = getopt_long(argc, argv, "f:hv", long_options, &option_index);
        if (c == -1)
            break;

        switch(c)
        {
            case 'f':
            {
                strcpy(inputFile, optarg);
                break;
            }

            case 'v':
            {
                do_verbose = 1;
                break;
            }

            case 'h':
            {
                do_help = 1;
                break;
            }

            case ':':
            {
                fprintf(stderr, " option %s requires an argument. \n", argv[1]);
                ifInvalid = TRUE;
                break;
            }

            default:
            {
                fprintf(stderr, "option %s is invalid : ignored\n",
                        argv[1]);
                ifInvalid = TRUE;
                break;
            }
        }

    }

    /* We got the options. Do the stuff. */
    if(TRUE == ifInvalid || 1 == do_help)
    {
        std::cout<<"\n-------------------------------------------------------";
        std::cout<<"\nUSAGE :";
        std::cout<<"\n\t --file <name>"
                 <<"\n\t\t provide the wav file."
                 <<"\n\t --help"
                 <<"\n\t\t to see the help. Should be used alone."
                 <<"\n\t --verbose"
                 <<"\n\t\t for verbose output.";
        std::cout<<"\n-------------------------------------------------------\n";
        return EXIT_FAILURE;
    }

    
    WavFile* pObjWavFile;
    printf("Input file : %s", inputFile);
    pObjWavFile = new WavFile;
    if (EXIT_SUCCESS != pObjWavFile->openWavFile(inputFile))
    {
        std::cout<<"\nCan't load wav file.";
        exit(-1);
    }

    pObjWavFile->displayInformation(inputFile);

    /* write data in a file. */
    char outfile[80] = {};
    strcat( outfile, inputFile);
    strcat( outfile, ".csv");
    pObjWavFile->writeDataToFile( outfile );
    return 0;
}
