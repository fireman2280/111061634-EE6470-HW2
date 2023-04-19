#include <cmath>
#include <iomanip>

#include "SobelFilter.h"

SobelFilter::SobelFilter(sc_module_name n)
    : sc_module(n), t_skt("t_skt"), base_offset(0) {
  SC_THREAD(do_filter);
  t_skt.register_b_transport(this, &SobelFilter::blocking_transport);
}
int read_count=0;
int write_count=0;

unsigned int get_med(unsigned char sort[9]){   /*med funtion*/
int i, j, tmp;
unsigned char temp[9]={0,0,0,0,0,0,0,0,0};
for(int i=0;i<9;i++){

 temp[i]=sort[i];

}

for(i = 8; i > 0; i--)
{
    for(j = 0; j <= i-1; j++)
    {
        if( temp[j] > temp[j+1])
        {
            tmp = temp[j];
            temp[j] = temp[j+1];
            temp[j+1] = tmp;
        }
    }
}
return (int)(temp[4]);
}

// sobel mask
const int mask[MASK_X][MASK_Y] = {{1, 1, 1}, {1, 2, 1}, {1, 1, 1}};

                                        

void SobelFilter::do_filter() {
  unsigned char med_buffer[(MASK_X*MASK_Y)]={0};
  unsigned char mean_buffer[MASK_X][MASK_Y]={{0,0,0},{0,0,0},{0,0,0}};
  while (true) {
    for (unsigned int v = 0; v < MASK_Y; ++v) 
      med_buffer[v] = (i_med_r.read()+i_med_g.read()+i_med_b.read())/3;

      int med_temp=get_med(med_buffer);

      o_med_result.write(med_temp);

    for ( int v = 8; v >=3; --v) {            /*shift buffer*/
      med_buffer[v]=med_buffer[v-3];
    }

    int val_mean=0;
    for (unsigned int v = 0; v < MASK_Y; ++v) {
      for (unsigned int u = 0; u < MASK_X; ++u) {
        if(u==0){ 
          mean_buffer[0][v] = (i_mean_r.read() + i_mean_g.read() + i_mean_b.read()) / 3;
        }
        val_mean +=mean_buffer[u][v]*mask[u][v];

      }
    }
    int mean_result =(int)(val_mean/(MASK_Y*MASK_X));
    o_mean_result.write(mean_result);


    for (unsigned int v = 0; v < MASK_Y; ++v) {
        for (unsigned int u = (MASK_X-1); u > 0; --u) {      //shift col1->col2 col0->col1
            mean_buffer[u][v] = mean_buffer[u-1][v] ;
        }
    }

  }
  
}


void SobelFilter::blocking_transport(tlm::tlm_generic_payload &payload,
                                     sc_core::sc_time &delay) {
  sc_dt::uint64 addr = payload.get_address();
  addr = addr - base_offset;
  unsigned char *mask_ptr = payload.get_byte_enable_ptr();
  unsigned char *data_ptr = payload.get_data_ptr();
  word buffer;
  switch (payload.get_command()) {
  case tlm::TLM_READ_COMMAND:
    switch (addr) {
    case SOBEL_FILTER_RESULT_ADDR:
      buffer.uint[0] = o_med_result.read();
      buffer.uint[1] = o_mean_result.read();
      break;
    case SOBEL_FILTER_CHECK_ADDR:
      buffer.uint[0] = o_med_result.num_available();
      buffer.uint[1] = o_mean_result.num_available();
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    data_ptr[0] = buffer.uc[0];
    data_ptr[1] = buffer.uc[1];
    data_ptr[2] = buffer.uc[2];
    data_ptr[3] = buffer.uc[3];
    data_ptr[4] = buffer.uc[4];
    data_ptr[5] = buffer.uc[5];
    data_ptr[6] = buffer.uc[6];
    data_ptr[7] = buffer.uc[7];
    break;

  case tlm::TLM_WRITE_COMMAND:
    switch (addr) {
    case SOBEL_FILTER_R_ADDR:
      if (mask_ptr[0] == 0xff) {
        i_med_r.write(data_ptr[0]);
      }
      if (mask_ptr[1] == 0xff) {
        i_med_g.write(data_ptr[1]);
      }
      if (mask_ptr[2] == 0xff) {
        i_med_b.write(data_ptr[2]);
      }
      if (mask_ptr[4] == 0xff) {
        i_mean_r.write(data_ptr[4]);
      }
      if (mask_ptr[5] == 0xff) {
        i_mean_g.write(data_ptr[5]);
      }
      if (mask_ptr[6] == 0xff) {
        i_mean_b.write(data_ptr[6]);
      }
      break;
    default:
      std::cerr << "Error! SobelFilter::blocking_transport: address 0x"
                << std::setfill('0') << std::setw(8) << std::hex << addr
                << std::dec << " is not valid" << std::endl;
      break;
    }
    break;

  case tlm::TLM_IGNORE_COMMAND:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  default:
    payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }
  payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
}
