#include <cstdint>
#include <vector>
#include <x264.h>

#include "ssim.hh"

struct x264_pixel_function_t
{
  // 158 is the number of pointers in this internal x264 struct
  // this is obviously a bit fragile since x264 could add more
  void * ptrs[ 158 ];
};

// These are internal libx264 instructions so they aren't included in x264.h
extern "C"
{
  float x264_pixel_ssim_wxh( const x264_pixel_function_t * func, const uint8_t *pix1,
                             uintptr_t stride1, const uint8_t *pix2, uintptr_t stride2,
                             int width, int height, void *buf, int *cnt );

  void x264_pixel_init( int cpu, x264_pixel_function_t *pixf );

  uint32_t x264_cpu_detect( void );
}

x264_pixel_function_t init_pixel_function( void )
{
  x264_pixel_function_t pix_func;
  x264_pixel_init( x264_cpu_detect(), &pix_func );

  return pix_func;
}

x264_pixel_function_t x264_funcs = init_pixel_function();

template<class TwoDType>
double ssim( const TwoDType & image, const TwoDType & other_image )
{
   std::vector<uint8_t> tmp_buffer;
   int count;
   // Buffer size calculation taken from x264
   tmp_buffer.resize( 8 * ( image.width() / 4 + 3 ) * sizeof( int ) );

   // No padding so stride = width
   double ssim = x264_pixel_ssim_wxh( &x264_funcs, &image.at( 0, 0 ), image.stride(),
                                      &other_image.at( 0, 0 ), other_image.stride(),
                                      image.width(), image.height(),
                                      tmp_buffer.data(), &count );

   return ssim / count;
}

template double ssim( const TwoD<uint8_t> & image, const TwoD<uint8_t> & other_image );
template double ssim( const TwoDSubRange<uint8_t, 16, 16> & image, const TwoDSubRange<uint8_t, 16, 16> & other_image );
