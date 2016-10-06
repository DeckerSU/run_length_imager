/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 &	NOTICE:
 &	 May 5th, 2014
 &
 &	 makers_mark @ xda-developers.com
 &   http://forum.xda-developers.com/showthread.php?t=2764354
 &
 &	 Original source:
 &	 https://android.googlesource.com/platform/build/+/b6c1cf6de79035f58b512f4400db458c8401379a/tools/rgb2565/to565.c
 &	 Based off of the original to565.c program to convert raw rgb888 r=8 g=8 b=8 images
 &	 to, 2 byte count, 2 byte color run length encoded rgb565 r=5 g=6 b=5 files.
 &	 Mainly, if not always to my knowledge, used for creating initlogo.rle files for kernel splash
 &	 screens.
 &
 &	 Added decoding of 2, 3, and 4 byte rgb(x) patterns
 &	 Added encoding of 3 and 4 byte rgb(x) patterns
 &	 Added byte offsets and maximum pixel runs for decoding files not totally
 &	 encoded in a run length manner
 & Version 1.2 added:
 &   Jpeg extractor, to pull jpegs from any file or drive/device image
 &   A zero byte writer, to undo what ffmpeg does to rgb0, 0rgb, bgr0, and 0bgr pixel formats

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

unsigned int _CRT_fmode = _O_BINARY;
static char fileName[1024];
void zeroBytes(long long, long, char *);
void jpegExtract(char *, unsigned long long);
void headerToFooter(int);
int getFilename(char *);
void decode_rgb16_rle(unsigned int, unsigned long long int);
void decode_rgb24_rle(unsigned int, unsigned long long int);
void decode_rgbx32_rle(unsigned long int, unsigned long long int);
void encode_rgb16_rle(void);
void encode_rgb24_rle(void);
void encode_rgbx32_rle(void);

void zeroBytes(long long offset, long z, char *inputFile)
{
	int readByte;
	FILE *zeroStream;
	FILE *inStream;
	long long int cursorPosition;
	char outputFile[1024];

	sprintf(outputFile, "%s.zero", inputFile);
	if ((inStream = fopen(inputFile, "rb")) == NULL){
		fclose(inStream);
		return;
	}
	if ((zeroStream = fopen(outputFile, "wb")) == NULL){
		fclose(inStream);
		fclose(zeroStream);
		return;
	}
	fprintf(stderr,"\nZeroing every %ld bytes starting at %lld", z, offset);
	fprintf(stderr,"\nInput file: %s", inputFile);
	while ((readByte = fgetc(inStream)) != EOF){
		cursorPosition = ftell(inStream);
		if (((cursorPosition - offset) % z) == 0 && cursorPosition >= offset){
			fputc(0,zeroStream);
		}else{
			fputc(readByte,zeroStream);
		}
	}
	fclose(inStream);
	fclose(zeroStream);
}

void jpegExtract(char outname[1024], unsigned long long o)
{
	int readByte;
	FILE *fileStream;
	unsigned int header;

	if (o != 0){
		fseek(stdin, o, SEEK_SET);
	}
	while ((readByte = fgetc(stdin)) != EOF){
		if (readByte == 0xff){
			readByte = fgetc(stdin);
			if (readByte == 0xd8){
				readByte = fgetc(stdin);
				if (readByte == 0xff){
					readByte = fgetc(stdin);
						if (readByte == 0xe0 || readByte == 0xe1){
							header = 0x00ffd8ff | (readByte << 24);
							if (!getFilename(outname)){
								if ((fileStream = fopen(fileName, "wb")) == NULL){
									fclose(fileStream);
									return;
								}
								int streamNumber = fileno(fileStream);
								fprintf(stderr, "\n%s", fileName);
								write(streamNumber, &header, 4);
								headerToFooter(streamNumber);
								close(streamNumber);
								continue;
							}else{
								break;
							}
						}
				}
			}
		}
	}
	fclose(stdin);
}

void headerToFooter(int streamNumber)
{
	short notFinished = 1;
	int polarity = 1;
	int readByte;

	while((readByte = fgetc(stdin)) != EOF){
		if (readByte == 0xff){
			readByte = fgetc(stdin);
			if (readByte == EOF){
				readByte = 0xff;
				write(streamNumber, &readByte, 1);
				break;
			}else if (readByte == 0xd8){
				polarity += 1;
				readByte = 0xd8ff;
				write(streamNumber, &readByte, 2);
			}else if (readByte == 0xd9){
				polarity -= 1;
				readByte = 0xd9ff;
				write(streamNumber, &readByte, 2);
				if (polarity == 0){
					break;
				}
			}else{
				readByte = 0x00ff | (readByte << 8);
				write(streamNumber, &readByte, 2);
			}
		}else{
			write(streamNumber, &readByte, 1);
		}
	}
}

int getFilename(char outputbase[1024])
{
	unsigned int counter;
	FILE *stream;

	for (counter = 1; counter <= 99999; counter++){
		sprintf(fileName, "%s_%05d.jpg", outputbase, counter);
		if ((stream = fopen(fileName, "r+")) != NULL){
			fclose(stream);
			continue;
		} else {
			fclose(stream);
			return(0);
		}
	}
	fclose(stream);
	return(1);
}

void decode_rgb16_rle(unsigned int m, unsigned long long int o)
{
    unsigned short data[2], repeats;

	if (o != 0){
		fseek(stdin, o, SEEK_SET);
	}

    while(read(0,data,4) == 4){

		if (data[0] > m){
			continue;
		}
		for (repeats = 0; repeats < data[0]; repeats++){
			write(1, &data[1], 2);
		}
    }
}

void decode_rgb24_rle(unsigned int m, unsigned long long int o)
{

    unsigned char repeats, data[4];
	unsigned long color;

	if (o != 0){
		fseek(stdin, o, SEEK_SET);
	}

    while(read(0, data, 4) == 4){

		if (data[0] > m){
			continue;
		}
		color = ((data[1]) | (data[2] << 8) | (data[3]) << 16);
		for (repeats = 0;repeats < data[0]; repeats++){
			write(1, &color, 3);
		}
    }
}


void decode_bgr24_rle_qualcomm(unsigned int m, unsigned long long int o)
{

    unsigned char repeats, data[4];
	unsigned long color;
	unsigned char repeat_run;

	if (o != 0){
		fseek(stdin, o, SEEK_SET);
	}

    while(read(0, data, 4) == 4){

		repeat_run = data[0] & 0x80;
		
		
		if (repeat_run) {
			if (((data[0] & 0x7F) + 1) > m){
				continue;
			}
			
			// color = ((data[1]) | (data[2] << 8) | (data[3]) << 16);
			color = ((data[3]) | (data[2] << 8) | (data[1]) << 16);
			for (repeats = 0;repeats < ((data[0] & 0x7F) + 1); repeats++){
				write(1, &color, 3);
			}
		} else {
			
			// color = ((data[1]) | (data[2] << 8) | (data[3]) << 16);
			color = ((data[3]) | (data[2] << 8) | (data[1]) << 16);
			write(1, &color, 3);
			
			for (repeats = 0;repeats < (data[0] & 0x7F); repeats++){
				 read(0, &data[1], 3);
				 // color = ((data[1]) | (data[2] << 8) | (data[3]) << 16);
				 color = ((data[3]) | (data[2] << 8) | (data[1]) << 16);
				 
				 write(1, &color, 3);
			}
		}
    }
}

void decode_rgbx32_rle(unsigned long int m, unsigned long long int o)
{
	unsigned long repeats;
    unsigned long data[2];
	
	if (o != 0){
		fseek(stdin, o, SEEK_SET);
	}

    while(read(0, data, 8) == 8){
		if (data[0] > m){
			continue;
		}
		for (repeats = 0; repeats < data[0]; repeats++){
			write(1, &data[1], 4);
		}
    }
}

void encode_rgbx32_rle(void)
{
    unsigned long color, last, count;
    count = 0;
	while(read(0, &color, 4) == 4){
	    if (count){
            if ((color == last) && (count != 0xFFFFFFFF)){
                count++;
                continue;
            } else {
                write(1, &count, 4);
                write(1, &last, 4);
            }
        }
        last = color;
        count = 1;
    }
    if (count){
        write(1, &count, 4);
        write(1, &last, 4);
    }
}

void encode_rgb16_rle(void)
{
    unsigned short int last, color, count;

    count = 0;
    while(read(0, &color, 2) == 2){
        if (count){
            if ((color == last) && (count != 0xFFFF)){
                count++;
                continue;
            } else {
                write(1, &count, 2);
                write(1, &last, 2);
            }
        }
        last = color;
        count = 1;
    }
    if (count){
        write(1, &count, 2);
        write(1, &last, 2);
    }
}

void encode_rgb24_rle(void)
{
    unsigned char count;
    unsigned long int last, color;

	count = 0;
    while(read(0, &color, 3) == 3){
        if (count){
            if ((color == last) && (count != 0xFF)){
                count++;
                continue;
            } else {
                write(1, &count, 1);
                write(1, &last, 3);
            }
        }
        last = color;
        count = 1;
    }
    if (count){
        write(1, &count, 1);
        write(1, &last, 3);
    }
}

int usage(void){
	fprintf(stderr, "\n\n\nUsage:\n\nrlimager.exe ([-e] [2-4] | [-d] [2-4] [-m] [max run] [-o] [offset]) < input_file > output_file\n\n");
	fprintf(stderr, "Mandatory, one or the other\n\n");
	fprintf(stderr, "-d (2-4)		Run Length Decode input_file from 2, 3, or 4 byte color pattern\n");
	fprintf(stderr, "-d (5)			Decode input_file from RLE encoded RAW BGR24 format // [+] Decker\n");
	fprintf(stderr, "-e (2-4)		Run Length Encode input_file to 2, 3, or 4 byte color pattern\n");
	fprintf(stderr, "-j (output root name)	Extract Jpegs from file.  Output name can include a full path.\n");
	fprintf(stderr, "-z (skip) -o (offset)	Zero every (skip) bytes, starting with (offset)\n\n");
	fprintf(stderr, "Optional for [-d] decoding only:\n\n");
	fprintf(stderr, "-m (max run)	Maximum pixel run to decode. Default is 0, which defaults to the maximum allowable for each color pattern\n");
	fprintf(stderr, "-o (offset)	Offset (in bytes) to start decoding.  Default is 0\n");
	fprintf(stderr, "\n\nExamples:\n\nrlimager1.2 -d 4 -m 8064 < \"C:\\example_file.rle\" > \"C:\\output\\rle_decoded\\example_file.rgb0\"\n");
	fprintf(stderr, "rlimager1.2 -j root_name < \"C:\\users\\downloads\\system.img\"\n\n");
	fprintf(stderr, "Notice with the jpeg extractor example above you have to use: < \"inputfile\"\n\n");
	fprintf(stderr, "rlimager1.2 -z 8 -o 5 -i \"C:\\output\\example_file.rle\" -e 4 < \"C:\\example_file.rgb0\" > \"C:\\output\\example_file.rle\"\n\n");
	fprintf(stderr, "The above example rle encodes the input file, then takes the output file and zeros every eighth byte starting at 5\n");
	fprintf(stderr, "You can also zero byte any file without doing it all on one line, like:\n\n");
	fprintf(stderr, "rlimager1.2 -z 5 -o 8 -i \"C:\\file_to_be_zeroed_every_5th_byte_strarting_with_the_8th.raw\"\n");
	return(1);
}

int main(int argc, char **argv)
{
	unsigned int decode_opt = 0, encode_opt = 0;
	unsigned long long int maxrun = 0;
	long long int offset = 0;
	long zeroByte = 0;
	short jflag = 0;
	char *d_string, *e_string, *m_string, *o_string, *j_string, *z_string, *inputFile;
	int c;

	while ((c = getopt (argc, argv, "i:z:j:m:o:e:d:")) != -1)
		switch(c)
			{

			case 'i':
				inputFile = optarg;
				break;

			case 'z':
				z_string = optarg;
				zeroByte = atol(z_string);
				break;

			case 'j':
				jflag = 1;
				j_string = optarg;
				break;

			case 'm':
				m_string = optarg;
				maxrun = atoll(m_string);
				break;

			case 'o':
				o_string = optarg;
				offset = atoll(o_string);
				break;

			case 'e':
				e_string = optarg;
				encode_opt = atoi(e_string);
				break;

			case 'd':
				d_string = optarg;
				decode_opt = atoi(d_string);
				break;
	}

	if ((encode_opt > 1) && (encode_opt < 5)){

			if (maxrun != 0 || decode_opt != 0){
				usage();
				return(1);
			}

			if (encode_opt == 2){
				encode_rgb16_rle();
				return(0);
			}

			if (encode_opt == 3){
				encode_rgb24_rle();
			}

			if (encode_opt == 4){
				encode_rgbx32_rle();
			}

			if (zeroByte){
				zeroBytes(offset, zeroByte, inputFile);
			}
			return(0);
	} else if (zeroByte){
			zeroBytes(offset, zeroByte, inputFile);
			return(0);

	} else if ((decode_opt > 1) && (decode_opt < 6)){

			if (encode_opt != 0){
				usage();
				return(1);
			}

			if (decode_opt == 2){
					if (maxrun == 0){
						maxrun = 0xFFFF;
					}
				fprintf(stderr, "Maximum pixel run set at %d\n",maxrun);
				decode_rgb16_rle(maxrun, offset);
				return(0);
			}

			if (decode_opt == 3){
					if (maxrun == 0){
						maxrun = 0xFF;
					}
				fprintf(stderr, "Maximum pixel run set at %d\n",maxrun);
				decode_rgb24_rle(maxrun, offset);
				return(0);
			}

			if (decode_opt == 4){
					if (maxrun == 0){
						maxrun = 0xFFFFFFFF;
					}
				fprintf(stderr, "Maximun pixel run set at %lld\n",maxrun);
				decode_rgbx32_rle(maxrun, offset);
				return(0);
			}
			
			if (decode_opt == 5){
					if (maxrun == 0){
						maxrun = 0xFF;
					}
				fprintf(stderr, "Maximum pixel run set at %d\n",maxrun);
				decode_bgr24_rle_qualcomm(maxrun, offset);
				return(0);
			}

			return(0);
	} else if (jflag){
		jpegExtract(j_string, offset);
		} else usage();

return(1);
}

