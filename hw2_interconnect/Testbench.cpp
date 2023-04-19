#include <cassert>
#include <cstdio>
#include <cstdlib>
using namespace std;
#include "time.h"
#include "Testbench.h"
#include "tlm_utils/tlm_quantumkeeper.h"

unsigned char header[54] = {
    0x42,          // identity : B
    0x4d,          // identity : M
    0,    0, 0, 0, // file size
    0,    0,       // reserved1
    0,    0,       // reserved2
    54,   0, 0, 0, // RGB data offset
    40,   0, 0, 0, // struct BITMAPINFOHEADER size
    0,    0, 0, 0, // bmp width
    0,    0, 0, 0, // bmp height
    1,    0,       // planes
    24,   0,       // bit per pixel
    0,    0, 0, 0, // compression
    0,    0, 0, 0, // data size
    0,    0, 0, 0, // h resolution
    0,    0, 0, 0, // v resolution
    0,    0, 0, 0, // used colors
    0,    0, 0, 0  // important colors
};
Testbench::Testbench(sc_module_name n)
  : sc_module(n), initiator("initiator"), output_rgb_raw_data_offset(54) {
  SC_THREAD(do_sobel);
}

int Testbench::read_bmp(string infile_name) {
  FILE *fp_s = NULL; // source file handler
  fp_s = fopen(infile_name.c_str(), "rb");
  if (fp_s == NULL) {
    printf("fopen %s error\n", infile_name.c_str());
    return -1;
  }
  // move offset to 10 to find rgb raw data offset
  fseek(fp_s, 10, SEEK_SET);
  assert(fread(&input_rgb_raw_data_offset, sizeof(unsigned int), 1, fp_s));

  // move offset to 18 to get width & height;
  fseek(fp_s, 18, SEEK_SET);
  assert(fread(&width, sizeof(unsigned int), 1, fp_s));
  assert(fread(&height, sizeof(unsigned int), 1, fp_s));

  // get bit per pixel
  fseek(fp_s, 28, SEEK_SET);
  assert(fread(&bits_per_pixel, sizeof(unsigned short), 1, fp_s));
  bytes_per_pixel = bits_per_pixel / 8;

  // move offset to input_rgb_raw_data_offset to get RGB raw data
  fseek(fp_s, input_rgb_raw_data_offset, SEEK_SET);

  source_bitmap =
      (unsigned char *)malloc((size_t)width * height * bytes_per_pixel);
  if (source_bitmap == NULL) {
    printf("malloc images_s error\n");
    return -1;
  }

  temp_bitmap =
      (unsigned char *)malloc((size_t)width * 4 * bytes_per_pixel);
  if (temp_bitmap == NULL) {
    printf("malloc images_s error\n");
    return -1;
  }
  temp2_bitmap =
      (unsigned char *)malloc((size_t)width * height * bytes_per_pixel);
  if (temp2_bitmap == NULL) {
    printf("malloc target_bitmap error\n");
    return -1;
  }


  target_bitmap =
      (unsigned char *)malloc((size_t)width * height * bytes_per_pixel);
  if (target_bitmap == NULL) {
    printf("malloc target_bitmap error\n");
    return -1;
  }

  printf("Image width=%d, height=%d\n", width, height);
  assert(fread(source_bitmap, sizeof(unsigned char),
               (size_t)(long)width * height * bytes_per_pixel, fp_s));
  fclose(fp_s);
  return 0;
}

int Testbench::write_bmp(string outfile_name) {
  FILE *fp_t = NULL;      // target file handler
  unsigned int file_size; // file size

  fp_t = fopen(outfile_name.c_str(), "wb");
  if (fp_t == NULL) {
    printf("fopen %s error\n", outfile_name.c_str());
    return -1;
  }

  // file size
  file_size = width * height * bytes_per_pixel + output_rgb_raw_data_offset;
  header[2] = (unsigned char)(file_size & 0x000000ff);
  header[3] = (file_size >> 8) & 0x000000ff;
  header[4] = (file_size >> 16) & 0x000000ff;
  header[5] = (file_size >> 24) & 0x000000ff;

  // width
  header[18] = width & 0x000000ff;
  header[19] = (width >> 8) & 0x000000ff;
  header[20] = (width >> 16) & 0x000000ff;
  header[21] = (width >> 24) & 0x000000ff;

  // height
  header[22] = height & 0x000000ff;
  header[23] = (height >> 8) & 0x000000ff;
  header[24] = (height >> 16) & 0x000000ff;
  header[25] = (height >> 24) & 0x000000ff;

  // bit per pixel
  header[28] = bits_per_pixel;

  // write header
  fwrite(header, sizeof(unsigned char), output_rgb_raw_data_offset, fp_t);

  // write image
  fwrite(target_bitmap, sizeof(unsigned char),
         (size_t)(long)width * height * bytes_per_pixel, fp_t);

  fclose(fp_t);
  return 0;
}

void Testbench::do_sobel() {
  int x, y, v,z, u,temp_y,temp_x;        // for loop counter
  unsigned char temp_R, temp_G, temp_B,R,G,B; // color of R, G, B
  int adjustX, adjustY, xBound, yBound;
  int total,med_result,mean_result;
  word data;
  unsigned char mask[8];
  double StartTime, EndTime;
  //wait(5 * CLOCK_PERIOD, SC_NS);
  StartTime = clock();
  for (y = 0; y != (height+2); ++y) {           
    temp_y=y-2; 
     for (z = 0; z != (width); ++z){  /*temp bitmap buffer移動*/
        *(temp_bitmap + bytes_per_pixel * (0* width + z) + 2) = *(temp_bitmap + bytes_per_pixel * (1* width + z) + 2);
        *(temp_bitmap + bytes_per_pixel * (0* width + z) + 1) = *(temp_bitmap + bytes_per_pixel * (1* width + z) + 1);
        *(temp_bitmap + bytes_per_pixel * (0* width + z) + 0) = *(temp_bitmap + bytes_per_pixel * (1* width + z) + 0);
        *(temp_bitmap + bytes_per_pixel * (1* width + z) + 2) = *(temp_bitmap + bytes_per_pixel * (2* width + z) + 2);
        *(temp_bitmap + bytes_per_pixel * (1* width + z) + 1) = *(temp_bitmap + bytes_per_pixel * (2* width + z) + 1);
        *(temp_bitmap + bytes_per_pixel * (1* width + z) + 0) = *(temp_bitmap + bytes_per_pixel * (2* width + z) + 0);
        *(temp_bitmap + bytes_per_pixel * (2* width + z) + 2) = *(temp_bitmap + bytes_per_pixel * (3* width + z) + 2);
        *(temp_bitmap + bytes_per_pixel * (2* width + z) + 1) = *(temp_bitmap + bytes_per_pixel * (3* width + z) + 1);
        *(temp_bitmap + bytes_per_pixel * (2* width + z) + 0) = *(temp_bitmap + bytes_per_pixel * (3* width + z) + 0);
     } 
    for (x = 0; x != (width+2); ++x) {
      temp_x=x-2;
      adjustX = (MASK_X % 2) ? 1 : 0; // 1
      adjustY = (MASK_Y % 2) ? 1 : 0; // 1
      xBound = MASK_X / 2;            // 1
      yBound = MASK_Y / 2;            // 1
        for (v = -yBound; v != yBound + adjustY; ++v) {   //-1, 0, 1 只傳1行格
            if (x - 1 >= 0 && x - 1 < width && y + v >= 0 && y + v < height) {
              R = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v) + (x - 1)) + 2);
              G = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v) + (x - 1)) + 1);
              B = *(source_bitmap +
                    bytes_per_pixel * (width * (y + v) + (x - 1)) + 0);
            } else {
              R = 0;
              G = 0;
              B = 0;
            }
            if (x - 1 >= 0 && x - 1 < width && temp_y + v >= 0 && temp_y + v < height) {
              temp_R = *(temp_bitmap +
                    bytes_per_pixel * (width * (v+1) + (x - 1)) + 2);
              temp_G = *(temp_bitmap +
                    bytes_per_pixel * (width * (v+1) + (x - 1)) + 1);
              temp_B = *(temp_bitmap +
                    bytes_per_pixel * (width * (v+1) + (x - 1)) + 0);
            } else {
              temp_R = 0;
              temp_G = 0;
              temp_B = 0;
            }
            data.uc[0] = R;
            data.uc[1] = G;
            data.uc[2] = B;
            data.uc[4] = temp_R;
            data.uc[5] = temp_G;
            data.uc[6] = temp_B;
            mask[0] = 0xff;
            mask[1] = 0xff;
            mask[2] = 0xff;
            mask[3] = 0;                         /*temp_bitmap 資料尚未就緒*/
            mask[4] = 0xff;
            mask[5] = 0xff;
            mask[6] = 0xff;
            mask[7] = 0;
     
  
            initiator.write_to_socket(SOBEL_FILTER_R_ADDR, mask, data.uc, 8);
         wait(1 * CLOCK_PERIOD, SC_NS);
        }


        bool done=false;
        int output_num_med=0;
        int output_num_mean=0;

        while(!done){
          initiator.read_from_socket(SOBEL_FILTER_CHECK_ADDR, mask, data.uc, 8);
          output_num_med = data.sint[0];
          output_num_mean = data.sint[1];
          if(output_num_med>0) done=true;
        }
            wait(3 * CLOCK_PERIOD, SC_NS);
        initiator.read_from_socket(SOBEL_FILTER_RESULT_ADDR, mask, data.uc, 8);
        med_result = data.sint[0];
        mean_result = data.sint[1];

        //debug
        //cout << "Now at " << sc_time_stamp() << endl; //print current sc_time

        if(temp_x>=0){
          if(y<height){
          *(temp_bitmap + bytes_per_pixel * (3* width + temp_x) + 2) = med_result;
          *(temp_bitmap + bytes_per_pixel * (3* width + temp_x) + 1) = med_result;
          *(temp_bitmap + bytes_per_pixel * (3* width + temp_x) + 0) = med_result;
          }
          if(temp_y>=0){
            *(target_bitmap + bytes_per_pixel * (width * temp_y + temp_x) + 2) = mean_result;
            *(target_bitmap + bytes_per_pixel * (width * temp_y + temp_x) + 1) = mean_result;
            *(target_bitmap + bytes_per_pixel * (width * temp_y + temp_x) + 0) = mean_result;
          }
        }

      
    }
  }
  	EndTime = clock();
    cout << " Time:" << (EndTime - StartTime) / CLOCKS_PER_SEC << "ms" << endl;
  sc_stop();
}
